#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <getopt.h>

#include "ranker.h"
#include "timestamp.h"
#include "tsv_reader.h"

std::string quote(std::string_view str)
{
    std::ostringstream stream;
    stream << "\"" << str << "\"";
    return stream.str();
}

template <typename F>
void onValidLines(std::istream& stream, F f)
{
    TSVReader reader(stream);
    std::vector<std::string_view> row;
    while (reader.readNextRow(row)) {
        if (row.size() != 2) {
            std::cerr << "invalid line: expected 2 columns" << std::endl;
            continue;
        }

        std::string_view timestamp_str = row[0];
        std::string_view query = row[1];

        auto timestamp = Timestamp::parse(timestamp_str);
        if (!timestamp) {
            std::cerr << "invalid line: " << quote(timestamp_str) << " is not a valid timestamp" << std::endl;
            continue;
        }

        f(*timestamp, query);
    }
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
    Timestamp start_timestamp = Timestamp::Min;
    Timestamp end_timestamp = Timestamp::Max;

    const char* optstring = "h";

    option long_options[] = {
        { "from", required_argument,  NULL, 'f' },
        { "to",   required_argument,  NULL, 't' },
        { "help", no_argument,        NULL, 'h' },
        { 0, 0, 0, 0}
    };

    int option_index = 0;
    int c = -1;
    while (true) {
        c = getopt_long(argc, argv, optstring, long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
            {
                printUsage(std::cout);
                return EXIT_SUCCESS;
            }

            case 'f':
            {
                auto timestamp = Timestamp::parse(optarg);
                if (!timestamp) {
                    std::cerr << argv[0] << ": " << "--from received an invalid timestamp" << std::endl;
                    return EXIT_FAILURE;
                }
                start_timestamp = *timestamp;
                break;
            }

            case 't':
            {
                auto timestamp = Timestamp::parse(optarg);
                if (!timestamp) {
                    std::cerr << argv[0] << ": " << "--to received an invalid timestamp" << std::endl;
                    return EXIT_FAILURE;
                }
                end_timestamp = *timestamp;
                break;
            }

            default:
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        std::cerr << argv[0] << ": " << "no command specified (view usage with -h/--help)" << std::endl;
        return EXIT_FAILURE;
    }

    std::string command = argv[optind++];
    if (command == "top") {
        if (optind >= argc) {
            std::cerr << argv[0] << ": " << "no maximum number of elements given" << std::endl;
            return EXIT_FAILURE;
        }

        std::string count_str = argv[optind++];
        int n;
        try {
            n = std::stoi(count_str);
            if (n < 0)
            {
                std::cerr << argv[0] << ": " << "expected a positive integer, got " << n << std::endl;
                return EXIT_FAILURE;
            }
        } catch (std::invalid_argument) {
            std::cerr << argv[0] << ": " << quote(count_str) << " is not an integer" << std::endl;
            return EXIT_FAILURE;
        } catch (std::out_of_range) {
            std::cerr << argv[0] << ": " << quote(count_str) << " is too large" << std::endl;
            return EXIT_FAILURE;
        }

        if (optind >= argc) {
            std::cerr << argv[0] << ": " << "no filename given" << std::endl;
            return EXIT_FAILURE;
        }

        std::string filename = argv[optind++];

        std::ifstream file(filename);
        if (!file) {
            std::cerr << argv[0] << ": file " << quote(filename) << " not readable" << std::endl;
            return EXIT_FAILURE;
        }

        printTopN(file, std::cout, start_timestamp, end_timestamp, n);
    } else if (command == "distinct") {
        if (optind >= argc) {
            std::cerr << argv[0] << ": " << "no filename given" << std::endl;
            return EXIT_FAILURE;
        }

        std::string filename = argv[optind++];

        std::ifstream file(filename);
        if (!file) {
            std::cerr << argv[0] << ": file " << quote(filename) << " not readable" << std::endl;
            return EXIT_FAILURE;
        }

        printDistinctCount(file, std::cout, start_timestamp, end_timestamp);
    } else {
        std::cerr << argv[0] << ": unrecognized command " << quote(command) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

