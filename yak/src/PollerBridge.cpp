#include "PollerBridge.h"

#include "ThreadGuard.h"

PollerBridge::PollerBridge(const PollerBridge &original)
{
  m_poller_ptr = original.m_poller_ptr->clone();
}

PollerBridge::PollerBridge(const AbstractPoller &inner_poller)
{
  m_poller_ptr = inner_poller.clone();
}

bool PollerBridge::start()
{
  m_t = std::make_unique<std::thread>(&AbstractPoller::run, m_poller_ptr);
  return true;
}

void PollerBridge::join() const { ThreadGuard g(*m_t); }

PollerBridge::~PollerBridge() { delete m_poller_ptr; }
