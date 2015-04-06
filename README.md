ece401
======

Project Repository for ECE401

-John; Big notes:

	The daemon should be closed currently with cntl+c. This will signal the read_share thread & wait for it to finish.
	
	1.) Because of the way it's currently written, please have atleast one "/" in your directoryPath.
		I'l fix that code later.

	2.) Daemon currently calls read_share every 60 seconds in a separate thread using monitored directory's share_id
		& the peerID inside of the file at the PeerIDFile location.

	3.) Daemon also adds all directories specified to Maurice's monitoring loop as before; that is integrated into the new daemon.cpp

	4.) Daemon will spin and look for updates, if you change something in a monitored directory it will indeed create a torrent file for you
		at the specified DirectoryTorrent path.

	5.) Utility Makefile is different (to accommodate libtorrent). It expects: (apt-get install) libboost1.49-dev; libtorrent-rasterbar-1.0.4; libcurl 7.41

	Config should look similar to this for my parser to work. Also, it is reading from the daemon w/ the path "../Config.conf":
	
	[daemon]
	PeerIDFile = /home/john/.btfs/id

	[network]
	TrackerPassword = testingpassword
	TrackerUsername = admin
	TrackerDomain = home.elemnir.com:8000/
	DaemonPort = auto

	[watchMe]
	DirectoryPath = /home/john/Desktop/watchMe
	DirectoryTorrent = /home/john/Desktop/watchMe.torrent
	ScanningRate = 60
	share_id = 1


## Team Members

| Name | Email | Roles |
|------|-------|-------|
| Adam Howard | ahoward31@vols.utk.edu | Team Leader, Librarian, Researcher, Writer |
| Matthew Seals | mseals1@vols.utk.edu | Lead Tester, Designer, Reviewer | 
| Jeremy Rogers | jroger44@vols.utk.edu | Solutions Architect, Lead Presenter, Tester, Writer |
| John Reynolds | jreyno40@vols.utk.edu | Designer/Implementer, Tester |
| Maurice Marx | mmarx@vols.utk.edu | Lead Writer, Designer, Researcher |
