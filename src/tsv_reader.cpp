#include "tsv_reader.h"

TSVReader::TSVReader(std::istream& input):
    input_(input)
{
}

bool TSVReader::readNextRow(std::vector<std::string_view>& row)
{
    if (!input_ || !std::getline(input_, line_))
        return false;

    row.clear();
    std::string_view line_view(line_);

    std::size_t pos = 0;
    do {
        std::size_t tabPos = line_view.find('\t', pos);
        row.emplace_back(line_view.substr(pos, tabPos));
        pos = tabPos + 1;
    } while (pos != 0); // npos + 1

    return true;
}
