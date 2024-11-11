#include "logging/Logging.h"
#include "SignalChannel.h"
#include "ArgParser.h"
#include "SafeQueue.h"
#include "Sorter.h"
#include "Version.h"
#include "PollResult.h"
#include "PollerBridge.h"

#include "DirectoryPollerBuilder.h"
#ifdef __linux__
#include "DirectoryPoller.h"
#elif __APPLE__
#include "DirectoryPollerMacOS.h"
#endif

#include <iostream>
#include <vector>
#include <string>
#include <err.h> /* GNU C lib error messages: err */
#include <memory>

#include <stdio.h>
#include <dirent.h>

#include <cstdio>
#include <cstdlib>

#include <atomic>
#include <condition_variable>

#include <signal.h>
#if __APPLE__
#include <sys/event.h> // for kqueue() etc.
#endif

static const std::string name = "main";

class FilePathCompare
{
    // Ascending order sort
    bool operator()(PollResult file_path_a, PollResult file_path_b)
    {
        return file_path_a.get().compare(file_path_b.get()) < 0;
    }
};

void print_usage(const std::string &name)
{
    std::cout << "usage: " << name << " [-h] [-d <directory>]"
              << "\n"
              << "Compare:\n"
              << "  -d    directory to read\n"
              << "Miscellaneous:\n"
              << "  -h    display this help text and exit\n"
              << "Example:\n"
              << "  " << name << " -d ../../benchmark/data\n"
              << std::endl;
}

/**
 * Create a return a shared channel for SIGINT signals.
 *
 */
std::shared_ptr<SignalChannel> listen_for_sigint(sigset_t &sigset)
{
    std::shared_ptr<SignalChannel> sig_channel = std::make_shared<SignalChannel>();

#ifdef __linux__
    // Listen for sigint event line Ctrl^c
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    std::thread signal_handler{
        [&sig_channel, &sigset]()
        {
            int signum = 0;

            // wait untl a signal is delivered
            sigwait(&sigset, &signum);
            sig_channel->m_shutdown_requested.store(true);

            // notify all waiting workers to check their predicate
            sig_channel->m_cv.notify_all();
            std::cout << "Received signal " << signum << "\n";
            return signum;
        }};
    signal_handler.detach();
#elif __APPLE__
    std::thread signal_handler{
        [&sig_channel]()
        {
            int kq = kqueue();

            /* Two kevent structs */
            struct kevent *ke = (struct kevent *)malloc(sizeof(struct kevent));

            /* Initialise struct for SIGINT */
            signal(SIGINT, SIG_IGN);
            EV_SET(ke, SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);

            /* Register for the events */
            if (kevent(kq, ke, 1, NULL, 0, NULL) < 0)
            {
                perror("kevent");
                return false;
            }

            memset(ke, 0x00, sizeof(struct kevent));

            // Camp here for event
            if (kevent(kq, NULL, 0, ke, 1, NULL) < 0)
            {
                perror("kevent");
            }

            switch (ke->filter)
            {
            case EVFILT_SIGNAL:
                std::cout << "Received signal " << strsignal(ke->ident) << "\n";
                sig_channel->m_shutdown_requested.store(true);
                sig_channel->m_cv.notify_all();
                break;
            default:
                break;
            }

            return true;
        }};
    signal_handler.detach();
#endif

    return sig_channel;
}

bool should_exit(std::shared_ptr<SignalChannel> sig_channel)
{
    {
        std::unique_lock shutdown_lock(sig_channel->m_cv_mutex);
        sig_channel->m_cv.wait_for(shutdown_lock, std::chrono::milliseconds(10), [sig_channel = sig_channel]()
                                   { bool should_shutdown = sig_channel->m_shutdown_requested.load();
                                     return should_shutdown; });
    }

    return sig_channel->m_shutdown_requested.load();
}

int main(int argc, char **argv)
{
    /*************************************************************************
     *
     * COMMAND LINE ARGS
     *
     *************************************************************************/
    ArgParser args(argc, argv);
    if (args.has_option("-h"))
    {
        print_usage(argv[0]);
        return 0;
    }

    if (!args.has_option("-d"))
    {
        print_usage(argv[0]);
        return 1;
    }

    std::vector<std::string> directory = args.option("-d");
    if (directory.size() < 1)
    {
        std::cerr << "Please provide a directory for reading files." << std::endl;
        return 1;
    }

    /*************************************************************************
     *
     * SIGINT CHANNEL
     *
     *************************************************************************/
    sigset_t sigset;
    std::shared_ptr<SignalChannel> sig_channel = listen_for_sigint(sigset);

    /*************************************************************************
     *
     * LOGGER
     *
     *************************************************************************/
    std::atomic<size_t> active_processors = 0;
    std::condition_variable log_cv;
    std::mutex log_cv_mutex;
    Logging::LogProcessor log_processor(&active_processors, &log_cv, &log_cv_mutex);
    log_processor.start();

    std::stringstream ss;
    ss << "Started - Git version: "
       << " tag="
       << GIT_TAG
       << ", revision="
       << GIT_REV
       << ", branch="
       << GIT_BRANCH;
    Logging::INFO(ss.str(), "main");

    /*************************************************************************
     *
     * DIRECTORY WATCHER
     *
     *************************************************************************/
    std::shared_ptr<SafeQueue<PollResult>> files_to_sort_queue = std::make_shared<SafeQueue<PollResult>>();
    // std::priority_queue<PollResult, std::vector<PollResult>, FilePathCompare> files_to_sort_queue;

    DirectoryPoller dir_poller = DirectoryPoller::builder("DirectoryPoller")
                                     .with_directory(directory[0])
                                     .with_sig_channel(sig_channel)
                                     .build();
    dir_poller.set_queue(files_to_sort_queue);

    PollerBridge dir_poller_bridge(dir_poller);
    dir_poller_bridge.start();

    /*************************************************************************
     *
     * MAIN LOOP
     *
     *************************************************************************/

    std::vector<std::string> files;
    while (true)
    {
        PollResult r;
        files_to_sort_queue->dequeue_with_timeout(1000, r);
        if (!r.empty())
        {
            std::string file_path = r.get();
            files.emplace_back(file_path);
            if (files.size() == 10)
            {
                Sorter s(sig_channel);
                const auto &sorted_file_path = s.sort(files);
                files.clear();
                const auto resampled_file_path = s.resample_and_write(15, sorted_file_path);
                std::remove(sorted_file_path.c_str());
                std::string new_resampled_file_path = resampled_file_path + ".csv";
                std::rename(resampled_file_path.c_str(), new_resampled_file_path.c_str());
                std::cout << "Resampled to:" << new_resampled_file_path << std::endl;
            }
        }

        if (should_exit(sig_channel))
        {
            break;
        }
    }

    dir_poller_bridge.join();
    log_processor.stop();
    log_processor.join();
    return 0;
}