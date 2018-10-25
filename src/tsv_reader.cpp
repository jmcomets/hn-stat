#include "tsv_reader.h"

TsvReader::TsvReader(std::istream& input):
    input_(input)
{
}

std::optional<std::vector<std::string_view>> TsvReader::readNextRow()
{
    if (!input_ || !std::getline(input_, line_))
        return {};

    std::string_view line_view(line_);
    std::vector<std::string_view> row;

    std::size_t pos = 0;
    do {
        std::size_t tabPos = line_view.find('\t', pos);
        row.emplace_back(line_view.substr(pos, tabPos));
        pos = tabPos + 1;
    } while (pos != 0); // npos + 1

    return row;
}
