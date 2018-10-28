#pragma once

#include <optional>

template <typename C>
class Iterator
{
public:
    explicit Iterator(const C& container):
        begin_(container.begin()),
        end_(container.end())
    {
    }

    std::optional<typename C::value_type> next()
    {
        if (begin_ == end_)
            return std::nullopt;
        return *begin_++;
    }

private:
    typename C::const_iterator begin_;
    typename C::const_iterator end_;
};
