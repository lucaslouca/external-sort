#ifndef POLL_RESULT_H
#define POLL_RESULT_H

#include <string>

class PollResult
{
public:
   PollResult();
   PollResult(std::string result_);
   std::string get() const;
   bool empty() const;
   ~PollResult() {}

private:
   std::string m_result;
};

#endif
