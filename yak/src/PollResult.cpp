#include "PollResult.h"

PollResult::PollResult()
{
}

PollResult::PollResult(std::string result) : m_result(result) {}

std::string PollResult::get() const
{
    return m_result;
}

bool PollResult::empty() const
{
    return this->get().empty();
}