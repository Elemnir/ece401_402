/*Includes libtorrent wrapper functions, http curl functions, and configParsing*/
/*Some of this stuff could use some re-working*/
//to-do: fix memory leak of new directory tInfo in load

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
#include "libtorrent/lazy_entry.hpp"
#include "libtorrent/magnet_uri.hpp"

/*Passed to libcurl to write HTTP responses to string*/
size_t write_to_string(void *ptr, size_t size, size_t count, void * stream);

/*Write the response torrent file to disk...*/
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*prevent hidden files from being added to torrents*/
bool file_filter(std::string const& f);

/*Does the file exist?*/
bool existingFile (const std::string & name);

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

				//printf("Got the peerID: %s\n", peerID.c_str());

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

				//printf("Reached the network header\n");
				directories = 1;
			}
		}else{
			/*Reading a directory from config*/
			/*Clearly I should be performing a better check here to see if it's directory*/
			if(header[0] == '['){
				DirectoryInfo * newDI = new DirectoryInfo;

				//printf("Reading a directory\n");
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

				if(existingFile(newDI->torrentPath)){
					int ret = load_file(newDI, 40 * 1000000);

					if(ret == -1){
						fprintf(stderr, "file too big, aborting\n");
						return 1;
					}

					if(ret != 0){
						fprintf(stderr, "failed to load file \n");
						return 1;
					}
				}
				//				libtorrent::lazy_entry e;
				//				int pos = -1;
				//				printf("decoding. recursion limit: %d total item count limit: %d\n", depth_limit, item_limit);
				//				ret = libtorrent::lazy_bdecode(&buf[0], &buf[0] + buf.size(), e, ec, &pos, depth_limit, item_limit);

				//				if(ret != 0){
				//					fprintf(stderr, "failed to decode: '%s' at character: %d\n", ec.message().c_str(), pos);
				//					return 1;
				//				}

				//				libtorrent::torrent_info t(e, ec);

				//				if(ec){
				//					fprintf(stderr, "%s\n", ec.message().c_str());
				//					return 1;
				//				}

				//				e.clear();
				//				std::vector<char>().swap(buf);

				//				char ih[41];
				//				libtorrent::to_hex((char const*)&t.info_hash()[0], 20, ih);

				//				printf("info hash strlen: %d info_hash: %s\n", strlen(ih), ih);
				//				printf("strlen: %d to_hex: %s\n", strlen(t.info_hash().to_string().c_str()), libtorrent::to_hex(t.info_hash().to_string()).c_str());
				//				printf("The string: %s\n", t.info_hash().to_string().c_str());

				DI.insert(make_pair(newDI->directoryPath, newDI));
			}
		}

	}

	return 0;
}

/*Pings the read_share tracker url to get a share*/
/*Need to change to send w/ info_hash*/
/*Still works*/
/*200 == update*/
/*204 == no_content (no file on server)*/
/*304 == same_file (no_modification)*/
/*404 == validation error*/
int confInfo::read_share(DirectoryInfo *DI){

	/*Pings the read_share url to get a share*/
	/*Need to change to send w/ info_hash*/
	/*Still works*/

	printf("\nReading share for %s...\n", DI->torrentPath.c_str());
	std::ofstream fout;

	string response = "";
	string s = "";
	s+=trackerDomain;
	s+="read_share/?peer_id=";
	s+=peerID;
	s+="&share_id=";
	s+=DI->share_id;
	s+="&info_hash=";
	s.append(DI->info_hash);

	//printf("\nread_share request string: %s\n", s.c_str());
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	int http_code = 0;

	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, s.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		res = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() GET failed: %s\n", curl_easy_strerror(res));
		}

		//printf("\nThe response: %s\n\n", response.c_str());
		curl_easy_cleanup(curl);
	}

	/*read_share responder*/
	if(http_code == 200){
//	if(response == ""){	
		printf("\n%s not synced w/ tracker; synchronizing\n", DI->torrentPath.c_str());
		fout.open(DI->torrentPath);
		fout << response;
		fout.close();

		if(existingFile(DI->torrentPath)){
			int ret = load_file(DI, 40 * 1000000);

			if(ret == -1){
				fprintf(stderr, "file too big, aborting\n");
				/*Need to handle this error*/
				return -1;
			}

			if(ret != 0){
				fprintf(stderr, "failed to load file \n");
				/*Need to handle this error*/
				return -2;
			}

			printf("    ...success!\n");
		}

	}
	if(http_code == 304){
		printf("\n%s is synchronized.\n", DI->torrentPath.c_str());
	}


	return http_code;
}

/*Update a share on the tracker*/
/*200 == works*/
/*400 == poorly formed request*/
/*404 == validation error*/
int confInfo::update_share(DirectoryInfo *DI){

	printf("\nUpdating share for %s...\n", DI->torrentPath.c_str());

	CURL *curl;
	CURLcode res;
	string response = "";
	curl = curl_easy_init();

	std::string s = trackerDomain;
	s+="update_share/";

	int http_code = 0;

	//printf("Here is s: %s\n", s.c_str());
	//printf("trying string: %s\n", DI->info_hash.c_str());
	if(curl){

		struct curl_httppost *post=NULL;
		struct curl_httppost *last=NULL;

		curl_easy_setopt(curl, CURLOPT_URL, s.c_str());
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "peer_id", CURLFORM_COPYCONTENTS, peerID.c_str(), CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "share_id", CURLFORM_COPYCONTENTS, DI->share_id.c_str(), CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "info_hash", CURLFORM_COPYCONTENTS, DI->info_hash.c_str(), CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "share_file", CURLFORM_FILE, DI->torrentPath.c_str(),CURLFORM_END);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		res = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() POST failed: %s\n", curl_easy_strerror(res));
		}

		printf("\nThe response: %s\n\n", response.c_str());
		curl_formfree(post);
	}

	return http_code;
}

/*Passed to libcurl to write HTTP responses to string */
size_t write_to_string(void *ptr, size_t size, size_t count, void * stream){
	((string *)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

/*This creates a torrent file.. hopefully this is correct.*/
int confInfo::torCreate(DirectoryInfo * DI){

	int flags = 0;

	libtorrent::file_storage fs;

	std::string full_path = libtorrent::complete(DI->directoryPath.c_str());
	add_files(fs, full_path, file_filter, flags);

	if(fs.num_files() == 0){
		fputs("no files specified.\n", stderr);
		return 0;
	}else{
		printf("\nCreating new metainfo...\n");
	}

	libtorrent::create_torrent t(fs);
	std::string trackerDom = trackerDomain;
	trackerDom+="tracker/";
	t.add_tracker(trackerDom.c_str());
	t.set_creator(trackerUsername.c_str());

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

	int ret = load_file(DI, 40 * 1000000);

	if(ret == -1){
		fprintf(stderr, "file too big, aborting\n");
		return 1;
	}

	if(ret != 0){
		fprintf(stderr, "failed to load file \n");
		return 1;
	}


	return 1;

}

/*INCOMPLETE*/
/*Need to finish this; stopped because info_hash isn't set*/
int confInfo::download_torrent(libtorrent::session *s, DirectoryInfo * DI){

	printf("\ndownloading... \n");
	libtorrent::error_code ec;

	libtorrent::add_torrent_params p;
	p.save_path = DI->directoryPath;
	p.ti = new libtorrent::torrent_info(DI->torrentPath.c_str(), ec);
	if(ec){
		fprintf(stderr, "%s\n", ec.message().c_str());
		return -1;
	}

	s->add_torrent(p, ec);
	if(ec){
		fprintf(stderr, "%s\n", ec.message().c_str());
		return -2;
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

/*this loads a torrent file into a passed in vector*/
int confInfo::load_file(DirectoryInfo *DI, int limit = 8000000)
{

	std::vector<char> buf;
	libtorrent::error_code ec;
	ec.clear();
	FILE* f = fopen(DI->torrentPath.c_str(), "rb");

	if (f == NULL)
	{
		ec.assign(errno, boost::system::get_generic_category());
		return -1;
	}

	int r = fseek(f, 0, SEEK_END);
	if (r != 0)
	{
		ec.assign(errno, boost::system::get_generic_category());
		fclose(f);
		return -1;
	}
	long s = ftell(f);
	if (s < 0)
	{
		ec.assign(errno, boost::system::get_generic_category());
		fclose(f);
		return -1;
	}

	if (s > limit)
	{
		fclose(f);
		return -2;
	}

	r = fseek(f, 0, SEEK_SET);
	if (r != 0)
	{
		ec.assign(errno, boost::system::get_generic_category());
		fclose(f);
		return -1;
	}

	buf.resize(s);
	if (s == 0)
	{
		fclose(f);
		return 0;
	}

	r = fread(&buf[0], 1, buf.size(), f);
	if (r < 0)
	{
		ec.assign(errno, boost::system::get_generic_category());
		fclose(f);
		return -1;
	}

	fclose(f);

	if (r != s) return -3;

	int item_limit = 1000000;
	int depth_limit = 1000;

	libtorrent::lazy_entry e;
	int pos = -1;
	int ret = 0;
	//	printf("decoding. recursion limit: %d total item count limit: %d\n", depth_limit, item_limit);
	ret = libtorrent::lazy_bdecode(&buf[0], &buf[0] + buf.size(), e, ec, &pos, depth_limit, item_limit);

	if(ret != 0){
		fprintf(stderr, "failed to decode: '%s' at character: %d\n", ec.message().c_str(), pos);
		return 1;
	}

	//libtorrent::torrent_info t(e, ec);
	DI->tInfo = new libtorrent::torrent_info(e, ec);//t;

	if(ec){
		fprintf(stderr, "%s\n", ec.message().c_str());
		return 1;
	}

	e.clear();
	std::vector<char>().swap(buf);

	char ih[41];
	libtorrent::to_hex((char const*)&(*(DI->tInfo)).info_hash()[0], 20, ih);
	
	CURL *curl = curl_easy_init();

	if(curl){
		
		char * urlEncoded = curl_easy_escape(curl, (*(DI->tInfo)).info_hash().to_string().c_str(),20);
		if(urlEncoded){
			printf("Encoded: %s\n", urlEncoded);
			curl_free(urlEncoded);
		}
		curl_easy_cleanup(curl);
	}
	//printf("info hash strlen: %d info_hash: %s\n", strlen(ih), ih);
	//printf("strlen: %d to_hex: %s\n", strlen(tInfo->info_hash().to_string().c_str()), libtorrent::to_hex(tInfo->info_hash().to_string()).c_str());
	//printf("The string: %s\n", tInfo->info_hash().to_string().c_str());

	std::string hexer(ih);
	DI->info_hash = hexer;

	return 0;
}

/*Does the file exist?*/
bool existingFile (const std::string & name){

	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

/*Write downloaded metainfo to disk*/
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream){
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}
