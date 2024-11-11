#include "CSV.h"
#include "Util.h"
#include <fstream>
#include <algorithm>

CSV::CSV()
{
}

CSV::CSV(std::string file_path)
{
    load(file_path);
}

std::vector<std::string> CSV::read_row(const std::string &row)
{
    CSVState state = CSVState::UnquotedField;
    std::vector<std::string> fields{""};
    size_t i = 0; // index of the current field
    for (char c : row)
    {
        switch (state)
        {
        case CSVState::UnquotedField:
            switch (c)
            {
            case ',': // end of field
                fields.push_back("");
                i++;
                break;
            case '"':
                state = CSVState::QuotedField;
                break;
            default:
                fields[i].push_back(c);
                break;
            }
            break;
        case CSVState::QuotedField:
            switch (c)
            {
            case '"':
                state = CSVState::QuotedQuote;
                break;
            default:
                fields[i].push_back(c);
                break;
            }
            break;
        case CSVState::QuotedQuote:
            switch (c)
            {
            case ',': // , after closing quote
                fields.push_back("");
                i++;
                state = CSVState::UnquotedField;
                break;
            case '"': // "" -> "
                fields[i].push_back('"');
                state = CSVState::QuotedField;
                break;
            default: // end of quote
                state = CSVState::UnquotedField;
                break;
            }
            break;
        }
    }
    return fields;
}

void CSV::load(const std::string &file_path)
{
    std::ifstream file(file_path);
    read_csv(file);
    file.close();
}

std::vector<std::string> CSV::read_header(const std::string &file_path)
{
    std::ifstream in(file_path);
    std::vector<std::string> header;
    if (!in.eof())
    {
        std::string line;
        std::getline(in, line);
        header = CSV::read_row(line);
    }
    in.close();
    return header;
}

Row CSV::convert_to_row(const std::string &line, const std::vector<std::string> &columns)
{
    auto fields = CSV::read_row(line);
    Row row;
    for (size_t i = 0; i < fields.size(); ++i)
    {
        row[columns[i]] = fields[i];
    }

    return row;
}

void CSV::read_csv(std::istream &in)
{
    bool first_line = true;
    std::string line;
    while (!in.eof())
    {
        std::getline(in, line);
        if (in.bad() || in.fail())
        {
            break;
        }

        if (first_line)
        {
            m_header = CSV::read_row(line);
            first_line = false;
        }
        else
        {
            Row row = CSV::convert_to_row(line, m_header);
            m_table.push_back(row);
        }
    }
}

bool is_smaller(const Row &ra, const Row &rb, const std::vector<std::string> &attr, size_t i)
{
    if (i == attr.size())
    {
        return false;
    }

    if (ra.size() == 0 || ra.find(attr[i]) == ra.end())
    {
        return true; // Consider an empty row to be "smaller"
                     // than any non-empty vector.
    }

    if (rb.size() == 0 || rb.find(attr[i]) == rb.end())
    {
        return false; // If right side is empty, left can only be equal or larger
    }

    bool equal;
    if (Util::is_number(ra.at(attr[i])) && Util::is_number(rb.at(attr[i])))
    {
        equal = stod(ra.at(attr[i])) == stod(rb.at(attr[i]));
    }
    else
    {
        equal = ra.at(attr[i]).compare(rb.at(attr[i])) == 0;
    }

    if (equal)
    {
        return is_smaller(ra, rb, attr, i + 1);
    }
    else
    {
        if (Util::is_number(ra.at(attr[i])) && Util::is_number(rb.at(attr[i])))
        {
            return stod(ra.at(attr[i])) < stod(rb.at(attr[i]));
        }
        else
        {
            return ra.at(attr[i]).compare(rb.at(attr[i])) < 0;
        }
    }
}

void CSV::sort_in_memory(const std::vector<std::string> &attr)
{
    std::sort(m_table.begin(), m_table.end(),
              [&attr](const Row &ra, const Row &rb) -> bool
              {
                  return is_smaller(ra, rb, attr, 0);
              });
}

bool CSV::sort_in_memory_and_write(const std::vector<std::string> &attr, const std::string &file_path)
{
    sort_in_memory(attr);

    std::string sep = "";
    std::ofstream file(file_path);

    if (file.is_open())
    {
        // Write header
        for (const auto &c : m_header)
        {
            file << sep << c;
            sep.assign(",");
        }
        file << std::endl;

        // Write rows
        for (const auto &row : m_table)
        {
            sep.assign("");
            for (const auto &c : m_header)
            {
                file << sep << "\"" << row.at(c) << "\"";
                sep.assign(",");
            }
            file << std::endl;
        }

        file.close();
    }
    else
    {
        std::cerr << "Unable to open file '" << file_path << "'";
        return false;
    }
    return true;
}

void CSV::resample_in_memory(size_t minutes)
{
    sort_in_memory({"id", "timestamp"});
    std::vector<Row> table;

    std::string current_id = m_table[0].at("id");
    long current_timestamp = stol(m_table[0].at("timestamp"));
    table.push_back(m_table[0]);

    for (const auto &row : m_table)
    {
        std::string id = row.at("id");
        long timestamp = stol(row.at("timestamp"));
        if (id.compare(current_id) != 0)
        {
            // New ID
            table.push_back(row);
            current_id = id;
            current_timestamp = timestamp;
        }
        else
        {
            if (timestamp > current_timestamp + minutes)
            {
                table.push_back(row);
                current_id = id;
                current_timestamp = timestamp;
            }
        }
    }

    m_table = std::move(table);
}

size_t CSV::size()
{
    return m_table.size();
}