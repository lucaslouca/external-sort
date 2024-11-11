#include "DirectoryPollerBuilder.h"
#include "DirectoryPoller.h"

DirectoryPollerBuilder::DirectoryPollerBuilder(std::string name) : m_name(name)
{
}

DirectoryPollerBuilder &DirectoryPollerBuilder::with_directory(std::string p)
{
    m_dir_to_watch = p;
    return *this;
}

DirectoryPollerBuilder &DirectoryPollerBuilder::with_sig_channel(std::shared_ptr<SignalChannel> sc)
{
    m_sig_channel = sc;
    return *this;
}

DirectoryPoller DirectoryPollerBuilder::build()
{

    if (m_dir_to_watch.empty())
    {
        throw std::runtime_error("No directory must be provided");
    }

    if (!m_sig_channel)
    {
        throw std::runtime_error("No signal channel provided");
    }

    DirectoryPoller poller = DirectoryPoller(m_name, m_dir_to_watch, m_sig_channel);

    return poller;
}