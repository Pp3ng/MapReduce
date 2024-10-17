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

// Global shared mutex for thread-safe access to wordCounts
std::shared_mutex mutex;
// Global unordered map to store word counts
std::unordered_map<std::string, int> wordCounts;

// Function to convert a string to lowercase
void toLowerCase(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

// Function to add a word to the wordCounts map in a thread-safe manner
void addWord(const std::string &word)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    wordCounts[word]++;
}

// Function to process a file and count the words
void mapFile(const std::string &filename)
{
    try
    {
        // Check if the file exists
        if (!std::filesystem::exists(filename))
        {
            std::cerr << "File does not exist: " << filename << std::endl;
            return;
        }

        // Open the file
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return;
        }

        std::string line;
        // Regex to match words
        std::regex wordRegex(R"(\w+|[^\w\s])");
        // Read the file line by line
        while (std::getline(file, line))
        {
            // Find all words in the line
            auto words_begin = std::sregex_iterator(line.begin(), line.end(), wordRegex);
            auto words_end = std::sregex_iterator();

            // Process each word
            for (std::sregex_iterator i = words_begin; i != words_end; ++i)
            {
                std::string word = (*i).str();
                toLowerCase(word); // Convert word to lowercase
                if (!word.empty())
                {
                    addWord(word); // Add word to the map
                }
            }
        }
        file.close(); // Close the file
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in thread processing file " << filename << ": " << e.what() << std::endl;
    }
}

// Function to reduce the word counts and print them
void reduce()
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    // Copy wordCounts to a vector and sort it
    std::vector<std::pair<std::string, int>> sortedWordCounts(wordCounts.begin(), wordCounts.end());
    std::sort(sortedWordCounts.begin(), sortedWordCounts.end());

    // Print the word counts
    std::cout << "Word counts:\n";
    for (const auto &entry : sortedWordCounts)
    {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }
}

int main(int argc, char *argv[])
{
    // Check if at least one file is provided
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <file1> <file2> ... <fileN>\n";
        return 1;
    }

    int numFiles = argc - 1;
    std::vector<std::future<void>> futures;

    // Launch a thread for each file to process it
    for (int i = 0; i < numFiles; ++i)
    {
        futures.emplace_back(std::async(std::launch::async, mapFile, argv[i + 1]));
    }

    // Wait for all threads to finish
    for (auto &f : futures)
    {
        try
        {
            f.get();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception while waiting for future: " << e.what() << std::endl;
        }
    }

    // Reduce the word counts and print them
    reduce();

    return 0;
}