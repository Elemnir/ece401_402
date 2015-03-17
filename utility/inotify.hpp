#ifndef INOTIFY_H
#define INOTIFY_H

#include <sys/inotify.h>
#include <linux/limits.h>
#include <stdint.h>
#include <string>
#include <map>
#include "boost/filesystem.hpp"
#include "directory_tree.hpp"

// Assuming length of filename will not exceed PATH_MAX
#define EVENT_SIZE sizeof(struct inotify_event) + PATH_MAX + 1  
#define BUF_SIZE 16384 * EVENT_SIZE


class Inotify {
    private:
        DirectoryTree directory_tree_;
        // Maps directory names to watch descriptors
        std::map<std::string,int> directory_to_watchd_;
        int inotify_fd_;         
        char event_buffer_[BUF_SIZE];
        static const uint32_t bitmask_ = 
            //IN_ATTRIB |          /* Metadata changed, e.g permissions, timestamps, etc. */
            IN_CREATE |          /* File/directory created in watched directory */
            IN_DELETE |          /* File/directory deleted from watched directory */
            IN_DELETE_SELF |     /* Watched file/directory was itself deleted */  
            IN_MODIFY |          /* File was modified */
            IN_MOVE_SELF |       /* Watched file/directory was itself moved */
            IN_MOVED_FROM |      /* File moved out of watched directory */
            IN_MOVED_TO;         /* File moved into watched directory */

        // Create a watch for the directory and return the watch descriptor assigned to it.
        int CreateWatch(boost::filesystem::path pathname);

        // Process an event from the inotify event queue. Depending on the event,
        // the directory tree will be updated accordingly. For example, if an event
        // signals that a directory has been removed, the directory will also be
        // removed from the directory tree. 
        void ProcessEvent(struct inotify_event *event);

        // Recursively add directories rooted at pathname to list of items to to be watched.
        // Returns the watch descriptor given to the path that is passed in as the argument.
        // Even though watch descriptors are assigned to each sub-directory, they are not
        // returned.
        int AddWatchRecursive(boost::filesystem::path path, int parent_watch_descriptor);

    public:
        Inotify(); 
        ~Inotify();
        void WatchDirectory(std::string pathname);
        // TODO: Provide the functionality to remove a directory from watch
        // void RemoveWatchDirectory(std::string pathname);

        // Get a flag for a watched directory which has the follwing enumerations:
        //  DELETED
        //  MODIFIED
        //  NOCHANGE
        // A watch must first be placed on a directory with a call to WatchDirectory(std::string)
        // before this function can be called. The MODIFIED bit for the directory (and it's 
        // descendants) is reset upon each call. Consequently, the MODIFIED bit being set means
        // that the directory was modified SINCE the last call to GetUpdateFlag.
        enum UPDATE_FLAG GetUpdateFlag(std::string pathname);

        // Read events that are currently waiting in the inotify event queue. Will call
        // ProcessEvent on each event.
        void ReadEvents();
};

#endif
