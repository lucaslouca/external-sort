#include <iostream>
#include <fstream>
#include <string>
#include <random>

using namespace std;

int random(int min, int max) // range : [min, max]
{
    static bool first = true;
    if (first)
    {
        srand(time(NULL)); // seeding for the first time only!
        first = false;
    }
    return min + rand() % ((max + 1) - min);
}

std::string generate_random_string(const int min_len, const int max_len)
{
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string tmp;
    std::random_device rd;                                   // obtain a random number generator for hardware
    std::mt19937 gen(rd());                                  // seed the generator
    std::uniform_int_distribution<> distr(min_len, max_len); // define the range
    const int len = distr(gen);                              // generate random string length

    tmp.reserve(len);
    for (int i = 0; i < len; ++i)
    {
        tmp += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return tmp;
}

int main(int argc, char **argv)
{
    size_t total_no_lines = 0;

    const unsigned int MAX_NUMBER_OF_ROWS = 1'000'000;
    std::vector<std::string> columns({"id", "timestamp"});
    ofstream fs;

    const size_t number_of_files = 10; // random(10, 20);
    for (size_t i = 0; i < number_of_files; ++i)
    {
        std::string fname = "benchmark/data/huge_" + std::to_string(i) + ".csv";
        fs.open(fname.c_str());

        for (const auto &c : columns)
        {
            fs << c << ",";
        }
        fs << endl;

        unsigned int rows = random(10, MAX_NUMBER_OF_ROWS / number_of_files);
        for (unsigned int i = 0; i < rows; ++i)
        {
            fs << generate_random_string(3, 5) << "," << std::to_string(random(100, 1000));
            fs << endl;
            ++total_no_lines;
        }
        fs.close();
    }

    std::cout << "Total number of lines excluding header=" << total_no_lines << ". Total number of files: " << number_of_files << std::endl;
    return 0;
}