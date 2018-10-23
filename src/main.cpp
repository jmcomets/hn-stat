#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <getopt.h>

template <typename F>
void onLines(std::istream& stream, F f)
{
    std::string line;
    while (std::getline(stream, line)) {
        f(line);
    }
}


class Timestamp
{
    // use string views since we don't need to keep timestamps
    typedef std::string_view Str;

    Timestamp(Str&& ts):
        ts_(ts),
        is_inf_(false)
    {
    }

    struct Infinity {};

    Timestamp(Infinity):
        is_inf_(true)
    {
    }

    friend std::ostream& operator<<(std::ostream&, const Timestamp&);

public:
    static const Timestamp Min;
    static const Timestamp Max;

    static std::optional<Timestamp> parse(const Str& ts)
    {
        if (ts.empty())
            return {};

        // only accept digits, since a timestamp is a positive integer
        if (std::any_of(ts.begin(), ts.end(), [](char c) { return c < '0' || c > '9'; }))
            return {};

        // skip leading zeros
        auto i = ts.find_first_not_of('0');
        if (i == -1)
            i = ts.size() - 1;

        return Timestamp(ts.substr(i));
    }

    bool operator<(const Timestamp& other) const
    {
        // infinity => bool ordering (false < true)
        if (is_inf_ != other.is_inf_)
            return is_inf_ < other.is_inf_;

        // smaller timestamp strings => smaller timestamps
        if (ts_.size() != other.ts_.size())
            return ts_.size() < other.ts_.size();

        // same string size => lexicographical order
        return ts_ < other.ts_;
    }

    bool operator>(const Timestamp& other) const
    { return other < *this; }

private:
    Str ts_;
    bool is_inf_;
};

const Timestamp Timestamp::Min("0");
const Timestamp Timestamp::Max(Timestamp::Infinity {});

std::ostream& operator<<(std::ostream& output, const Timestamp& timestamp)
{
    if (timestamp.is_inf_) {
        output << "infinity";
    } else {
        output << timestamp.ts_;
    }
    return output;
}

template <typename F>
void onValidLines(std::istream& stream, F f)
{
    onLines(stream, [&](std::string_view line) {
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

        std::string_view timestamp_str = line.substr(0, tabPos);
        auto timestamp = Timestamp::parse(timestamp_str);

        if (!timestamp) {
            std::cerr << "invalid line: \"" << timestamp_str << "\" is not a valid timestamp" << std::endl;
            return;
        }

        std::string_view query = line.substr(tabPos+1);

        f(*timestamp, query);
    });
}

template <typename F>
void onTimestampRange(std::istream& stream, const Timestamp& start_timestamp, const Timestamp& end_timestamp, F f)
{
    onValidLines(stream, [&](const Timestamp& timestamp, std::string_view query) {
        if (timestamp < start_timestamp || end_timestamp < timestamp)
            return;
        f(query);
    });
}

typedef std::size_t Count;

void printTopN(std::istream& input, std::ostream& output, Timestamp start_timestamp, Timestamp end_timestamp, Count n)
{
    if (n == 0)
        return;

    std::unordered_map<std::string, Count> occurences;
    std::multimap<Count, std::string_view> maxOccurences;

    onTimestampRange(input, start_timestamp, end_timestamp, [&](std::string_view q) {
        auto insertIt = occurences.emplace(q, 0).first;
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
        output << it->second << ' ' << it->first << '\n';
    }
    output.flush();
}

void printDistinctCount(std::istream& input, std::ostream& output, Timestamp start_timestamp, Timestamp end_timestamp)
{
    std::unordered_set<std::string> queries;
    onTimestampRange(input, start_timestamp, end_timestamp,
                     [&](std::string_view q) { queries.emplace(q); });

    output << queries.size() << std::endl;
}

void printUsage(std::ostream& output)
{
    output << "usage: "
        << "\n\thnStat top nb_top_queries [--from TIMESTAMP] [--to TIMESTAMP] input_file"
        << "\n\thnStat distinct [--from TIMESTAMP] [--to TIMESTAMP] input_file"
        << std::endl;
}

int main(int argc, char* argv[])
{
    std::optional<Timestamp> start_timestamp = Timestamp::Min;
    std::optional<Timestamp> end_timestamp = Timestamp::Max;

    int option_index = 0;

    option long_options[] = {
        { "from", required_argument,  NULL, 'f' },
        { "to",   required_argument,  NULL, 't' },
        { 0, 0, 0, 0}
    };

    const char* optstring = "f:t:";

    int c = -1;
    while (true) {
        c = getopt_long(argc, argv, optstring, long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'f':
            {
                start_timestamp = Timestamp::parse(optarg);
                if (!start_timestamp) {
                    std::cerr << argv[0] << ": " << "--from received an invalid timestamp" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }

            case 't':
            {
                end_timestamp = Timestamp::parse(optarg);
                if (!end_timestamp) {
                    std::cerr << argv[0] << ": " << "--to received an invalid timestamp" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }

            default:
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        std::cerr << argv[0] << ": " << "missing command" << std::endl;
        return EXIT_FAILURE;
    }

    std::string command = argv[optind++];
    if (command == "top") {
        if (optind >= argc) {
            std::cerr << argv[0] << ": " << "missing count" << std::endl;
            return EXIT_FAILURE;
        }

        std::string count_str = argv[optind++];
        Count n;
        try {
            n = std::stoull(count_str); // TODO actually check this f**king integer
        } catch (std::invalid_argument) {
            std::cerr << argv[0] << ": " << "\"" << count_str << "\" is not an integer" << std::endl;
            return EXIT_FAILURE;
        } catch (std::out_of_range) {
            std::cerr << argv[0] << ": " << "\"" << count_str << "\" is too large" << std::endl;
            return EXIT_FAILURE;
        }

        if (optind >= argc) {
            std::cerr << argv[0] << ": " << "missing filename" << std::endl;
            return EXIT_FAILURE;
        }

        std::string filename = argv[optind++];

        std::ifstream file(filename);
        if (!file) {
            std::cerr << argv[0] << ": file " << "\"" << filename << "\"" << " not readable" << std::endl;
            return EXIT_FAILURE;
        }

        printTopN(file, std::cout, *start_timestamp, *end_timestamp, n);
    } else if (command == "distinct") {
        if (optind >= argc) {
            std::cerr << argv[0] << ": " << "missing filename" << std::endl;
            return EXIT_FAILURE;
        }

        std::string filename = argv[optind++];

        std::ifstream file(filename);
        if (!file) {
            std::cerr << argv[0] << ": file " << "\"" << filename << "\"" << " not readable" << std::endl;
            return EXIT_FAILURE;
        }

        printDistinctCount(file, std::cout, *start_timestamp, *end_timestamp);
    } else {
        std::cerr << argv[0] << ": unrecognized command \"" << command << "\"" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

