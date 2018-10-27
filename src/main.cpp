#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <getopt.h>

#include "options.h"
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

void printUsage(std::ostream& output, const Options& options)
{
    output << "Usage: "
        << "\n\thnStat top nb_top_queries [--from TIMESTAMP] [--to TIMESTAMP] input_file"
        << "\n\thnStat distinct [--from TIMESTAMP] [--to TIMESTAMP] input_file"
        << "\n\n" << options.help()
        << std::endl;
}

int main(int argc, char* argv[])
{
    auto options = Options({
            Option('h', "help", "Display this help"),
            LongOption("from", ArgumentRequired, "Minimum (inclusive) timestamp to consider. Defaults to all timestamps"),
            LongOption("to", ArgumentRequired, "Maximum (inclusive) timestamp to consider. Default to all timestamps.")
            });

    Parser parser(options);
    auto arguments = parser.parse(argc, argv);
    if (!arguments)
        return EXIT_FAILURE;

    if (arguments->hasOption("help")) {
        printUsage(std::cout, options);
        return EXIT_SUCCESS;
    }

    Timestamp start_timestamp = Timestamp::Min;
    if (auto timstamp_str = arguments->getOption("from")) {
        auto timestamp = Timestamp::parse(*timstamp_str);
        if (!timestamp) {
            std::cerr << argv[0] << ": " << "--from received an invalid timestamp" << std::endl;
            return EXIT_FAILURE;
        }
        start_timestamp = *timestamp;
    }

    Timestamp end_timestamp = Timestamp::Max;
    if (auto timstamp_str = arguments->getOption("to")) {
        auto timestamp = Timestamp::parse(*timstamp_str);
        if (!timestamp) {
            std::cerr << argv[0] << ": " << "--to received an invalid timestamp" << std::endl;
            return EXIT_FAILURE;
        }
        end_timestamp = *timestamp;
    }

    if (end_timestamp < start_timestamp) {
        std::cerr << argv[0] << ": " << "--from cannot receive a larger timestamp than the one specified with --to" << std::endl;
        return EXIT_FAILURE;
    }

    auto positionalArguments = arguments->getPositional();

    auto command = positionalArguments.next();
    if (!command) {
        std::cerr << argv[0] << ": " << "no command specified (view usage with -h/--help)" << std::endl;
        return EXIT_FAILURE;
    }

    static const std::string PrintTopNCommand = "top";
    static const std::string PrintDistinctCommand = "distinct";

    if (command == PrintTopNCommand) {
        auto count_str = positionalArguments.next();
        if (!count_str) {
            std::cerr << argv[0] << ": " << "no maximum number of elements given" << std::endl;
            return EXIT_FAILURE;
        }

        int n;
        try {
            n = std::stoi(*count_str);
            if (n < 0)
            {
                std::cerr << argv[0] << ": " << "expected a positive integer, got " << n << std::endl;
                return EXIT_FAILURE;
            }
        } catch (std::invalid_argument) {
            std::cerr << argv[0] << ": " << std::quoted(*count_str) << " is not an integer" << std::endl;
            return EXIT_FAILURE;
        } catch (std::out_of_range) {
            std::cerr << argv[0] << ": " << std::quoted(*count_str) << " is too large" << std::endl;
            return EXIT_FAILURE;
        }

        auto filename = positionalArguments.next();
        if (!filename) {
            std::cerr << argv[0] << ": " << "no filename given" << std::endl;
            return EXIT_FAILURE;
        }

        std::ifstream file(*filename);
        if (!file) {
            std::cerr << argv[0] << ": " << "file " << std::quoted(*filename) << " not readable" << std::endl;
            return EXIT_FAILURE;
        }

        printTopN(file, std::cout, start_timestamp, end_timestamp, n);
    } else if (command == PrintDistinctCommand) {
        auto filename = positionalArguments.next();
        if (!filename) {
            std::cerr << argv[0] << ": " << "no filename given" << std::endl;
            return EXIT_FAILURE;
        }

        std::ifstream file(*filename);
        if (!file) {
            std::cerr << argv[0] << ": " << "file " << std::quoted(*filename) << " not readable" << std::endl;
            return EXIT_FAILURE;
        }

        printDistinctCount(file, std::cout, start_timestamp, end_timestamp);
    } else {
        std::cerr << argv[0] << ": unrecognized command " << std::quoted(*command) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
