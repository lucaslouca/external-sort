#include "Sorter.h"
#include "ThreadGuard.h"
#include "Util.h"
#include "logging/Logging.h"
#include <iostream>
#include <sstream>
#include <future>
#include <algorithm> // std::min_element
#include <iterator>  // std::begin, std::end
#include <unistd.h>  // getpid()
#include <signal.h>  // kill()

const size_t MAX_CHUNK_SIZE_MB = 10;
const size_t NUM_THREADS = 4;
std::vector<std::string> CSV_COLUMNS;
const std::vector<std::string> COLUMNS_TO_SORT({"id", "timestamp"});

bool Compare::is_smaller(const Row &ra, const Row &rb, const std::vector<std::string> &attr, size_t i)
{
    if (i == attr.size())
    {
        return false;
    }

    if (ra.size() == 0)
    {
        return true; // Consider an empty row to be "smaller"
                     // than any non-empty vector.
    }

    if (rb.size() == 0)
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

// Ascending order sort
bool Compare::operator()(std::pair<int, std::string> line_a, std::pair<int, std::string> line_b)
{
    Row ra = CSV::convert_to_row(line_a.second, CSV_COLUMNS);
    Row rb = CSV::convert_to_row(line_b.second, CSV_COLUMNS);
    bool rs = is_smaller(ra, rb, COLUMNS_TO_SORT, 0);

    /*
    Because the priority queue outputs largest elements first,
    the elements that "come before" are actually output last.
    */
    return !rs;
}

Sorter::Sorter(std::shared_ptr<SignalChannel> sig_channel) : m_sig_channel(sig_channel)
{
    m_name = "Sorter";
}

std::string Sorter::process(const std::string &file_path)
{
    std::string sorted_chunk_path = file_path + "_s";

    std::stringstream ss;
    ss << "Sorting chunk file '"
       << file_path
       << "' and writing to '"
       << sorted_chunk_path
       << "'"
       << std::endl;
    Logging::INFO(ss.str(), m_name);

    CSV csv(file_path);

    csv.sort_in_memory_and_write(COLUMNS_TO_SORT, sorted_chunk_path);
    return sorted_chunk_path;
}

/**
 * Writes lines into a .txt file and returns the path to the newly created file. The file gets deleted on exit.
 *
 */
void Sorter::write_chunk(std::string file_path, const std::vector<std::string> &lines)
{
    Logging::INFO("Creating tmp file '" + file_path + "'. Writing " + std::to_string(lines.size()) + " lines", m_name);

    std::ofstream file(file_path);

    if (file.is_open())
    {
        for (const auto &line : lines)
        {
            file << line << std::endl;
        }
        file.close();
    }
    else
    {
        Logging::ERROR("Unable to open file", m_name);
    }
}

/**
 * Reads the file in chunks of size mb megabytes, sort each chunk and writes it to a temp file.
 *
 */
std::vector<std::string> Sorter::split_file_in_chunks_of_size(int mb, std::string file_path)
{
    Logging::INFO("Spliting file '" + file_path + "' in " + std::to_string(mb) + "mb chunks", m_name);

    std::vector<std::string> paths;
    int chunk_size_in_bytes = mb * 1024 * 1024;
    int current_bytes_read = 0;
    size_t file_count = 0;
    std::string header;

    std::ifstream in(file_path);
    std::string line;
    std::vector<std::string> lines;
    while (!in.eof())
    {
        std::getline(in, line);

        if (in.bad() || in.fail())
        {
            break;
        }

        current_bytes_read += line.size();

        if (header.empty())
        {
            header.assign(line);
        }
        else
        {
            lines.emplace_back(line);
        }

        if (current_bytes_read >= chunk_size_in_bytes)
        {
            lines.insert(lines.begin(), header);
            std::string path = file_path + "_" + std::to_string(file_count++) + "_tmp";
            write_chunk(path, lines);
            paths.push_back(path);
            current_bytes_read = 0;
            lines.clear();
        }
    }

    if (!lines.empty())
    {
        lines.insert(lines.begin(), header);
        std::string path = file_path + "_" + std::to_string(file_count++) + "_tmp";
        write_chunk(path, lines);
        paths.push_back(path);
        current_bytes_read = 0;
        lines.clear();
    }

    return paths;
}

void Sorter::merge_sort(const std::vector<std::string> &sorted_chunk_paths, const std::string &result_path)
{
    Logging::INFO("Merge sorting " + std::to_string(sorted_chunk_paths.size()) + " chunks to '" + result_path + "'", m_name);
    std::vector<std::shared_ptr<std::ifstream>> streams;
    for (const auto &file_path : sorted_chunk_paths)
    {
        std::shared_ptr<std::ifstream> in = std::make_shared<std::ifstream>(file_path);
        streams.push_back(in);
    }

    // Skip header first so we don't mess up our sort
    for (size_t i = 0; i < streams.size(); ++i)
    {
        std::shared_ptr<std::ifstream> in = streams[i];
        std::string line;
        std::getline(*in, line);
    }

    std::priority_queue<std::pair<int, std::string>, std::vector<std::pair<int, std::string>>, Compare> min_heap;
    for (size_t i = 0; i < streams.size(); ++i)
    {
        std::shared_ptr<std::ifstream> in = streams[i];

        std::string line;
        std::getline(*in, line);

        if (!in->bad() && !in->fail())
        {
            min_heap.push(std::pair<int, std::string>(i, line));
        }
    }

    std::ofstream file(result_path);

    // Write header
    std::string sep = "";
    for (const auto &c : COLUMNS_TO_SORT)
    {
        file << sep << c;
        sep.assign(",");
    }
    file << std::endl;
    flush(file);

    while (min_heap.size() > 0)
    {
        std::pair<int, std::string> min_pair = min_heap.top();
        min_heap.pop();
        if (file.is_open())
        {
            file << min_pair.second << std::endl;
        }
        flush(file);

        std::string next_line;
        std::getline(*streams[min_pair.first], next_line);
        if (!streams[min_pair.first]->bad() && !streams[min_pair.first]->fail())
        {
            min_heap.push(std::pair<int, std::string>(min_pair.first, next_line));
        }

        // if (check_exit())
        // {
        //     break;
        // }
    }

    // clean up
    for (int i = 0; i < streams.size(); i++)
    {
        streams[i]->close();
        std::remove(sorted_chunk_paths[i].c_str());
    }
    file.close();
}

std::string Sorter::external_sort(const std::string &file_path)
{
    /*
    1. Put all paths into queue
    2. Start n fixed number of worker threads
    3. Sort files in parallel
    */

    Logging::INFO("External sorting file '" + file_path + "'", m_name);
    auto tmp_file_paths = split_file_in_chunks_of_size(MAX_CHUNK_SIZE_MB, file_path);
    Logging::INFO("Split into " + std::to_string(tmp_file_paths.size()) + " chunks", m_name);

    std::vector<std::future<std::string>> sort_results;
    for (const auto &chunk_path : tmp_file_paths)
    {
        std::future<std::string> chunk_fut = std::async(std::launch::async,
                                                        [chunk_path, this]()
                                                        {
                                                            return this->process(chunk_path);
                                                        });
        sort_results.push_back(std::move(chunk_fut));
    }

    // Wait for result
    std::vector<std::string> sorted_chunk_file_paths;
    for (auto &f : sort_results)
    {
        sorted_chunk_file_paths.emplace_back(f.get());
    }

    std::string result_path = file_path + "_s";
    if (sorted_chunk_file_paths.size() == tmp_file_paths.size())
    {
        merge_sort(sorted_chunk_file_paths, result_path);
    }

    // Clean up tmp files
    for (int i = 0; i < tmp_file_paths.size(); i++)
    {
        std::remove(tmp_file_paths[i].c_str());
    }

    return result_path;
}

std::string Sorter::sort(std::vector<std::string> files)
{

    std::vector<std::string> sorted_files;
    for (const auto &f : files)
    {
        if (CSV_COLUMNS.empty())
        {
            CSV_COLUMNS = CSV::read_header(f);
        }
        auto f_sorted = external_sort(f);
        sorted_files.emplace_back(f_sorted);
    }

    std::string min = *std::min_element(std::begin(files), std::end(files));
    std::string max = *std::max_element(std::begin(files), std::end(files));

    std::string result_path = Util::remove_extension(min) + "-" + Util::base_name(max);
    merge_sort(sorted_files, result_path);
    return result_path;
}

/**
 * @brief Assumes file is already sorted
 *
 * @param minutes
 * @param file_path
 * @return true
 * @return false
 */
std::string Sorter::resample_and_write(size_t minutes, const std::string &file_path)
{
    std::string result_path = file_path + "_r";
    std::ifstream in(file_path);
    std::ofstream out(result_path);
    std::string line;

    // Skip header
    std::getline(in, line);

    // Write header
    out << line << std::endl;
    flush(out);

    // Attempt to read first line
    std::getline(in, line);
    if (!in.bad() && !in.fail())
    {
        out << line << std::endl;

        Row row = CSV::convert_to_row(line, COLUMNS_TO_SORT);

        std::string current_id = row.at("id");
        long current_timestamp = stol(row.at("timestamp"));

        while (!in.eof())
        {
            std::getline(in, line);

            if (!in.bad() && !in.fail())
            {
                Row row = CSV::convert_to_row(line, COLUMNS_TO_SORT);

                std::string id = row.at("id");
                long timestamp = stol(row.at("timestamp"));
                if (id.compare(current_id) != 0)
                {
                    // New ID
                    out << line << std::endl;
                    flush(out);
                    current_id = id;
                    current_timestamp = timestamp;
                }
                else
                {
                    if (timestamp > current_timestamp + minutes)
                    {
                        out << line << std::endl;
                        flush(out);
                        current_id = id;
                        current_timestamp = timestamp;
                    }
                }
            }
        }
    }

    return result_path;
}

bool Sorter::check_exit()
{
    {
        std::unique_lock shutdown_lock(m_sig_channel->m_cv_mutex);
        m_sig_channel->m_cv.wait_for(shutdown_lock, std::chrono::milliseconds(10), [this]()
                                     { bool should_shutdown = m_sig_channel->m_shutdown_requested.load();
                                     return should_shutdown; });
    }

    return m_sig_channel->m_shutdown_requested.load();
}
