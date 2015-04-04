/*********************************************************************/
/*This is the client. It will perform the following tasks:*/
/*********************************************************************/
/*  -Daemon process                                                  */
/*  -Monitors directories for changes via iNotify                    */
/*  -Automatically creates a Metainfo (.torrent) for each directory  */
/*  -Informs peers of a change by spreading metainfo file to tracker */
/*  -Listens for updates from peers, and will reflect the update     */
/*  -Allows for file adding, removing, renaming, & modification      */
/*  -NAT & Gateway: automatic port choosing & UPnP usage             */
/*  -User can override automatic network settings                    */
/*  -Users must authenticate a peer                                  */
/*  -Filetransfer between peers will utilize BitTorrent Protocol     */
/*********************************************************************/
//TO-DO List:
//1): Calculate info_hash
//2): update read_share function
//3): finish update_share
//4): put read_shares and update_shares in the correct places
//5): Set update timer according to config tool

#include <string>
#include <vector>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <map>
#include <pthread.h>
#include <signal.h>
#include "inotify.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/file_pool.hpp"
#include "libtorrent/session.hpp"
#include "confClass.h"
#include "boost/filesystem.hpp"
#include <curl/curl.h>

/* Create a file descriptor timer and return the file descriptor */
int create_timer(int interval);

/*Prevent hidden files beginning with '.'*/
/*from being torrented*/
bool file_filter(const std::string & f);

/*Parses a config file and loads info into a confInfo class object*/
int configParse(std::ifstream * fin, confInfo * ci);

/*Does the file exist?*/
bool existingFile (const std::string & name);

/*Creates a torrent file*/
int torCreate (DirectoryInfo * DI);

/*Saves libcurl responses to string*/
size_t write_to_string(void *ptr, size_t size, size_t count, void * stream);

/*Pings the read_share url for a share*/
string read_share(string &trackerDomain, string &peerID, string &share_id);

/*This downloads the torrent file and places it at the correct path*/
int download_torrent(libtorrent::session *s, DirectoryInfo * DI);

/*thread function for reading_shares*/
void * read_share_timer(void * CI);

/*cntl+c signal handler*/
void cntl_c_handler(int dummy);

/*read_share thread*/
pthread_t tcb;
void * status;

/*When cntl+c is hit, this changes to 1 and read_share_timer thread breaks loop*/
int stopLoop = 0;

namespace fs = boost::filesystem;

int main(int argc, const char* argv[])
{

	/*set cntl-c sig handler*/
	signal(SIGINT, cntl_c_handler);
	
	/*Directory Monitoring*/
    Inotify notify;

	/*libtorrent variables*/
	libtorrent::session sess;
	libtorrent::error_code ec;
	std::string creator_str = "libtorrent";
	std::string comment_str;
	std::string outfile;
	std::string merklefile;
	std::map<string, DirectoryInfo *>::iterator mit;

	/*General Variables*/
    std::vector<fs::path> watched_directories;
	
	/*Kept a string version of the vector because I was using it for stuff*/
	std::vector<std::string> watched_directories2;
    int timer_fd, bytes_read; //update_interval = 1;
    char buffer[1024];

	/*Open Configuration File for parsing*/
	std::ifstream * fin = new std::ifstream;
	fin->open("../Config.conf");

	/*Write downloaded torrents...*/
	std::ofstream fout;

	/*Config file fail check*/
	if(fin->fail()){
		fprintf(stderr, "ERROR: Could not open config. Exiting.\n");
		exit(-1);
	}

	/*The confInfo object*/
	confInfo * CI = new confInfo;

	/*configParse(fin, CI) parses opened config file and fills confInfo class appropriately*/
	if(configParse(fin, CI) != 0){
		fprintf(stderr, "ERROR: parsing of configuration file failed; exiting\n");
		exit(EXIT_FAILURE);
	}

	/*No directories to monitor... exiting*/
	if(CI->DI.size() == 0){
		fprintf(stderr, "ERROR: Config file specifies no directories...\n");
		exit(EXIT_FAILURE);
	}
	
	/*Loop through directories found in config file...*/
	/*Check for existing metainfo: if it does, does it need updates?*/
	/*If it doesn't exist, create it!*/
	for(mit=CI->DI.begin(); mit!=CI->DI.end(); mit++){

		printf("\nMonitoring: %s\n", mit->second->directoryPath.c_str());	
		
		/*Maurice's Code*/
		fs::path directory(mit->second->directoryPath.c_str());
		fs::path canonical_directory = boost::filesystem::canonical(directory);
		watched_directories2.push_back(mit->second->directoryPath);
		watched_directories.push_back(canonical_directory);
		notify.AddWatchRecursive(canonical_directory);

		/*Does torrent file exist? If not, create one!*/
		/*This logic needs a clean-up; Need to read_share & update accordingly*/
		/*If read_share detects no file, create one and update_share*/

		if(!existingFile(mit->second->torrentPath)){

			/*I should hit read_share here & see if torrent exists already*/
			
			if(!(torCreate(mit->second))){
				fprintf(stderr, "ERROR: unable to create metainfo.. continuing\n");
			}

		}else{
		/*Torrent file exists!*/
	
			printf("\nTorrent file already exists! Reading share for updates...\n");
			
			string response = read_share(CI->trackerDomain, CI->peerID, mit->second->share_id);
			printf("Read share was succesful! (Share not written here yet though)\n");
		}
	}

	/*libtorrent; open session to communicate w/ peers*/
	sess.listen_on(std::make_pair(6881, 6889), ec);

	if(ec){
		fprintf(stderr, "failed to open listen socket: %s\n", ec.message().c_str());
		exit(EXIT_FAILURE);
	}

	/*Not calling stop_upnp() anywhere or manually port mapping;*/
	sess.start_upnp();
	
	if(pthread_create(&tcb, NULL, read_share_timer, CI) != 0){
		perror("read_share_timer");
		exit(1);
	}

	// Create timer
    timer_fd = create_timer(60);
	//timer_fd = create_timer(update_interval);

    while (1) {
        
		printf("\nModification checker loop tick\n");
		fflush(stdout);

		bytes_read = read(timer_fd, &buffer, 1024);
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Read the events in the event-queue, updating the directory tree in the process
        notify.ReadEvents();

        // Check status of each watched directory
        for(unsigned int i=0; i<watched_directories.size(); i++) {
            
			fs::path w_directory = watched_directories[i];
			std::string directory = watched_directories2[i];
            enum UPDATE_FLAG flag = notify.GetUpdateFlag(w_directory);

            if (flag == MODIFIED) {
                
				// update torrent in here! (create updated metainfo & ping tracker w/ it)
                printf("    %s: MODIFIED\n", directory.c_str());

				/*First, hit read_share for update if there is one. Else, torCreate and update_share*/
				if(!(torCreate(CI->DI[directory]))){
					printf("There was a problem updating the torrent file.\n");
				}else{
					printf("Succesfully updated: %s\n", w_directory.c_str());
				}
			}
            else if (flag == DOESNOTEXIST) {
                // do something
					printf("DOESNOTEXIST\n");
            }
        }
    }
    
    if (close(timer_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

/*Parse the configuration file and load up the confInfo class*/
/*Probably would have been better form to make this OO and put it in the confInfo class*/
int configParse(std::ifstream *fin, confInfo * ci){

	std::string s="", header="";
	std::ifstream idFin;
	std::istringstream ss;

	/*Variable determining if we've passed the daemon & network line*/
	/*If we have, we're now reading directory info*/
	int directories = 0;

	while(getline(*fin, s)){
		//printf("Line: %s\n", s.c_str());
		
		header = "";
		ss.clear();
		ss.str(s);
		ss >> header;

		if(!directories){

			/*Reading Daemon info*/
			if(header == "[daemon]"){
				
				/*getline() followed by ss >> s... is for skipping "PeerIDFile =" and grabbing the value*/
				/*I should write a 3-line function that does this*/
				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> ci->PeerIDFile;
				
				idFin.open(ci->PeerIDFile);
				
				if(idFin.fail()){
					fprintf(stderr, "ERROR: Couldn't read PeerID at:%s \nExiting.\n", ci->PeerIDFile.c_str());
					perror("filemsg:");
					exit(1);
				}

				getline(idFin, s);
				ss.clear();
				ss.str(s);
				ss >> ci->peerID;

				//printf("Got the peerID: %s\n", ci->peerID.c_str());

			/*Reading Network info*/
			}else if(header == "[network]"){

				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> ci->trackerPassword;

				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> ci->trackerUsername;
				
				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> ci->trackerDomain;

				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;

				/*TO-DO:Need to check here to see if it's auto*/
				ss >> ci->DaemonPort;

//				printf("Reached the network header\n");
				directories = 1;
			}
		}else{
			
			/*Reading a directory from config*/
			/*Clearly I should be performing a better check here to see if it's directory*/
			if(header[0] == '['){
				DirectoryInfo * DI = new DirectoryInfo;

//				printf("Reading a directory\n");
				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> DI->directoryPath;
				
				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> DI->torrentPath;

				getline(*fin, s);
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> DI->scanRate;

				getline(*fin, s);
				
				ss.clear();
				ss.str(s);
				ss >> s;
				ss >> s;
				ss >> DI->share_id;

				ci->DI.insert(make_pair(DI->directoryPath, DI));
			}
		}

	}
	
	return 0;
}

/*maintains synchronized update-check time*/
int create_timer(int interval)
{
		int timer_fd_;
		struct itimerspec time;

		// Set interval
		time.it_value.tv_sec = interval;
		time.it_interval.tv_sec = interval;
		time.it_value.tv_nsec = 0;
		time.it_interval.tv_nsec = 0;

		timer_fd_ = timerfd_create(CLOCK_MONOTONIC, 0);
		if (timer_fd_ == -1) {
				perror("timerfd_create");
				exit(EXIT_FAILURE);
		}

		if (timerfd_settime(timer_fd_, 0, &time, NULL) == -1) {
				perror("timerfd_settime");
				exit(EXIT_FAILURE);
		}

		return timer_fd_;
}

/*Ripped from libtorrent examples*/
/*Prevent hidden files from being added*/
bool file_filter(std::string const& f){

	if(libtorrent::filename(f)[0] == '.') return false;
	fprintf(stderr, "%s\n", f.c_str());
	return true;
}


/*Does the file exist?*/
bool existingFile (const std::string & name){

		struct stat buffer;
		return (stat (name.c_str(), &buffer) == 0);
}

/*Create the torrent. Need to have this generate the info_hash.*/
int torCreate (DirectoryInfo * DI){

		int flags = 0;
		
		libtorrent::file_storage fs;
			
		std::string full_path = libtorrent::complete(DI->directoryPath.c_str());
		add_files(fs, full_path, file_filter, flags);

		if(fs.num_files() == 0){
			fputs("no files specified.\n", stderr);
			return 0;
		}else{
			printf("fs.num_files() > 0!\n");
		}
				
		libtorrent::create_torrent t(fs);
		t.add_tracker("http://tempTracker.com/");
		t.set_creator("creatorExample");
		
		/*set_piece_hashes requires the path of the directory containing the monitored directory..*/
		/*Need to remove the directory name...*/
		/*Also need to check to make sure this is necessary first! (depends on full_path)*/
		std::string tmp = string(full_path.rbegin(), full_path.rend());
		std::string::size_type n;
		n = tmp.find('/', 0);
		tmp = tmp.substr(n);
		tmp = string(tmp.rbegin(), tmp.rend());

		/*set_piece_hashes requires the path of the directory containing the monitored directory*/
		set_piece_hashes(t, tmp);

		ofstream out(DI->torrentPath.c_str(), std::ios_base::binary|std::ios_base::trunc);
		libtorrent::bencode(std::ostream_iterator<char>(out), t.generate());

		out.close();
	
		return 1;
}

/*Passed to libcurl to write HTTP responses to string */
size_t write_to_string(void *ptr, size_t size, size_t count, void * stream){
	((string *)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

/*Pings the read_share url to get a share*/
/*Need to change to send w/ info_hash*/
/*Still works*/
string read_share(string &trackerDomain, string &peerID, string &share_id){
		
		string response = "";
		string s = "";
		s+=trackerDomain;
		s+="read_share/?peer_id=";
		s+=peerID;
		s+="&share_id=";
		s+=share_id;

		printf("\nhere is the formatted read_share request string: %s\n", s.c_str());
		CURL *curl;
		CURLcode res;
		curl = curl_easy_init();

		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, s.c_str());
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		
			res = curl_easy_perform(curl);

			if(res != CURLE_OK){
				fprintf(stderr, "curl_easy_perform() GET failed: %s\n", curl_easy_strerror(res));
			}

			printf("The response: %s\n", response.c_str());
			curl_easy_cleanup(curl);
		}

	return response;
}

/*UNTESTED&&INCOMPLETE*/
/*Update a share!*/
string update_share(string &trackerDomain, string &share_id, string &info_hash, string &torrentPath){

	CURL *curl;
	CURLcode res;
	string response = "";
	curl = curl_easy_init();
	
	if(curl){
		struct curl_httppost *post=NULL;
		struct curl_httppost *last=NULL;
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "share_id", CURLFORM_COPYCONTENTS, share_id.c_str(), CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "info_hash", CURLFORM_COPYCONTENTS, info_hash.c_str(), CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "share_file", CURLFORM_FILECONTENT, torrentPath.c_str(),CURLFORM_END);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	
		res = curl_easy_perform(curl);

		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() POST failed: %s\n", curl_easy_strerror(res));
		}
		curl_formfree(post);
	}

	return response;
}

/*INCOMPLETE*/
/*Need to finish this; stopped because info_hash isn't set*/
int download_torrent(libtorrent::session *s, DirectoryInfo * DI){

	libtorrent::error_code ec;

	libtorrent::add_torrent_params p;
	p.save_path = DI->directoryPath;
	p.ti = new libtorrent::torrent_info(DI->torrentPath, ec);
	if(ec){
		fprintf(stderr, "%s\n", ec.message().c_str());
		return -1;
	}

	printf("info_hash: %s\n", p.info_hash.to_string().c_str());

	return 0;
}

/*Thread function checking for directory meta-info updates on an interval*/
/*Currently set to 60 seconds*/
void * read_share_timer(void * CI){
	
	std::ofstream fout;
	std::map<string, DirectoryInfo *>::iterator mit;
	
	confInfo * ci = (confInfo *)CI;
	
	while(1){
		
		sleep(60);
		
		for(mit = ci->DI.begin(); mit != ci->DI.end(); mit++){
	
			string response = read_share(ci->trackerDomain, ci->peerID, mit->second->share_id);
			//fout.open(mit->second->torrentPath);
			//fout << response;
			//fout.close();
		}

		/*stopLoop is set in the cntl+c signal handler*/
		if(stopLoop)break;
		
	}

	return NULL;
}

/*cntl_c_handler*/
void cntl_c_handler(int dummy){

	signal(SIGINT, cntl_c_handler);
	printf("\nYou just typed cntl-c\n");
	stopLoop = 1;
	if(pthread_join(tcb, &status) != 0){
		perror("pthread_join");
		exit(1);
	}
	exit(1);
}
