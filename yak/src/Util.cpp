#include "Util.h"
#include <cstring>

bool Util::str_ends_with(const char *str, const char *suffix)
{
    if (str == NULL || suffix == NULL)
    {
        return false;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
    {
        return false;
    }

    return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}

bool Util::str_starts_with(const char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL)
    {
        return false;
    }

    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);

    if (prefix_len > str_len)
    {
        return false;
    }

    return 0 == strncmp(str, prefix, prefix_len);
}

std::string Util::base_name(const std::string &path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}

std::string Util::remove_extension(const std::string &filename)
{
    typename std::string::size_type const p(filename.find_last_of('.'));
    return p > 0 && p != std::string::npos ? filename.substr(0, p) : filename;
}

std::string Util::what(const std::exception_ptr &eptr = std::current_exception())
{
    if (!eptr)
    {
        throw std::bad_exception();
    }

    try
    {
        std::rethrow_exception(eptr);
    }
    catch (const std::exception &e)
    {
        return e.what();
    }
    catch (const std::string &e)
    {
        return e;
    }
    catch (const char *e)
    {
        return e;
    }
    catch (...)
    {
        return "who knows";
    }
}

std::set<int> Util::split(const std::string &str, char sep)
{
    std::set<int> result;
    std::stringstream ss(str);
    for (int i; ss >> i;)
    {
        result.emplace(i);
        if (ss.peek() == sep)
        {
            ss.ignore();
        }
    }
    return result;
}

unsigned long Util::count_lines(const std::string fname)
{
    unsigned long count = 0;
    std::ifstream file(fname);
    std::string line;
    while (std::getline(file, line))
    {
        ++count;
    }
    return count;
}

bool Util::is_number(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it))
        ++it;
    return !s.empty() && it == s.end();
}