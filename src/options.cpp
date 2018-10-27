#include "options.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <cassert>

#include <getopt.h>

const ArgumentConstraint NoArgument       = ArgumentConstraint::None;
const ArgumentConstraint ArgumentOptional = ArgumentConstraint::Optional;
const ArgumentConstraint ArgumentRequired = ArgumentConstraint::Required;

namespace {

int toGetOptConstraint(ArgumentConstraint constraint)
{
    switch (constraint) {
        case ArgumentConstraint::None:
            return no_argument;

        case ArgumentConstraint::Optional:
            return optional_argument;

        case ArgumentConstraint::Required:
            return required_argument;

        default:
            std::abort();
    }
}

typedef std::pair<std::string, std::string> DescriptionItem;
typedef std::vector<DescriptionItem> DescriptionList;

std::string formatDescriptionList(const DescriptionList& descriptionList,
                                  std::string_view prefix = "", std::size_t paddingWidth = 4)
{
    if (descriptionList.empty())
        return "";

    auto comparePrefixLengths = [](const DescriptionItem& lhs, const DescriptionItem& rhs) {
        return lhs.first.size() < rhs.first.size();
    };

    auto maxIt = std::max_element(descriptionList.begin(), descriptionList.end(),
                                  comparePrefixLengths);
    std::size_t maxPrefixLength = maxIt->first.size();

    std::ostringstream formatted;

    const auto itBegin = descriptionList.begin();
    const auto itEnd = descriptionList.end();
    for (auto it = itBegin; it != itEnd; ++it) {
        if (it != itBegin)
            formatted << "\n";
        formatted << prefix;

        std::size_t paddingLength = paddingWidth + maxPrefixLength - it->first.size();
        std::string padding(paddingLength, ' ');
        formatted << it->first << padding << it->second;
    }

    return formatted.str();
}

} // namespace

Option::Option(std::optional<char> shortName, std::optional<std::string> longName,
               ArgumentConstraint constraint, std::string description):
    shortName_(std::move(shortName)),
    longName_(std::move(longName)),
    constraint_(constraint),
    description_(std::move(description))
{
    assert(shortName_ || longName_);
}

Option::Option(char shortName, std::string longName, ArgumentConstraint constraint, std::string description):
    Option(shortName, std::make_optional(std::move(longName)), constraint, std::move(description))
{
}

Option::Option(char shortName, std::string longName, std::string description):
    Option(shortName, std::make_optional(std::move(longName)), ArgumentConstraint::None, std::move(description))
{
}

bool Option::hasShortName() const
{
    return shortName_.has_value();
}

const std::optional<char>& Option::getShortName() const
{
    return shortName_;
}

bool Option::hasLongName() const
{
    return longName_.has_value();
}

const std::optional<std::string>& Option::getLongName() const
{
    return longName_;
}

ArgumentConstraint Option::getArgumentConstraint() const
{
    return constraint_;
}

const std::string& Option::getDescription() const
{
    return description_;
}

DescriptionItem Option::toDescriptionItem() const
{
    std::ostringstream prefix;

    if (shortName_) {
        prefix << "-" << *shortName_;
        if (longName_)
            prefix << ", ";
    }

    if (longName_)
        prefix << "--" << *longName_;
    return { prefix.str(), description_ };
}

bool Option::DescriptionComparator::operator()(const Option& lhs, const Option& rhs) const
{
    // (null, --another-option) before (-a, --an-option)
    // and
    // (-a, --another-option) after (null, --an-option)
    if (rhs.hasShortName() != lhs.hasShortName())
        return lhs.hasShortName() < rhs.hasShortName();

    // (-b, null) before (-a, --an-option)
    // and
    // (-a, --an-option) after (-b, null)
    if (rhs.hasLongName() != lhs.hasLongName())
        return lhs.hasLongName() < rhs.hasLongName();

    // (-a, null) before (-b, null)
    if (lhs.hasShortName() && rhs.hasShortName()) {
        return lhs.getShortName() < rhs.getShortName();
    }

    // (null, --an-option) before (null, --another-option)
    if (lhs.hasLongName() && rhs.hasLongName()) {
        return lhs.getLongName() < rhs.getLongName();
    }

    // unreachable code (see assertion)
    std::abort();
}

bool Option::ConflictComparator::operator()(const Option& lhs, const Option& rhs) const
{
    if (lhs.hasShortName() && rhs.hasShortName()) {
        return lhs.getShortName() < rhs.getShortName();
    }

    if (lhs.hasLongName() && rhs.hasLongName()) {
        return lhs.getLongName() < rhs.getLongName();
    }

    // short options before long options (arbitrary)
    return lhs.hasShortName();
}

ShortOption::ShortOption(char shortName, ArgumentConstraint constraint, std::string description):
    Option(shortName, std::nullopt, constraint, std::move(description))
{
}

ShortOption::ShortOption(char shortName, std::string description):
    ShortOption(shortName, ArgumentConstraint::None, std::move(description))
{
}

LongOption::LongOption(std::string longName, ArgumentConstraint constraint, std::string description):
    Option(std::nullopt, std::move(longName), constraint, std::move(description))
{
}

LongOption::LongOption(std::string longName, std::string description):
    LongOption(std::move(longName), ArgumentConstraint::None, std::move(description))
{
}

Options::Options(std::initializer_list<Option> il):
    options_(il)
{
}

Options::NonConflictingOptions::const_iterator Options::begin() const
{
    return options_.begin();
}

Options::NonConflictingOptions::const_iterator Options::end() const
{
    return options_.end();
}

std::string Options::help() const
{
    if (options_.empty())
        return "";

    typedef std::reference_wrapper<const Option> OptionRef;

    // sort the options for a description
    std::vector<OptionRef> descriptionOptions { options_.begin(), options_.end() };
    std::sort(descriptionOptions.begin(), descriptionOptions.end(),
                Option::DescriptionComparator());

    DescriptionList descriptionList;
    std::transform(descriptionOptions.begin(), descriptionOptions.end(),
                    std::back_inserter(descriptionList),
                    [](const OptionRef& option) {
                        return option.get().toDescriptionItem();
                    });
    return "Options:\n" + formatDescriptionList(descriptionList, "  ");
}

bool Arguments::hasOption(char shortName) const
{
    auto findIt = optionValues_.find(ShortOption(shortName));
    return findIt != optionValues_.end();
}

std::optional<std::string_view> Arguments::getOption(char shortName) const
{
    auto findIt = optionValues_.find(ShortOption(shortName));
    if (findIt != optionValues_.end())
        return findIt->second;
    return std::nullopt;
}

bool Arguments::hasOption(std::string longName) const
{
    auto findIt = optionValues_.find(LongOption(std::move(longName)));
    return findIt != optionValues_.end();
}

std::optional<std::string_view> Arguments::getOption(std::string longName) const
{
    auto findIt = optionValues_.find(LongOption(std::move(longName)));
    if (findIt != optionValues_.end())
        return findIt->second;
    return std::nullopt;
}

void Arguments::setOption(Option option, std::optional<std::string> value)
{
    if (!optionValues_.emplace(std::move(option), std::move(value)).second) {
        // conflicting option values
    }
}

void Arguments::addPositional(std::string value)
{
    positionalArguments_.push_back(std::move(value));
}

Iterator<Arguments::PositionalArguments> Arguments::getPositional() const
{
    return Iterator<Arguments::PositionalArguments>(positionalArguments_);
}

Parser::Parser(Options options)
{
    int nextId = 256;
    for (const Option& option : options) {
        if (const auto& shortName = option.getShortName()) {
            optstring_ += *shortName;

            switch (option.getArgumentConstraint()) {
                case ArgumentConstraint::Required:
                    optstring_ += ":";
                    break;

                case ArgumentConstraint::Optional:
                    optstring_ += "::";
                    break;

                default:
                    break;
            }

            int id = static_cast<int>(*shortName);
            optionsById_.emplace(id, option);
        }

        if (const auto& longName = option.getLongName()) {
            struct option longopt;

            longopt.name = longName->c_str();
            longopt.has_arg = toGetOptConstraint(option.getArgumentConstraint());
            longopt.flag = 0;

            if (const auto& shortName = option.getShortName()) {
                longopt.val = *shortName;
            } else {
                longopt.val = nextId++;
            }

            optionsById_.emplace(longopt.val, option);
            longopts_.push_back(longopt);
        }
    }
    longopts_.push_back({ NULL, 0, NULL, 0 });
}

std::optional<Arguments> Parser::parse(int argc, char* argv[]) const
{
    Arguments arguments;

    int id;
    while ((id = getopt_long(argc, argv, optstring_.c_str(), &longopts_[0], NULL)) != -1) {
        auto findIt = optionsById_.find(id);
        if (findIt == optionsById_.end())
            return std::nullopt;
        const Option& option = findIt->second;

        switch (option.getArgumentConstraint()) {
            case ArgumentConstraint::Required: {
                arguments.setOption(option, optarg);
                break;
            }

            case ArgumentConstraint::Optional: {
                if (optarg) {
                    arguments.setOption(option, optarg);
                } else {
                    arguments.setOption(option);
                }
                break;
            }

            case ArgumentConstraint::None: {
                arguments.setOption(option);
                break;
            }

            default:
                break;
        }
    }

    for (; optind < argc; optind++) {
        arguments.addPositional(argv[optind]);
    }

    return arguments;
}
