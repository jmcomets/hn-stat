#include "timestamp.h"

const Timestamp Timestamp::Min("0");
const Timestamp Timestamp::Max(Timestamp::Infinity {});

Timestamp::Timestamp(Str&& timestamp_str):
    timestamp_str_(timestamp_str),
    is_inf_(false)
{
}

Timestamp::Timestamp(Infinity):
    is_inf_(true)
{
}

std::optional<Timestamp> Timestamp::parse(const Str& timestamp_str)
{
    if (timestamp_str.empty())
        return {};

    // only accept digitimestamp_str, since a timestamp is a positive integer
    if (std::any_of(timestamp_str.begin(), timestamp_str.end(), [](char c) { return c < '0' || c > '9'; }))
        return {};

    // skip leading zeros
    auto i = timestamp_str.find_first_not_of('0');
    if (i == -1)
        i = timestamp_str.size() - 1;

    return Timestamp(timestamp_str.substr(i));
}

bool Timestamp::operator<(const Timestamp& other) const
{
    // infinity => bool ordering (false < true)
    if (is_inf_ != other.is_inf_)
        return is_inf_ < other.is_inf_;

    // smaller timestamp strings => smaller timestamps
    if (timestamp_str_.size() != other.timestamp_str_.size())
        return timestamp_str_.size() < other.timestamp_str_.size();

    // same string size => lexicographical order
    return timestamp_str_ < other.timestamp_str_;
}

std::ostream& operator<<(std::ostream& output, const Timestamp& timestamp)
{
    if (timestamp.is_inf_) {
        output << "infinity";
    } else {
        output << timestamp.timestamp_str_;
    }
    return output;
}
