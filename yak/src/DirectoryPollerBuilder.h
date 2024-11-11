#ifndef DIRECTORY_POLLER_BUILDER_H
#define DIRECTORY_POLLER_BUILDER_H

#ifdef __linux__
#include "DirectoryPoller.h"
#elif __APPLE__
#include "DirectoryPollerMacOS.h"
#endif

#include <string>

class DirectoryPollerBuilder
{
private:
    std::string m_name;
    std::string m_dir_to_watch;
    std::shared_ptr<SignalChannel> m_sig_channel;

public:
    DirectoryPollerBuilder(std::string name);
    DirectoryPollerBuilder &with_directory(std::string p);
    DirectoryPollerBuilder &with_sig_channel(std::shared_ptr<SignalChannel> sc);
    DirectoryPoller build();
};

#endif