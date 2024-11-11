#include "ArgParser.h"

ArgParser::ArgParser(int &argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        tokens.emplace_back(std::string(argv[i]));
    }
}

bool ArgParser::has_option(const std::string &option)
{
    std::vector<std::string>::const_iterator it = std::find(tokens.begin(), tokens.end(), option);
    return it != tokens.end();
}

std::vector<std::string> ArgParser::option(const std::string &option)
{
    std::vector<std::string> result;
    std::vector<std::string>::iterator it = std::find(tokens.begin(), tokens.end(), option);
    while (it != tokens.end() && ++it != tokens.end())
    {
        result.emplace_back(*it);
        it = std::find(it, tokens.end(), option);
    }
    return result;
}