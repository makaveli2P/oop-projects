// command.h - the commands of the set calculator and the parser for one
// command line.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace strset {

enum class CommandKind : std::uint8_t {
    Erase,         ///< e: erase S1.
    Switch,        ///< s: swap S1 and S2.
    Subset,        ///< b: is S2 a proper subset of S1?
    Copy,          ///< c: deep copy S1 into S2.
    List,          ///< l: list S1 and S2 in alphabetical order.
    Add,           ///< a <string>: add a string to S1.
    Union,         ///< u: S1 = S1 ∪ S2.
    Intersection,  ///< i: S1 = S1 ∩ S2.
    Quit,          ///< q: print the sets and quit.
    Help,          ///< ?: show the commands.
};

struct Command {
    CommandKind kind;
    /// The string for Add. Empty for the other commands, and for an Add
    /// without a string (the caller then asks for the string).
    std::string argument;
};

/// The letter of a command: 'e', 's', ..., '?'.
[[nodiscard]] char letter_of(CommandKind kind) noexcept;
/// A short description, for example "Erase S1".
[[nodiscard]] const char* summary_of(CommandKind kind) noexcept;
/// The command for a letter, if there is one.
[[nodiscard]] bool command_for_letter(char letter, CommandKind& kind) noexcept;

/// An error message for a line that is not a command.
struct ParseError {
    std::string message;
};

/// The result of parse_command(). An empty line gives std::monostate.
using ParseResult = std::variant<std::monostate, Command, ParseError>;

/// Parses one command line. The rules:
/// - Spaces at the start and at the end do not count.
/// - The first character is the command letter. Letters are case-sensitive.
/// - For "a", the rest of the line (without the spaces around it) is the
///   string. The string can contain spaces. "a" alone gives an Add with an
///   empty argument.
/// - The other commands take no argument: "e x" is an error.
[[nodiscard]] ParseResult parse_command(std::string_view line);

/// `text` without the spaces, tabs and line ends at the start and at the end.
[[nodiscard]] std::string_view trim(std::string_view text) noexcept;

}  // namespace strset
