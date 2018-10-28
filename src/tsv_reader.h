#pragma once

#include <istream>
#include <string>
#include <string_view>
#include <vector>

class TSVReader
{
public:
    TSVReader(std::istream& input);

    bool readNextRow(std::vector<std::string_view>& row);

private:
    std::istream& input_;
    std::string line_;
};
