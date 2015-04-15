/*********************************************************************/
/*This is the client. It will perform the following tasks:           */
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
//1): put read_shares and update_shares in the correct places
//2): Set update timer according to config tool

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
#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/peer_info.hpp"
#include "confClass.hpp"
#include "boost/filesystem.hpp"
#include <curl/curl.h>
#include <openssl/sha.h>
#include "libtorrent/torrent_handle.hpp"
#include "boost/asio.hpp"
/* Create a file descriptor timer and return the file descriptor */
int create_timer(int interval);

/*Does the file exist?*/
bool existFile (const std::string & name);

/*thread function for reading_shares*/
void * read_share_timer(void * CI);

/*cntl+c signal handler*/
void cntl_c_handler(int dummy);

/*read_share thread*/
pthread_t tcb;
void * status;

/*Prints the welcome message*/
void printWelcome();

/*Prints the exit message*/
void printGoodbye();

/*When cntl+c is hit, this changes to 1 and read_share_timer thread breaks loop*/
int stopLoop = 0;

namespace fs = boost::filesystem;

pthread_mutex_t lock;
libtorrent::torrent_status ts;
libtorrent::torrent_handle th1;
std::vector<libtorrent::announce_entry> announcers;
std::vector<libtorrent::peer_info> peers;
int main(int argc, const char* argv[])
{

	printWelcome();

	/*set cntl-c sig handler*/
	signal(SIGINT, cntl_c_handler);

	/*Directory Monitoring*/
	Inotify notify;

	/*libtorrent variables*/
	libtorrent::session sess;
	libtorrent::session_settings sessSet;
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

	/*set libtorrent settings*/
	//sessSet.user_agent = CI->peerID;
	libtorrent::sha1_hash sHash(CI->peerID);
	libtorrent::peer_id pid= sHash;
	sess.set_peer_id(pid);
	sessSet.announce_ip = "10.0.0.30";
	sessSet.allow_multiple_connections_per_ip = true;
	sess.set_settings(sessSet);
	
	/*libtorrent; open session to communicate w/ peers*/
	sess.listen_on(std::make_pair(6882, 6882), ec);

	if(sess.is_listening()){
		printf("Successfully listening!\n");	
	}else{
		fprintf(stderr, "unsuccessfully listening :(\n");
	}
	/*Not calling stop_upnp() anywhere or manually port mapping;*/
//	sess.start_upnp();

	/*Loop through directories found in config file...*/
	/*Check for existing metainfo: if it does, does it need updates?*/
	/*If it doesn't exist, create it!*/
	printf("\n/******* Initialization *******/\n");
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

		if(!existFile(mit->second->torrentPath)){

			/*I should hit read_share here & see if torrent exists already*/

//			printf("\n%s not detected\n", mit->second->torrentPath.c_str());
		
			/*This is next*/
			int response = CI->read_share(mit->second);

			if(response == 204){
				if(!(CI->torCreate(mit->second))){
					fprintf(stderr, "ERROR: unable to create metainfo.. continuing\n");
				}

				int ret = CI->update_share(mit->second);
				printf("ret: %d\n", ret);
			}

			if(response == 200){
				
				pthread_mutex_lock(&lock);
				int ret = CI->download_torrent(&sess, mit->second);
				if(ret != 0){
					printf("ERROR: download_torrent failed\n");
				}
				pthread_mutex_unlock(&lock);
			}

		}else{
			/*Torrent file exists!*/

			printf("\n%s detected\n", mit->second->torrentPath.c_str());

			pthread_mutex_lock(&lock);
			int response = CI->read_share(mit->second);
			if(response == 204){
				int ret = CI->update_share(mit->second);
				printf("ret: %d\n", ret);
			}
			if(response == 200){
				int ret = CI->download_torrent(&sess, mit->second);
				if(ret != 0){
					printf("ERROR: download_torrent failed\n");
				}
			}
			pthread_mutex_unlock(&lock);
			/*This was for testing*/
			//response = CI->update_share(mit->second);
			//printf("\nRead share for %s in initialization loop... not checking result atm...\n", mit->second->directoryPath.c_str());
		}

		libtorrent::add_torrent_params p;
		p.save_path = mit->second->directoryPath;
		p.ti = new libtorrent::torrent_info(mit->second->torrentPath.c_str(), ec);
		if(ec){
			fprintf(stderr, "%s\n", ec.message().c_str());
			return 1;
		}
		announcers = p.ti->trackers();
		//p.ti->add_tracker("http://home.elemnir.com:8000/tracker/",0);
		p.flags = p.flag_auto_managed;
		libtorrent::torrent_handle th = sess.add_torrent(p, ec);
		printf("Just added %s to session...\n", mit->second->torrentPath.c_str());

		if(ec){
			fprintf(stderr, "%s\n", ec.message().c_str());
			return 1;
		}
		th1 = th;
		ts = th.status();
		th.get_peer_info(peers);

		printf("Error?: %s\ntracker?: %s\n",ts.error.c_str(), ts.current_tracker.c_str());
		
		if(ec){
			fprintf(stderr, "%s\n", ec.message().c_str());
			return 1;
		}

	}

	printf("\n/******* Init Complete *******/\n");

	if(ec){
		fprintf(stderr, "failed to open listen socket: %s\n", ec.message().c_str());
		exit(EXIT_FAILURE);
	}

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
		ts = th1.status();
		announcers = th1.trackers();
		th1.get_peer_info(peers);
		printf("numPeers: %u\n", peers.size());
		
		for(unsigned int k=0; k<peers.size(); k++){
			//peers[k].
			boost::asio::ip::address remote_ad = peers[k].ip.address();
			unsigned short remote_port = peers[k].ip.port();
			printf("	peer #%u ip: %s port: %hd\n", k, remote_ad.to_string().c_str(), remote_port);

			if((peers[k].flags && 1) == 1){
				printf("	peer #%u is interesting\n", k);
			}
			if((peers[k].flags && 2) == 2){
				printf("	peer #%u is choked\n", k);
			}
			if((peers[k].flags && 4) == 4 ){
				printf("	peer #%u is remote interested\n", k);
			}
			if((peers[k].flags && 8) == 8){
				printf("	peer #%u is remote choked\n", k);
			}
			if((peers[k].flags && 16) == 16){
				printf("	peer #%u is supporting extensions\n", k);
			}
			if((peers[k].flags && 32) == 32){
				printf("	peer #%u is a local connection\n", k);
			}
			if((peers[k].flags && 64) == 64){
				printf("	peer #%u is in handshake\n", k);
			}
			if((peers[k].flags && 128) == 128){
				printf("	peer #%u is connecting\n", k);
			}
			if((peers[k].flags && 256) == 256){
				printf("	peer #%u is queued\n", k);
			}
			

		}

		printf("Error?: %s\ntracker?: %s\n",ts.error.c_str(), ts.current_tracker.c_str());

		printf("announcers!: \n");
		for(unsigned int i=0; i<announcers.size(); i++){
			printf("url: %s\n",announcers[i].url.c_str());

			printf("next_announce: %d\n", announcers[i].next_announce_in());
			printf("min_announce_in: %d\n", announcers[i].min_announce_in());
			printf("server message: %s\n", announcers[i].message.c_str());
			if(announcers[i].last_error){
				fprintf(stderr, "error?: %s\n", announcers[i].last_error.message().c_str());
			}
		}

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
					printf("\nThere was a problem updating the torrent file.\n");
				}else{

					pthread_mutex_lock(&lock);
					int response = CI->update_share(CI->DI[directory]);

					/*Need to handle this..*/
					if(response != 200){
						fprintf(stderr,"ERROR: tracker update unsuccessful...\n");
					}else{
						printf("\nSuccessfully updated: %s\n", w_directory.c_str());
					}

					pthread_mutex_unlock(&lock);
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
bool existFile (const std::string & name){

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

		printf("\nread_share_timer tick\n");

		for(mit = ci->DI.begin(); mit != ci->DI.end(); mit++){

			pthread_mutex_lock(&lock);
			int response = ci->read_share(mit->second);
			printf("read_share return: %d\n", response);
			if(response == 204){
				int ret = ci->update_share(mit->second);
				printf("ret: %d\n", ret);
			}
			pthread_mutex_unlock(&lock);

		}

		/*stopLoop is set in the cntl+c signal handler*/
		if(stopLoop)break;

	}

	return NULL;
}

/*cntl_c_handler*/
void cntl_c_handler(int dummy){

	signal(SIGINT, cntl_c_handler);
	printf("\n\n/******* Shut down initialized  *******/\n\nWaiting for next read_share_timer tick to end process...\n");
	stopLoop = 1;
	if(pthread_join(tcb, &status) != 0){
		perror("pthread_join");
		printGoodbye();
		exit(1);
	}
	printGoodbye();
	exit(1);
}

void printWelcome(){

	printf("\n\n/*********************************************************/\n");
	printf("\n   ____                   ____  ____  ____ \n");
	printf("  / __ \\____  ___  ____  / __ )/ __ \\/ __ \\\n");
	printf(" / / / / __ \\/ _ \\/ __ \\/ __  / / / / /_/ /\n");
	printf("/ /_/ / /_/ /  __/ / / / /_/ / /_/ / _, _/ \n");
	printf("\\____/ .___/\\___/_/ /_/_____/_____/_/ |_|  \n");
	printf("    /_/                                    \n");
}

void printGoodbye(){
	printf("\n\n/******* Shutting down *******/\n\n   ______                ____             \n");
	printf("  / ____/___  ____  ____/ / /_  __  _____ \n");
	printf(" / / __/ __ \\/ __ \\/ __  / __ \\/ / / / _ \\\n");
	printf("/ /_/ / /_/ / /_/ / /_/ / /_/ / /_/ /  __/\n");
	printf("\\____/\\____/\\____/\\__,_/_.___/\\__, /\\___/ \n");
	printf("                             /____/       \n");
	printf("\n\n/*********************************************************/\n\n");
}
