/*I would pragma, but I was getting a warning. Weird.*/
//#pragma once
#include <string>
#include <vector>
#include <map>
using namespace std;

/*Class for DirectoryInfo*/
class DirectoryInfo{

	public:
		string directoryName;
		string torrentPath;
		string directoryPath;
		int scanRate;
		string share_id;
};

/*This is instantiated & filled while parsing configuration file*/
class confInfo{

	public:
		
		int DaemonPort;

		string peerID;
		string PeerIDFile;
		string trackerDomain;
		string trackerUsername;
		string trackerPassword;

		map<string, DirectoryInfo *> DI;

};
