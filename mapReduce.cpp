#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <shared_mutex>
#include <future>
#include <cctype>
#include <unordered_map>
#include <filesystem>
#include <exception>
#include <mutex>
#include <regex>
#include <ranges>

// global shared mutex for thread-safe access to wordCounts
std::shared_mutex mutex;
// global unordered map to store word counts
std::unordered_map<std::string, int> wordCounts;

// function to convert a string to lowercase
void toLowerCase(std::string &str)
{
    std::ranges::transform(str, str.begin(), ::tolower);
}

// function to add a word to the wordCounts map in a thread-safe manner
void addWord(const std::string &word)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    wordCounts[word]++;
}

// function to process a file and count the words
void mapFile(const std::string &filename)
{
    try
    {
        // check if the file exists
        if (!std::filesystem::exists(filename))
        {
            std::cerr << "file does not exist: " << filename << std::endl;
            return;
        }

        // open the file
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "cannot open file: " << filename << std::endl;
            return;
        }

        std::string line;
        // regex to match words
        std::regex wordRegex(R"(\w+|[^\w\s])");
        // read the file line by line
        while (std::getline(file, line))
        {
            // find all words in the line
            auto words_begin = std::sregex_iterator(line.begin(), line.end(), wordRegex);
            auto words_end = std::sregex_iterator();

            // process each word
            for (std::sregex_iterator i = words_begin; i != words_end; ++i)
            {
                std::string word = (*i).str();
                toLowerCase(word); // convert word to lowercase
                if (!word.empty())
                {
                    addWord(word); // add word to the map
                }
            }
        }
        file.close(); // close the file
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception in thread processing file " << filename << ": " << e.what() << std::endl;
    }
}

// function to reduce the word counts and print them
void reduce()
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    // copy wordCounts to a vector and sort it
    std::vector<std::pair<std::string, int>> sortedWordCounts(wordCounts.begin(), wordCounts.end());
    std::ranges::sort(sortedWordCounts);

    // print the word counts
    std::cout << "word counts:\n";
    for (const auto &entry : sortedWordCounts)
    {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }
}

int main(int argc, char *argv[])
{
    // check if at least one file is provided
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <file1> <file2> ... <fileN>\n";
        return EXIT_FAILURE;
    }

    int numFiles = argc - 1;
    std::vector<std::future<void>> futures;

    // launch a thread for each file to process it
    for (int i = 0; i < numFiles; ++i)
    {
        futures.emplace_back(std::async(std::launch::async, mapFile, argv[i + 1]));
    }

    // wait for all threads to finish
    for (auto &f : futures)
    {
        try
        {
            f.get();
        }
        catch (const std::exception &e)
        {
            std::cerr << "exception while waiting for future: " << e.what() << std::endl;
        }
    }

    // reduce the word counts and print them
    reduce();

    return EXIT_SUCCESS;
}