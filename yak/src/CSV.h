#ifndef CSV_H
#define CSV_H

#include "CSVDefinitions.h"
#include "CSVIterator.h"

#include <istream>
#include <string>
#include <vector>
#include <map>

enum class CSVState
{
    UnquotedField,
    QuotedField,
    QuotedQuote
};

class CSV
{

private:
    std::vector<std::string> m_header;
    std::vector<Row> m_table;

private:
    void read_csv(std::istream &in);
    void sort_in_memory(const std::vector<std::string> &attr);

public:
    CSV();
    CSV(std::string file_path);
    void load(const std::string &file_path);
    size_t size();
    void resample_in_memory(size_t minutes);
    bool sort_in_memory_and_write(const std::vector<std::string> &attr, const std::string &file_path);
    static std::vector<std::string> read_header(const std::string &file_path);
    static std::vector<std::string> read_row(const std::string &row);
    static Row convert_to_row(const std::string &line, const std::vector<std::string> &columns);

    CSVIterator begin() { return CSVIterator(m_table.begin()); }
    CSVIterator end() { return CSVIterator(m_table.end()); }
};

#endif