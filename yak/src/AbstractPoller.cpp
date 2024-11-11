#include "AbstractPoller.h"

AbstractPoller::AbstractPoller(std::string name, std::shared_ptr<SignalChannel> sig_channel) : AbstractWorker(name, sig_channel)
{
}

void AbstractPoller::step()
{
  PollResult r = poll();
  if (!r.empty())
  {
    m_queue->enqueue(r);
  }
}
