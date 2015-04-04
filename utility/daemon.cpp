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
#include "confClass.hpp"
#include "boost/filesystem.hpp"
#include <curl/curl.h>
#include <openssl/sha.h>

/* Create a file descriptor timer and return the file descriptor */
int create_timer(int interval);

/*Does the file exist?*/
bool existingFile (const std::string & name);

/*thread function for reading_shares*/
void * read_share_timer(void * CI);

/*cntl+c signal handler*/
void cntl_c_handler(int dummy);

/*Creates an info hash given a directory*/
//void create_info_hash(DirectoryInfo * DI);

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
	if(CI->configParse(fin) != 0){
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
			
			if(!(CI->torCreate(mit->second))){
				fprintf(stderr, "ERROR: unable to create metainfo.. continuing\n");
			}

		}else{
		/*Torrent file exists!*/
	
			printf("\nTorrent file already exists! Reading share for updates...\n");
			
			string response = CI->read_share(mit->second);
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
				if(!(CI->torCreate(CI->DI[directory]))){
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

/*Does the file exist?*/
bool existingFile (const std::string & name){

		struct stat buffer;
		return (stat (name.c_str(), &buffer) == 0);
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
	
			string response = ci->read_share(mit->second);
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

/*Creates an info_hash given a directory*/
//void create_info_hash(DirectoryInfo * DI){

//		std::ifstream fin(DI->torrentPath);
//		std::string s((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
//		SHA1((unsigned char *)s.c_str(), strlen(s.c_str()), DI->info_hash);
		
//}
