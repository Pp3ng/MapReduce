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
#include <barrier>
#include <syncstream>
#include <locale>
#include <codecvt>

class WordCounter
{
private:
    // shared mutex for thread-safe access to word counts
    std::shared_mutex mutex;

    // unordered map to store word counts
    std::unordered_map<std::string, int> wordCounts;

    // convert string to lowercase using ranges and locale
    static void toLowerCase(std::wstring &wstr)
    {
        std::ranges::transform(wstr, wstr.begin(), ::towlower);
    }

    // add a word to the word counts map in a thread-safe manner
    void addWord(const std::wstring &wword)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string word = converter.to_bytes(wword);
        std::unique_lock<std::shared_mutex> lock(mutex);
        wordCounts[word]++;
    }

    // process a single file and count words
    void processFile(const std::string &filename)
    {
        try
        {
            // check if file exists
            if (!std::filesystem::exists(filename))
            {
                std::osyncstream(std::cerr) << "File does not exist: " << filename << std::endl;
                return;
            }

            // open file
            std::ifstream file(filename);
            if (!file.is_open())
            {
                std::osyncstream(std::cerr) << "Cannot open file: " << filename << std::endl;
                return;
            }

            // use ranges to read and process lines
            auto lines = std::ranges::istream_view<std::string>(file) | std::ranges::views::filter([](const std::string &line)
                                                                                                   { return !line.empty(); });

            // regex to match words in different languages
            std::wregex wordRegex(LR"(\w+|[^\w\s])");

            // process each line
            for (const auto &line : lines)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::wstring wline = converter.from_bytes(line);

                std::wsregex_iterator words_begin = std::wsregex_iterator(wline.begin(), wline.end(), wordRegex);
                std::wsregex_iterator words_end = std::wsregex_iterator();

                for (std::wsregex_iterator i = words_begin; i != words_end; ++i)
                {
                    std::wstring wword = (*i).str();
                    toLowerCase(wword);
                    if (!wword.empty())
                    {
                        addWord(wword);
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            std::osyncstream(std::cerr) << "Exception processing file "
                                        << filename << ": " << e.what() << std::endl;
        }
    }

public:
    // process multiple files using a barrier
    void processFiles(const std::vector<std::string> &filenames)
    {
        // create a barrier to synchronize threads
        std::size_t threadCount = filenames.size();
        std::barrier sync_point(threadCount, [this]()
                                {
            // optional completion function - could be used for logging or additional processing
            std::osyncstream(std::cout) << "All files processed." << std::endl; });

        // vector to store futures
        std::vector<std::future<void>> futures;

        // launch a thread for each file
        for (const auto &filename : filenames)
        {
            futures.emplace_back(std::async(std::launch::async, [this, &filename, &sync_point]()
                                            {
                processFile(filename);
                // signal completion at the barrier
                sync_point.arrive_and_wait(); }));
        }

        // wait for all threads to complete
        for (auto &future : futures)
        {
            try
            {
                future.get();
            }
            catch (const std::exception &e)
            {
                std::osyncstream(std::cerr) << "Exception waiting for future: "
                                            << e.what() << std::endl;
            }
        }
    }

    // reduce and print word counts
    void printWordCounts()
    {
        std::shared_lock<std::shared_mutex> lock(mutex);

        std::vector<std::pair<std::string, int>> sortedWordCounts(wordCounts.begin(), wordCounts.end());
        std::ranges::sort(sortedWordCounts);

        std::osyncstream(std::cout) << "Word Counts:\n";
        for (const auto &[word, count] : sortedWordCounts)
        {
            std::osyncstream(std::cout) << word << ": " << count << std::endl;
        }
    }
};

auto main(int argc, char *argv[]) -> int
{
    // check if at least one file is provided
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <file1> <file2> ... <fileN>\n";
        return EXIT_FAILURE;
    }

    // create vector of filenames
    std::vector<std::string> filenames(argv + 1, argv + argc);

    try
    {
        // create word counter and process files
        WordCounter counter;
        counter.processFiles(filenames);
        counter.printWordCounts();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}