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
        std::vector<std::pair<Ref, Count>> topN;
        topN.reserve(n);
        for (const std::pair<Owned, Count>& p : occurrences_) {
            std::pair<Ref, Count> item(p.first, p.second);
            auto insertIt = std::upper_bound(topN.begin(), topN.end(), item,
                                             [](const std::pair<Ref, Count>& p1, const std::pair<Ref, Count>& p2) {
                                                return p1.second > p2.second;
                                             });
            topN.insert(insertIt, item);
            if (topN.size() > n)
                topN.pop_back();
        }
        return topN;
    }

private:
    std::unordered_map<Owned, Count> occurrences_;
};
