/*I would pragma, but I was getting a warning. Weird.*/
//#pragma once
#include <string>
#include <vector>
#include <map>
#include "libtorrent/session.hpp"
using namespace std;

/*Class for DirectoryInfo*/
class DirectoryInfo{

	public:
		string directoryName;
		string torrentPath;
		string directoryPath;
		int scanRate;
		string share_id;
		string info_hash;
};

/*This is instantiated & filled while parsing configuration file*/
/*Methods operating on the confInfo or torrents/directories monitored will be called from this class*/
class confInfo{

	public:
		
		int DaemonPort;

		string peerID;
		string PeerIDFile;
		string trackerDomain;
		string trackerUsername;
		string trackerPassword;

		libtorrent::torrent_info *tInfo;

		map<string, DirectoryInfo *> DI;

		/*Parse the config file; need to change to accept a path*/
		int configParse(std::ifstream * fin);
		
		/*Create a metainfo file*/
		int torCreate(DirectoryInfo *DI);

		/*Pings the tracker read_share URL*/
		string read_share(DirectoryInfo *DI);

		/*Pings the tracker update_share URL*/
		string update_share(DirectoryInfo *DI);

		/*Starts a download given a session*/
		int download_torrent(libtorrent::session *s, DirectoryInfo * DI);

		/*load_file and store info_hash in the class*/
		int load_file(DirectoryInfo *DI, int limit);
};
