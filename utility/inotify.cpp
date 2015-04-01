#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/ioctl.h>
#include <queue>
#include "inotify.hpp"


namespace fs = boost::filesystem;


/*******************/
/* Private Methods */
/*******************/

int Inotify::CreateWatch(fs::path directory_path)
{
    int watch_descriptor;

    if (!fs::exists(directory_path)) {
        fprintf(stderr, "File/directory does not exist: %s\n", directory_path.c_str());
        exit(EXIT_FAILURE);
    }

    watch_descriptor = inotify_add_watch(inotify_fd_, directory_path.c_str(), bitmask_);
    if (watch_descriptor == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    return watch_descriptor;
}

/*
void Inotify::RemoveWatch(int watch_descriptor)
{
    int ret;

    ret = inotify_rm_watch(inotify_fd_, watch_descriptor);
    if (ret == -1) {
        perror("inotify_rm_watch");
        exit(EXIT_FAILURE);
    }
}
*/


// TODO: Handle case where watched directory is itself moved
void Inotify::ProcessEvent(struct inotify_event *event)
{
    if (event->mask & IN_Q_OVERFLOW) {
        fprintf(stderr, "Buffer overflow. Events were lost.\n");
        exit(EXIT_FAILURE);
    }

    // File/directory created in watched directory
    // File/directory moved into watched directory
    if ((event->mask & IN_CREATE) || (event->mask & IN_MOVED_TO)) {
        if (event->name[0] != '.') { // Ignore hidden files/directories
            if (event->mask & IN_ISDIR) { // if directory was created
                fs::path directory_path = directory_tree_.GetPath(event->wd) / event->name;
                AddWatchRecursive(directory_path);
            }
            directory_tree_.SetModifyBit(event->wd);

            //printf("(%d) %s: File/directory created or moved into directory\n",event->wd,
            //      directory_tree_.GetPath(event->wd).c_str());
        }
    }

    // File/directory deleted in watched directory
    // File/directory moved from watched directory
    if ((event->mask & IN_DELETE) || (event->mask & IN_MOVED_FROM)) {
        if (event->name[0] != '.') {    // ignore hidden files/directories
            directory_tree_.SetModifyBit(event->wd);
            //printf("(%d) %s: File/directory deleted or moved from directory\n",event->wd,
            //       directory_tree_.GetPath(event->wd).c_str());
        }
    }

    // File modified in watched directory
    if (event->mask & IN_MODIFY) {
        if (event->name[0] != '.') {    // ignore hidden files/directories
            directory_tree_.SetModifyBit(event->wd);
            //printf("(%d) %s: File/directory modified\n",event->wd,
            //      directory_tree_.GetPath(event->wd).c_str());
        }
    }

    
    // Watched directory was itself deleted 
    // IMPORTANT: The watch is automatically removed when a directory is deleted
    if (event->mask & IN_DELETE_SELF) {
        directory_tree_.RemoveNode(event->wd);
    }


    // Watched directory was itself moved
    if (event->mask & IN_MOVE_SELF) {
    }

    // Watch was remove explicitly (inotify_rm_watch) or automatically (file was
    // deleted, or file system was unmounted)
    if (event->mask & IN_IGNORED) {
    }
}


/******************/
/* Public Methods */
/******************/
// TODO: Throw error exception upon failure
Inotify::Inotify() 
{
    inotify_fd_ = inotify_init();
    if (inotify_fd_ == -1) {
        perror("inotify");
        exit(EXIT_FAILURE);
    }
}

// TODO: Throw error exception upon failure
Inotify::~Inotify()
{
    if (close(inotify_fd_) == -1) {
        perror("close(inotify_fd_)");
        exit(EXIT_FAILURE);
    }
}

void Inotify::AddWatchRecursive(fs::path pathname)
{
    int root_watch_descriptor, watch_descriptor;
    std::queue<fs::path> q;
    fs::path parent_path, directory_path;
    fs::directory_iterator end_itr;  // default construction yields past the end

    // Create a watch for the pathname
    root_watch_descriptor = CreateWatch(pathname);
    directory_tree_.AddNode(root_watch_descriptor, pathname);
    q.push(pathname);

    // Add watches for all nested directories recursively 
    while (!q.empty()) {
        parent_path = q.front();
        q.pop();

        for (fs::directory_iterator itr(parent_path); itr != end_itr; ++itr) {
            if (fs::is_directory(itr->status())) {
                directory_path = itr->path();
                watch_descriptor = CreateWatch(directory_path);
                directory_tree_.AddNode(watch_descriptor, directory_path);

                q.push(directory_path);
            }
        }
    }
}

//TODO: Handle exception
enum UPDATE_FLAG Inotify::GetUpdateFlag(boost::filesystem::path directory_path)
{
    enum UPDATE_FLAG flag;

    flag = directory_tree_.GetUpdateFlag(directory_path);
    if (flag != DOESNOTEXIST) {
        directory_tree_.ResetModifyBit(directory_path);
    }

    return flag;
}


void Inotify::ReadEvents()
{
    struct inotify_event *event;
    int bytes_read;
    char *p;
    unsigned int available, total_bytes_read;
    //static unsigned int eventnum = 0;

    ioctl(inotify_fd_, FIONREAD, &available);

    total_bytes_read = 0;
    while (total_bytes_read < available) {
        bytes_read = read(inotify_fd_, &event_buffer_, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_bytes_read += bytes_read;

        // Process each event
        for (p = event_buffer_; p < event_buffer_ + bytes_read; ) {
            event = (struct inotify_event *) p;
            p += sizeof(struct inotify_event) + event->len;
            //printf("Event %d\n", eventnum++);
            ProcessEvent(event);
        }
    }
}
