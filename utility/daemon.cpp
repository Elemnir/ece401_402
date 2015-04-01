#include <string>
#include <vector>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "boost/filesystem.hpp"
#include "inotify.hpp"

namespace fs = boost::filesystem;


/* Create a file descriptor timer and return the file descriptor */
int create_timer(int interval);

int main(int argc, const char* argv[])
{
    Inotify notify;
    std::vector<fs::path> watched_directories;
    int timer_fd, bytes_read, update_interval = 3;
    char buffer[1024];

    // Check that at least one directory is passed as an argument
    if (argc < 2) {
        fprintf(stderr, "usage: directory1 [directory2 ..]\n");
        exit(EXIT_SUCCESS);
    }

    // Watch directories passed in on command line
    for (int i=1; i<argc; i++) {
        fs::path directory(argv[i]);
        fs::path canonical_directory = fs::canonical(directory);
        watched_directories.push_back(canonical_directory);
        notify.AddWatchRecursive(canonical_directory);
        printf("Watching %s\n", canonical_directory.c_str());
    }

    // Create timer
    timer_fd = create_timer(update_interval);

    while (1) {
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
            enum UPDATE_FLAG flag = notify.GetUpdateFlag(w_directory);

            if (flag == MODIFIED) {
                // update torrent
                printf("%s: MODIFIED\n", w_directory.c_str());
            }
            else if (flag == DOESNOTEXIST) {
                // do something
                printf("%s: DOESNOTEXIST\n", w_directory.c_str());
            }
        }
    }
    
    if (close(timer_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

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
