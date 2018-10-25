#pragma once

#include <algorithm>
#include <optional>
#include <ostream>
#include <string_view>

class Timestamp
{
    // We use string views since we don't need to keep timestamps.
    //
    // WARNING: all input timstamp strings must live longer than their
    // associated timestamps!
    typedef std::string_view Str;

    //! Constructimestamp_str a timestamp from a string.
    //!
    //! This method is private to enforce checks on the underlying string, use
    //! the `parse()` method instead.
    Timestamp(Str&& timestamp_str);

    struct Infinity {};

    //! Constructimestamp_str a timestamp larger than all others.
    Timestamp(Infinity);

    friend std::ostream& operator<<(std::ostream&, const Timestamp&);

public:
    static const Timestamp Min;
    static const Timestamp Max;

    //! Parse a string as a timestamp.
    //!
    //! A timestamp string is considered valid if it only contains digitimestamp_str.
    //! Leading zeros are stripped when parsing.
    //!
    //! @param[in] timestamp_str The timestamp string.
    //!
    //! @return The parsed timestamp, or none if it isn't valid.
    static std::optional<Timestamp> parse(const Str& timestamp_str);

    //! Compare two timestamps.
    bool operator<(const Timestamp& other) const;

    inline bool operator>(const Timestamp& other) const
    { return other < *this; }

private:
    Str timestamp_str_;
    bool is_inf_;
};
