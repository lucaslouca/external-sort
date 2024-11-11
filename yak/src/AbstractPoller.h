#ifndef ABSTRACT_POLLER_H
#define ABSTRACT_POLLER_H

#include "AbstractWorker.h"
#include "PollResult.h"

class AbstractPoller : public AbstractWorker
{
private:
  void step() override;
  virtual PollResult poll() = 0;
  virtual void clean() = 0;

public:
  AbstractPoller(std::string name, std::shared_ptr<SignalChannel> sig_channel);
  virtual ~AbstractPoller(){};
  virtual AbstractPoller *clone() const = 0;
};

#endif
