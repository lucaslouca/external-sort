#ifndef WORKER_H
#define WORKER_H

#include <string>
#include <memory>
#include <thread>
#include <vector>
#include "SafeQueue.h"
#include "SignalChannel.h"
#include "CSV.h"

class Compare
{
public:
    bool is_smaller(const Row &ra, const Row &rb, const std::vector<std::string> &attr, size_t i);

    // Ascending order sort
    bool operator()(std::pair<int, std::string> line_a, std::pair<int, std::string> line_b);
};

class Sorter
{
private:
    std::string m_name;
    std::shared_ptr<SignalChannel> m_sig_channel;

private:
    void write_chunk(std::string file_path, const std::vector<std::string> &lines);
    std::vector<std::string> split_file_in_chunks_of_size(int mb, std::string file_path);
    void merge_sort(const std::vector<std::string> &sorted_chunk_paths, const std::string &result_path);
    std::string external_sort(const std::string &file_path);
    std::string process(const std::string &file_path);
    bool check_exit();

public:
    Sorter(std::shared_ptr<SignalChannel> sig_channel);
    std::string sort(std::vector<std::string> files);
    std::string resample_and_write(size_t minutes, const std::string &file_path);
};

#endif