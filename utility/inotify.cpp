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

int Inotify::AddWatchRecursive(fs::path pathname, int parent_watch_descriptor = -1)
{
    int root_wd, wd, parent_wd;
    std::queue<int> q;
    fs::path path;
    fs::directory_iterator end_itr;  // default construction yields past the end

    // Create a watch for the pathname
    root_wd = CreateWatch(pathname);
    directory_tree_.InsertNode(root_wd, pathname, parent_watch_descriptor);
    q.push(root_wd);

    // Add watches for all nested directories recursively 
    while (!q.empty()) {
        parent_wd = q.front();
        q.pop();

        path = directory_tree_.GetPath(parent_wd);

        for (fs::directory_iterator itr(path); itr != end_itr; ++itr) {
            if (fs::is_directory(itr->status())) {
                wd = CreateWatch(itr->path());
                directory_tree_.InsertNode(wd, itr->path(), parent_wd);

                q.push(wd);
            }
        }
    }

    return root_wd;
}


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
void Inotify::ReadEventsBlocking()
{
    struct inotify_event *event;
    int bytes_read;
    char *p;

    while (1) {
        bytes_read = read(inotify_fd_, &event_buffer_, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        //printf("Bytes Read: %d\n", bytes_read);

        for (p = event_buffer_; p < event_buffer_ + bytes_read; ) {
            event = (struct inotify_event *) p;
            p += sizeof(struct inotify_event) + event->len;
            ProcessEvent(event);
        }
        sleep(1);
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
        if (event->mask & IN_ISDIR) { // if directory was created
            fs::path directory_path = directory_tree_.GetPath(event->wd) / event->name;
            AddWatchRecursive(directory_path, event->wd);
        }
        directory_tree_.SetModifyBit(event->wd);
    }

    // File/directory deleted in watched directory
    // File/directory moved from watched directory
    if ((event->mask & IN_DELETE) || (event->mask & IN_MOVED_FROM)) {
        directory_tree_.SetModifyBit(event->wd);
    }
    
    // Watched file/directory was itself deleted 
    // IMPORTANT: The watch is automatically removed when a directory is deleted
    if (event->mask & IN_DELETE_SELF) {
        directory_tree_.RemoveNode(event->wd);
    }

    // File modified in watched directory
    if (event->mask & IN_MODIFY) {
        directory_tree_.SetModifyBit(event->wd);
    }

    if (event->mask & IN_MOVE_SELF) {
    }

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


void Inotify::WatchDirectory(std::string path)
{
    int watch_descriptor;

    fs::path directory_path(path);
    watch_descriptor = AddWatchRecursive(directory_path, -1);
    directory_to_watchd_[path] = watch_descriptor;
}


//TODO: Handle exception
enum UPDATE_FLAG Inotify::GetUpdateFlag(std::string pathname)
{
    int watch_descriptor;
    enum UPDATE_FLAG flag;

    watch_descriptor = directory_to_watchd_.at(pathname);
    flag = directory_tree_.GetUpdateFlag(watch_descriptor);
    if (flag != DELETED) {
        directory_tree_.ResetModifyBit(watch_descriptor);
    }

    return flag;
}


void Inotify::ReadEvents()
{
    struct inotify_event *event;
    int bytes_read;
    char *p;
    unsigned int available, total_bytes_read;

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
            ProcessEvent(event);
        }
    }
}
