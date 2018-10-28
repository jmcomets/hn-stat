#pragma once

#include <optional>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>

#include "iterator.h"

enum class ArgumentConstraint
{
    None,
    Optional,
    Required,
};

extern const ArgumentConstraint NoArgument;
extern const ArgumentConstraint ArgumentOptional;
extern const ArgumentConstraint ArgumentRequired;

class Option
{
    typedef std::pair<std::string, std::string> DescriptionItem;

protected:
    Option(std::optional<char> shortName, std::optional<std::string> longName, ArgumentConstraint constraint, std::string description);

public:
    Option(char shortName, std::string longName, ArgumentConstraint constraint = ArgumentConstraint::None, std::string description = "");

    Option(char shortName, std::string longName, std::string description);

    bool hasShortName() const;
    bool hasLongName() const;

    const std::optional<char>& getShortName() const;
    const std::optional<std::string>& getLongName() const;

    const std::string& getDescription() const;

    ArgumentConstraint getArgumentConstraint() const;

    DescriptionItem toDescriptionItem() const;

    // Comparator ordering options for use in a description.
    //
    // Sorts options so that short-only options are before long-only options,
    // with options having both a short and long name come last.
    struct DescriptionComparator
    {
        bool operator()(const Option& lhs, const Option& rhs) const;
    };

    // Two options conflict if either their short name or their long name
    // are the same, whereas the description is just extra information.
    struct ConflictComparator
    {
        bool operator()(const Option& lhs, const Option& rhs) const;
    };

private:
    std::optional<char> shortName_;
    std::optional<std::string> longName_;
    ArgumentConstraint constraint_;
    std::string description_;
};

class ShortOption : public Option
{
public:
    ShortOption(char shortName, ArgumentConstraint constraint = ArgumentConstraint::None, std::string description = "");

    ShortOption(char shortName, std::string description);
};

class LongOption : public Option
{
public:
    LongOption(std::string longName, ArgumentConstraint constraint = ArgumentConstraint::None, std::string description = "");

    LongOption(std::string longName, std::string description);
};

class Options
{
    typedef std::set<Option, Option::ConflictComparator> NonConflictingOptions;

public:
    Options(std::initializer_list<Option> il = {});

    std::string help() const;

    NonConflictingOptions::const_iterator begin() const;
    NonConflictingOptions::const_iterator end() const;

private:
    NonConflictingOptions options_;
};

class Arguments
{
    typedef std::vector<std::string> PositionalArguments;

public:
    bool hasOption(char shortName) const;
    bool hasOption(std::string longName) const;

    std::optional<std::string_view> getOption(char shortName) const;
    std::optional<std::string_view> getOption(std::string longName) const;

    Iterator<PositionalArguments> getPositional() const;

    void setOption(Option option, std::optional<std::string> value = {});
    void addPositional(std::string value);

private:
    std::map<Option, std::optional<std::string>, Option::ConflictComparator> optionValues_;
    PositionalArguments positionalArguments_;
};

class Parser
{
public:
    Parser(Options options);

    std::optional<Arguments> parse(int argc, char* argv[]) const;

private:
    std::string optstring_;
    std::vector<struct option> longopts_;
    std::unordered_map<int, Option> optionsById_;
};
