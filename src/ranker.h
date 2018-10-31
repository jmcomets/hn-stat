#pragma once

#include <type_traits>
#include <unordered_map>

template <typename Owned, typename Ref = const Owned&>
class MaxOccurrenceRanker
{
    static_assert(std::is_constructible<Owned, Ref>::value,
                  "Owned should be constructible from Ref");

public:
    typedef unsigned int Count;

    //! Add a new element to the ranker.
    void update(Ref element)
    {
        auto insertIt = occurrences_.emplace(element, 0).first;
        ++insertIt->second;
    }

    //! Get the `n` elements of highest rank.
    std::vector<std::pair<Ref, Count>> top(Count n) const
    {
        std::vector<std::pair<Ref, Count>> v;
        v.reserve(occurrences_.size());
        std::copy(occurrences_.begin(), occurrences_.end(),
                  std::back_inserter(v));
        std::sort(v.begin(), v.end(),
                [](const std::pair<Ref, Count>& p1, const std::pair<Ref, Count>& p2) {
                    return p1.second > p2.second;
                });
        v.resize(n);
        return v;
    }

private:
    std::unordered_map<Owned, Count> occurrences_;
};
