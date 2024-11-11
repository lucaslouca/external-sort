#ifndef CSV_ITERATOR_H
#define CSV_ITERATOR_H

#include "CSVDefinitions.h"

#include <iostream>
#include <vector>

class CSVIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Row;
    using pointer = Row *;
    using reference = Row &; // or also value_type&

    CSVIterator(std::vector<Row>::iterator it);

    // Pre Increment
    CSVIterator &operator++();

    // Post Increment
    CSVIterator operator++(int);

    Row const &operator*() const;
    Row const *operator->() const;

    bool operator==(CSVIterator const &rhs) const;
    bool operator!=(CSVIterator const &rhs) const;

private:
    std::vector<Row>::iterator m_it;
};

#endif