// Tests for the command parser.
#include <catch2/catch_test_macros.hpp>

#include "strset/command.h"

using namespace strset;

namespace {

Command command(std::string_view line) {
    const ParseResult result = parse_command(line);
    REQUIRE(std::holds_alternative<Command>(result));
    return std::get<Command>(result);
}

std::string error(std::string_view line) {
    const ParseResult result = parse_command(line);
    REQUIRE(std::holds_alternative<ParseError>(result));
    return std::get<ParseError>(result).message;
}

}  // namespace

TEST_CASE("Each letter is a command", "[command]") {
    CHECK(command("e").kind == CommandKind::Erase);
    CHECK(command("s").kind == CommandKind::Switch);
    CHECK(command("b").kind == CommandKind::Subset);
    CHECK(command("c").kind == CommandKind::Copy);
    CHECK(command("l").kind == CommandKind::List);
    CHECK(command("u").kind == CommandKind::Union);
    CHECK(command("i").kind == CommandKind::Intersection);
    CHECK(command("q").kind == CommandKind::Quit);
    CHECK(command("?").kind == CommandKind::Help);
    CHECK(command("  q \r").kind == CommandKind::Quit);  // Spaces and CRLF do not count.
}

TEST_CASE("a takes the rest of the line as the string", "[command]") {
    CHECK(command("a pear").argument == "pear");
    CHECK(command("a   two words  ").argument == "two words");
    CHECK(command("a\tpear").argument == "pear");
    const Command bare = command("a");
    CHECK(bare.kind == CommandKind::Add);
    CHECK(bare.argument.empty());
}

TEST_CASE("Bad lines give a message", "[command]") {
    CHECK(std::holds_alternative<std::monostate>(parse_command("")));
    CHECK(std::holds_alternative<std::monostate>(parse_command("   ")));
    CHECK(error("x") == "Unknown command 'x'. Type ? to see the commands.");
    CHECK(error("E") == "Unknown command 'E'. Type ? to see the commands.");  // Letters are case-sensitive.
    CHECK(error("e now") == "The command 'e' takes no argument.");
    CHECK(error("list") == "The command 'l' takes no argument.");
    CHECK(error("apear") == "Put a space between a and the string, for example: a pear");
}

TEST_CASE("letter_of and summary_of describe each command", "[command]") {
    CHECK(letter_of(CommandKind::Subset) == 'b');
    CHECK(std::string{summary_of(CommandKind::Subset)} == "Is S2 a proper subset of S1?");
    CommandKind kind{};
    CHECK(command_for_letter('u', kind));
    CHECK(kind == CommandKind::Union);
    CHECK_FALSE(command_for_letter('z', kind));
}
