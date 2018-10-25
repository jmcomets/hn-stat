#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <getopt.h>

#include "ranker.h"
#include "timestamp.h"

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

void printTopN(std::istream& input, std::ostream& output, Timestamp start_timestamp, Timestamp end_timestamp, unsigned int n)
{
    if (n == 0)
        return;

    MaxOccurrenceRanker<std::string, std::string_view> ranker(n);

    // rank queries in the given timestamp range
    onTimestampRange(input, start_timestamp, end_timestamp, [&ranker](std::string_view query) {
        ranker.update(query);
    });

    // print out the top n elements
    ranker.visit([&output](std::string_view query, unsigned int count) {
        output << query << ' ' << count << '\n';
    });
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
        unsigned int n;
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

