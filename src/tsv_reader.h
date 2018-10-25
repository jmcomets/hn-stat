#pragma once

#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class TsvReader
{
public:
    TsvReader(std::istream& input);

    std::optional<std::vector<std::string_view>> readNextRow();

private:
    std::istream& input_;
    std::string line_;
};
