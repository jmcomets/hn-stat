#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
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

typedef long long Timestamp;

template <typename F>
void onValidLines(std::istream& stream, F f)
{
    std::istringstream timestamp_input;
    std::string query;
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

        std::string timestamp_str = line.substr(0, tabPos); // string copy
        std::string query = line.substr(tabPos+1); // string copy
        timestamp_input.str(timestamp_str); // string copy
        timestamp_input.clear();

        Timestamp timestamp = -1;
        timestamp_input >> timestamp;
        if (!timestamp_input || timestamp < 0) {
            std::cerr << "invalid line: \"" << timestamp_str << "\" is not a valid timestamp" << std::endl;
            return;
        }

        f(timestamp, query);
    });
}

template <typename F>
void onTimestampRange(std::istream& stream, Timestamp start_timestamp, Timestamp end_timestamp, F f)
{
    onValidLines(stream, [&](Timestamp timestamp, const std::string& query) {
        if (timestamp < start_timestamp || end_timestamp < timestamp)
            return;
        f(query);
    });
}

typedef std::size_t Count;

int main()
{
    Count n = 10;
    Timestamp start_timestamp = 0;
    Timestamp end_timestamp = std::numeric_limits<Timestamp>::max();

    if (n == 0)
        return EXIT_SUCCESS;

    std::unordered_map<std::string, Count> occurences;
    std::multimap<Count, std::string_view, std::greater<Count>> maxOccurences;

    std::ifstream file("hn_logs.tsv");
    onTimestampRange(file, start_timestamp, end_timestamp, [&](const std::string& q) {
        auto insertIt = occurences.emplace(q, 0).first; // string copy
        std::string_view query = insertIt->first;
        Count count = ++insertIt->second;

        // search for an existing entry for this query in `maxOccurences`
        for (auto range = maxOccurences.equal_range(count-1); range.first != range.second; ++range.first) {
            if (range.first->second == query) {
                maxOccurences.erase(range.first);
                break;
            }
        }

        if (maxOccurences.size() == n) {
            auto smallestCount = maxOccurences.rbegin()->first;
            if (count > smallestCount) {
                maxOccurences.emplace(count, query);

                // only keep the n max occurences
                maxOccurences.erase(smallestCount);
            }
        } else {
            maxOccurences.emplace(count, query);
        }
    });

    for (const std::pair<Count, std::string_view>& maxOccuring : maxOccurences) {
        std::cout << maxOccuring.second << ' ' << maxOccuring.first << '\n';
    }
    std::cout.flush();

    return EXIT_SUCCESS;
}
