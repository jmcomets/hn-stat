#pragma once

#include <type_traits>
#include <unordered_map>
#include <map>

template <typename Owned, typename Ref = const Owned&>
class MaxOccurrenceRanker
{
    static_assert(std::is_constructible<Owned, Ref>::value,
                  "Owned should be constructible from Ref");

public:
    typedef unsigned int Count;

    //! Construct a ranker with the number of highest occurring elements to track.
    explicit MaxOccurrenceRanker(Count n):
        n_(n)
    {
    }

    //! Add a new element to the ranker, incrementing its rank,
    //! potentially including it in the `n` elements of highest rank.
    void update(Ref element)
    {
        auto insertIt = occurrences_.emplace(element, 0).first;
        const Owned& insertedElement = insertIt->first;
        Count count = ++insertIt->second;

        // erase an existing entry for this element
        for (auto range = rankedOccurrences_.equal_range(count-1); range.first != range.second; ++range.first) {
            if (range.first->second == element) {
                rankedOccurrences_.erase(range.first);
                break;
            }
        }

        if (rankedOccurrences_.size() == n_) {
            Count smallestCount = rankedOccurrences_.rbegin()->first;
            if (count > smallestCount) {
                rankedOccurrences_.emplace(count, insertedElement);

                // only keep the n max occurrences
                rankedOccurrences_.erase(smallestCount);
            }
        } else {
            rankedOccurrences_.emplace(count, insertedElement);
        }
    }

    //! Visit the `n` elements of highest rank.
    template <typename F>
    void visit(F f) const
    {
        for (const std::pair<Count, Ref>& p : rankedOccurrences_) {
            f(p.second, p.first);
        }
    }

private:
    Count n_;
    std::unordered_map<Owned, Count> occurrences_;
    std::multimap<Count, Ref, std::greater<Count>> rankedOccurrences_;
};
