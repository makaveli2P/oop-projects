#include "strset/command.h"

#include <array>

namespace strset {

namespace {

struct Entry {
    CommandKind kind;
    char letter;
    const char* summary;
};

constexpr std::array<Entry, 10> kCommands{{
    {CommandKind::Erase, 'e', "Erase S1"},
    {CommandKind::Switch, 's', "Switch S1 and S2"},
    {CommandKind::Subset, 'b', "Is S2 a proper subset of S1?"},
    {CommandKind::Copy, 'c', "Copy S1 into S2"},
    {CommandKind::List, 'l', "List S1 and S2 in alphabetical order"},
    {CommandKind::Add, 'a', "Add a string to S1"},
    {CommandKind::Union, 'u', "Union: S1 = S1 ∪ S2"},
    {CommandKind::Intersection, 'i', "Intersection: S1 = S1 ∩ S2"},
    {CommandKind::Quit, 'q', "Print the sets and quit"},
    {CommandKind::Help, '?', "Show the commands"},
}};

bool is_space(char ch) noexcept { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\v' || ch == '\f'; }

}  // namespace

char letter_of(CommandKind kind) noexcept {
    for (const Entry& entry : kCommands) {
        if (entry.kind == kind) return entry.letter;
    }
    return '?';
}

const char* summary_of(CommandKind kind) noexcept {
    for (const Entry& entry : kCommands) {
        if (entry.kind == kind) return entry.summary;
    }
    return "";
}

bool command_for_letter(char letter, CommandKind& kind) noexcept {
    for (const Entry& entry : kCommands) {
        if (entry.letter == letter) {
            kind = entry.kind;
            return true;
        }
    }
    return false;
}

std::string_view trim(std::string_view text) noexcept {
    while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
    while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
    return text;
}

ParseResult parse_command(std::string_view line) {
    const std::string_view text = trim(line);
    if (text.empty()) {
        return std::monostate{};
    }
    CommandKind kind{};
    if (!command_for_letter(text.front(), kind)) {
        return ParseError{"Unknown command '" + std::string{text.substr(0, 1)} + "'. Type ? to see the commands."};
    }
    const std::string_view rest = text.substr(1);
    if (kind == CommandKind::Add) {
        if (!rest.empty() && !is_space(rest.front())) {
            return ParseError{"Put a space between a and the string, for example: a pear"};
        }
        return Command{kind, std::string{trim(rest)}};
    }
    if (!rest.empty()) {
        return ParseError{std::string{"The command '"} + text.front() + "' takes no argument."};
    }
    return Command{kind, {}};
}

}  // namespace strset
