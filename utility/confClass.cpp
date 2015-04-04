/*Includes libtorrent wrapper functions, http curl functions, and configParsing*/
/*Some of this stuff could use some re-working*/

#include "confClass.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <curl/curl.h>
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/file_pool.hpp"
#include "libtorrent/session.hpp"

/*Passed to libcurl to write HTTP responses to string*/
size_t write_to_string(void *ptr, size_t size, size_t count, void * stream);

/*prevent hidden files from being added to torrents*/
bool file_filter(std::string const& f);

/*Parse the configuration file and load up the confInfo class*/
/*Probably would have been better form to make this OO and put it in the confInfo class*/
int confInfo::configParse(ifstream * fin){

		std::string s="", header="";
		std::ifstream idFin;
		std::istringstream ss;

		/*Variable determining if we've passed the daemon & network line*/
		/*If we have, we're now reading directory info*/
		int directories = 0;

		while(getline(*fin, s)){
//				printf("Line: %s\n", s.c_str());

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
								ss >> PeerIDFile;

								idFin.open(PeerIDFile);

								if(idFin.fail()){
										fprintf(stderr, "ERROR: Couldn't read PeerID at:%s \nExiting.\n", PeerIDFile.c_str());
										perror("filemsg:");
										exit(1);
								}

								getline(idFin, s);
								ss.clear();
								ss.str(s);
								ss >> peerID;

//								printf("Got the peerID: %s\n", peerID.c_str());

								/*Reading Network info*/
						}else if(header == "[network]"){

								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;
								ss >> trackerPassword;

								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;
								ss >> trackerUsername;

								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;
								ss >> trackerDomain;

								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;

								/*TO-DO:Need to check here to see if it's auto*/
								ss >> DaemonPort;

								//              printf("Reached the network header\n");
								directories = 1;
						}
				}else{
						/*Reading a directory from config*/
						/*Clearly I should be performing a better check here to see if it's directory*/
						if(header[0] == '['){
								DirectoryInfo * newDI = new DirectoryInfo;

//								printf("Reading a directory\n");
								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;
								ss >> newDI->directoryPath;

								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;
								ss >> newDI->torrentPath;

								getline(*fin, s);
								ss.clear();
								ss.str(s);
								ss >> s;
								ss >> s;
								ss >> newDI->scanRate;

								getline(*fin, s);

								ss.clear();
								ss.str(s);

								ss >> s;
								ss >> s;
								ss >> newDI->share_id;

								//              if(existingFile(DI->torrentPath)){
								//                  create_info_hash(DI);
								//                  printf("\n\nHash: ");
								//                  for(int i=0; i<20; i++){
								//                      printf("%c ", (char)(DI->info_hash[i]));
								//                  }
								//                  printf("\n\n");
								//              }
								DI.insert(make_pair(newDI->directoryPath, newDI));
						}
				}

		}

		return 0;
}

/*Pings the read_share tracker url to get a share*/
/*Need to change to send w/ info_hash*/
/*Still works*/
string confInfo::read_share(DirectoryInfo *DI){

		/*Pings the read_share url to get a share*/
		/*Need to change to send w/ info_hash*/
		/*Still works*/

		string response = "";
		string s = "";
		s+=trackerDomain;
		s+="read_share/?peer_id=";
		s+=peerID;
		s+="&share_id=";
		s+=DI->share_id;
		//      s+="&info_hash=";
		//      s.append((char *)(DI->info_hash));

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

/*Update a share on the tracker*/
string confInfo::update_share(DirectoryInfo *DI){

		CURL *curl;
		CURLcode res;
		string response = "";
		curl = curl_easy_init();

		if(curl){
				struct curl_httppost *post=NULL;
				struct curl_httppost *last=NULL;
				curl_formadd(&post, &last, CURLFORM_COPYNAME, "share_id", CURLFORM_COPYCONTENTS, DI->share_id.c_str(), CURLFORM_END);
				//curl_formadd(&post, &last, CURLFORM_COPYNAME, "info_hash", CURLFORM_COPYCONTENTS, DI->info_hash, CURLFORM_END);
				curl_formadd(&post, &last, CURLFORM_COPYNAME, "share_file", CURLFORM_FILECONTENT, DI->torrentPath.c_str(),CURLFORM_END);
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

/*Passed to libcurl to write HTTP responses to string */
size_t write_to_string(void *ptr, size_t size, size_t count, void * stream){
		((string *)stream)->append((char*)ptr, 0, size*count);
		return size*count;
}

int confInfo::torCreate(DirectoryInfo * DI){

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

/*INCOMPLETE*/
/*Need to finish this; stopped because info_hash isn't set*/
int confInfo::download_torrent(libtorrent::session *s, DirectoryInfo * DI){

		libtorrent::error_code ec;

		libtorrent::add_torrent_params p;
		p.save_path = DI->directoryPath;
		p.ti = new libtorrent::torrent_info(DI->torrentPath, ec);
		if(ec){
				fprintf(stderr, "%s\n", ec.message().c_str());
				return -1;
		}

		//char ih[41];
		//libtorrent::torrent_info t = p.ti;

		//to_hex((char const*)&ti.info_hash()[0], 20, ih);
		//printf("info_hash: %s\n", ih);

		return 0;
}


/*Ripped from libtorrent examples*/
/*Prevent hidden files from being added*/
bool file_filter(std::string const& f){

		if(libtorrent::filename(f)[0] == '.') return false;
		fprintf(stderr, "%s\n", f.c_str());
		return true;
}

