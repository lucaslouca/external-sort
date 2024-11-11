#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <exception>
#include <stdexcept>
#include <set>
#include <sstream>
#include <iterator>
#include <fstream>
#include <map>

namespace Util
{
    bool str_starts_with(const char *str, const char *prefix);
    bool str_ends_with(const char *str, const char *suffix);
    std::string base_name(std::string const &path);
    std::string remove_extension(const std::string &filename);
    std::string what(const std::exception_ptr &eptr);
    std::set<int> split(const std::string &str, char sep);
    unsigned long count_lines(const std::string fname);
    bool is_number(const std::string &s);

    template <typename T>
    inline std::string to_string(const std::set<T> &s)
    {
        std::stringstream result;
        std::copy(s.begin(), s.end(), std::ostream_iterator<T>(result, " "));
        return result.str();
    }

    template <typename TargetType>
    inline TargetType convert(const std::string &value)
    {
        TargetType converted;
        std::istringstream stream(value);
        stream >> converted;
        return converted;
    }

    template <typename K, typename V>
    inline std::vector<K> keys(std::map<K, V> m)
    {
        std::vector<K> keys;
        for (const auto &[k, v] : m)
        {
            keys.push_back(k);
        }

        return keys;
    }
}

#endif
