#include "CSVIterator.h"

CSVIterator::CSVIterator(std::vector<Row>::iterator it) : m_it(it)
{
}

// Pre Increment
CSVIterator &CSVIterator::operator++()
{
    ++m_it;
    return *this;
}

// Post Increment
CSVIterator CSVIterator::operator++(int)
{
    CSVIterator tmp(*this);
    ++(*this);
    return tmp;
}

const Row &CSVIterator::operator*() const
{
    return *m_it;
}

const Row *CSVIterator::operator->() const
{
    return &(*m_it);
}

bool CSVIterator::operator==(CSVIterator const &rhs) const
{
    return ((this == &rhs) || (this->m_it == rhs.m_it));
}

bool CSVIterator::operator!=(CSVIterator const &rhs) const
{
    return !((*this) == rhs);
}
