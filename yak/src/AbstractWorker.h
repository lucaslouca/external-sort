#ifndef ABSTRACT_WORKER_H
#define ABSTRACT_WORKER_H

#include "SafeQueue.h"
#include "PollResult.h"
#include "SignalChannel.h"
#include "ThreadGuard.h"
#include <string>
#include <memory>
#include <thread>

class AbstractWorker
{

public:
  AbstractWorker(std::string name, std::shared_ptr<SignalChannel> sig_channel) : m_name(name), m_sig_channel(sig_channel){};
  void set_queue(std::shared_ptr<SafeQueue<PollResult>> queue_);
  void run();
  ~AbstractWorker(){};

protected:
  std::shared_ptr<SafeQueue<PollResult>> m_queue;
  const std::string m_name;

private:
  std::shared_ptr<SignalChannel> m_sig_channel;
  virtual void step() = 0;
  virtual void clean() = 0;
};

#endif
