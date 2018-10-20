#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <algorithm>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <map>

template <typename F>
void onLines(std::istream& stream, F f)
{
    std::string line;
    while (std::getline(stream, line)) {
        f(line);
    }
}

template <typename F>
void onValidLines(std::istream& stream, F f)
{
    onLines(stream, [&](const std::string& line) {
        std::size_t tabPos = line.find('\t');
        if (tabPos == std::string::npos) {
            std::cerr << "invalid line: less than 2 columns" << std::endl;
            return;
        }

        std::size_t nextTabPos = line.find('\t', tabPos+1);
        if (nextTabPos != std::string::npos) {
            std::cerr << "invalid line: more than 2 columns" << std::endl;
            return;
        }

        std::string timestamp = line.substr(0, tabPos); // string copy
        std::string query = line.substr(tabPos+1); // string copy

        if (std::any_of(timestamp.begin(), timestamp.end(), [](char c) { return c < '0' || c > '9'; })) {
            std::cerr << "invalid line: \"" << timestamp << "\" is not a valid timestamp" << std::endl;
            return;
        }

        f(timestamp, query);
    });
}

template <typename F>
void onTimestampRange(std::istream& stream, std::string_view start_timestamp, std::string_view end_timestamp, F f)
{
    onValidLines(stream, [&](const std::string& timestamp, const std::string& query) {
        if (timestamp < start_timestamp || end_timestamp < timestamp)
            return;
        f(query);
    });
}

int main()
{
    typedef std::size_t Count;

    Count n = 10;
    std::string start_timestamp = "0";
    std::string end_timestamp(1, '9' + 1);

    if (n == 0)
        return EXIT_SUCCESS;

    std::unordered_map<std::string, Count> occurences;
    std::multimap<Count, std::string_view> maxOccurences;

    std::ifstream file("hn_logs.tsv");
    onTimestampRange(file, start_timestamp, end_timestamp, [&](const std::string& q) {
        auto insertIt = occurences.emplace(q, 0).first; // string copy
        std::string_view query = insertIt->first;
        Count count = ++insertIt->second;

        // erase an existing entry for this query in `maxOccurences`
        for (auto range = maxOccurences.equal_range(count-1); range.first != range.second; ++range.first) {
            if (range.first->second == query) {
                maxOccurences.erase(range.first);
                break;
            }
        }

        if (maxOccurences.size() == n) {
            Count smallestCount = maxOccurences.begin()->first;
            if (count > smallestCount) {
                maxOccurences.emplace(count, query);

                // only keep the n max occurences
                maxOccurences.erase(smallestCount);
            }
        } else {
            maxOccurences.emplace(count, query);
        }
    });

    for (auto it = maxOccurences.rbegin(), itEnd = maxOccurences.rend(); it != itEnd; ++it) {
        std::cout << it->second << ' ' << it->first << '\n';
    }
    std::cout.flush();

    return EXIT_SUCCESS;
}
