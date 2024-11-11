#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <vector>
#include <string>

class ArgParser
{
public:
    ArgParser(int &argc, char **argv);
    bool has_option(const std::string &option);
    std::vector<std::string> option(const std::string &option);

private:
    std::vector<std::string> tokens;
};

#endif