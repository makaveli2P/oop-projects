#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <string>

#include "helpers.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {

std::string repeat(const std::string& line, int count) {
    std::string text;
    for (int i = 0; i < count; ++i) text += line;
    return text;
}

/// "A", "B", ..., "Z", "AA", "AB", ...: distinct alphabetic names.
std::string name(int index) {
    std::string text;
    ++index;
    while (index > 0) {
        --index;
        text.insert(text.begin(), static_cast<char>('A' + index % 26));
        index /= 26;
    }
    return text;
}

}  // namespace

TEST_CASE("Blank lines and comments take no address", "[assembler]") {
    const auto m = test::load(
        "; header comment\n"
        "\n"
        "DEC X        ; inline comment\n"
        "   # hash comment\n"
        "\t\n"
        "LDI 5 # another\n"
        "HLT\n");
    const auto& code = m.program().code();
    REQUIRE(code.size() == 3);
    CHECK(code[0].text == "DEC X");
    CHECK(code[0].line == 3);
    CHECK(code[1].text == "LDI 5");
    CHECK(code[1].line == 6);
    CHECK(code[2].text == "HLT");
    CHECK(code[2].line == 7);
}

TEST_CASE("CRLF line ends and trailing spaces are accepted", "[assembler]") {
    const auto m = test::load("DEC X \r\nLDI 7\t\r\nSTR X  \r\n\r\nHLT\r\n");
    REQUIRE(m.program().size() == 4);
    CHECK(m.program().at(1).text == "LDI 7");
    CHECK(m.program().at(3).line == 5);
}

TEST_CASE("The last line needs no line end", "[assembler]") {
    const auto m = test::load("LDI 1\nHLT");
    CHECK(m.program().size() == 2);
}

TEST_CASE("Opcodes are case-insensitive", "[assembler]") {
    const auto m = test::load("dec X\nLdI 3\nstr X\nxch\nAdd\njmp 6\nhlt\n");
    CHECK(m.program().at(0).instruction->opcode() == sal::Opcode::DEC);
    CHECK(m.program().at(4).instruction->opcode() == sal::Opcode::ADD);
    CHECK(m.program().at(6).instruction->mnemonic() == "HLT");
}

TEST_CASE("Symbols are case-sensitive", "[assembler]") {
    const auto errors = test::errors_of("DEC Total\nLDA total\nHLT\n");
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].line == 2);
    CHECK(errors[0].column == 5);
    CHECK_THAT(errors[0].message, ContainsSubstring("undeclared symbol 'total'"));

    const auto m = test::load("DEC x\nDEC X\nHLT\n");
    CHECK(m.program().address_of("x") == 128);
    CHECK(m.program().address_of("X") == 129);
}

TEST_CASE("DEC gives data addresses in declaration order at load time", "[assembler]") {
    const auto m = test::load("LDA B\nDEC A\nLDI 1\nDEC B\nDEC C\nHLT\n");
    const auto& symbols = m.program().symbols();
    REQUIRE(symbols.size() == 3);
    CHECK(symbols[0].name == "A");
    CHECK(symbols[0].address == 128);
    CHECK(symbols[1].name == "B");
    CHECK(symbols[1].address == 129);
    CHECK(symbols[2].address == 130);
    // A symbol can be used before its DEC line.
    const auto& lda = dynamic_cast<const sal::Lda&>(*m.program().at(0).instruction);
    CHECK(lda.address() == 129);
    CHECK(m.program().symbol_at(130)->name == "C");
    CHECK(m.program().symbol_at(131) == nullptr);
    CHECK(m.program().symbol_at(5) == nullptr);
}

TEST_CASE("Unknown opcode is an error with line and column", "[assembler][errors]") {
    const auto errors = test::errors_of("LDI 1\n\n  FOO 3\nHLT\n");
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].line == 3);
    CHECK(errors[0].column == 3);
    CHECK_THAT(errors[0].message, ContainsSubstring("unknown opcode 'FOO'"));
    CHECK(sal::to_string(errors[0]) == "line 3, column 3: unknown opcode 'FOO'");
}

TEST_CASE("Wrong operand count is an error", "[assembler][errors]") {
    SECTION("missing operand") {
        const auto errors = test::errors_of("DEC X\nLDA\nHLT\n");
        REQUIRE(errors.size() == 1);
        CHECK(errors[0].line == 2);
        CHECK(errors[0].message == "LDA expects 1 operand, found 0");
    }
    SECTION("extra operand") {
        const auto errors = test::errors_of("LDI 1 2\nHLT\n");
        REQUIRE(errors.size() == 1);
        CHECK(errors[0].line == 1);
        CHECK(errors[0].column == 7);
        CHECK(errors[0].message == "LDI expects 1 operand, found 2");
    }
    SECTION("operand on an instruction without operands") {
        const auto errors = test::errors_of("LDI 1\nADD 5\nHLT X\n");
        REQUIRE(errors.size() == 2);
        CHECK(errors[0].line == 2);
        CHECK(errors[0].message == "ADD expects 0 operands, found 1");
        CHECK(errors[1].line == 3);
    }
}

TEST_CASE("Symbols must be alphabetic", "[assembler][errors]") {
    const auto errors = test::errors_of("DEC X1\nDEC Y\nLDA Y_2\nHLT\n");
    REQUIRE(errors.size() == 2);
    CHECK(errors[0].line == 1);
    CHECK_THAT(errors[0].message, ContainsSubstring("not a valid symbol"));
    CHECK(errors[1].line == 3);
}

TEST_CASE("Duplicate DEC is an error", "[assembler][errors]") {
    const auto errors = test::errors_of("DEC X\nDEC Y\nDEC X\nHLT\n");
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].line == 3);
    CHECK(errors[0].column == 5);
    CHECK_THAT(errors[0].message, ContainsSubstring("duplicate declaration of symbol 'X'"));
}

TEST_CASE("At most 128 symbols", "[assembler][errors]") {
    std::string ok;
    for (int i = 0; i < 128; ++i) ok += "DEC " + name(i) + "\n";
    CHECK(sal::assemble(ok).ok());

    // 129 symbols need 129 DEC lines, so the instruction limit also fails.
    std::string over;
    for (int i = 0; i < 129; ++i) over += "DEC " + name(i) + "\n";
    const auto errors = test::errors_of(over);
    REQUIRE(errors.size() == 2);
    CHECK(errors[0].line == 129);
    CHECK_THAT(errors[0].message, ContainsSubstring("more than 128 instructions"));
    CHECK(errors[1].line == 129);
    CHECK_THAT(errors[1].message, ContainsSubstring("more than 128 symbols"));
}

TEST_CASE("At most 128 instructions", "[assembler][errors]") {
    CHECK(sal::assemble(repeat("XCH\n", 127) + "HLT\n").ok());
    const auto errors = test::errors_of("; comment\n" + repeat("XCH\n", 128) + "HLT\n");
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].line == 130);
    CHECK_THAT(errors[0].message, ContainsSubstring("more than 128 instructions"));
}

TEST_CASE("An empty program is an error", "[assembler][errors]") {
    for (const char* source : {"", "\n\n", "; only a comment\r\n"}) {
        const auto errors = test::errors_of(source);
        REQUIRE(errors.size() == 1);
        CHECK(errors[0].line == 0);
        CHECK(sal::to_string(errors[0]) == "the program contains no instructions");
    }
}

TEST_CASE("LDI accepts every int32 value and nothing else", "[assembler]") {
    SECTION("valid values") {
        const auto m = test::load("LDI 2147483647\nLDI -2147483648\nLDI +7\nLDI -0\nLDI 007\nHLT\n");
        auto value = [&](std::size_t address) {
            return dynamic_cast<const sal::Ldi&>(*m.program().at(address).instruction).value();
        };
        CHECK(value(0) == 2147483647);
        CHECK(value(1) == -2147483647 - 1);
        CHECK(value(2) == 7);
        CHECK(value(3) == 0);
        CHECK(value(4) == 7);
    }
    SECTION("invalid values") {
        const auto errors = test::errors_of(
            "LDI 2147483648\n"
            "LDI -2147483649\n"
            "LDI 99999999999999999999999\n"
            "LDI 12a\n"
            "LDI +-5\n"
            "LDI -\n"
            "LDI X\n"
            "HLT\n");
        REQUIRE(errors.size() == 7);
        for (int i = 0; i < 7; ++i) {
            CHECK(errors[static_cast<std::size_t>(i)].line == i + 1);
            CHECK(errors[static_cast<std::size_t>(i)].column == 5);
            CHECK_THAT(errors[static_cast<std::size_t>(i)].message,
                       ContainsSubstring("is not a 32-bit signed integer"));
        }
    }
}

TEST_CASE("Jump targets must be instruction addresses", "[assembler][errors]") {
    CHECK(sal::assemble("JMP 2\nJZS 0\nJVS 1\n").ok());
    const auto errors = test::errors_of("JMP 4\nJZS -1\nJVS X\nJMP 99999999999999999999\n");
    REQUIRE(errors.size() == 4);
    CHECK(errors[0].line == 1);
    CHECK_THAT(errors[0].message, ContainsSubstring("jump target '4' is not an instruction address (0-3)"));
    CHECK(errors[1].line == 2);
    CHECK(errors[2].line == 3);
    CHECK(errors[3].line == 4);
}

TEST_CASE("The assembler reports all errors in line order", "[assembler][errors]") {
    const auto errors = test::errors_of(
        "DEC X\n"
        "BAD\n"
        "LDA Y\n"
        "DEC X\n"
        "LDI\n"
        "HLT\n");
    REQUIRE(errors.size() == 4);
    CHECK(errors[0].line == 2);
    CHECK(errors[1].line == 3);
    CHECK(errors[2].line == 4);
    CHECK(errors[3].line == 5);
}

TEST_CASE("The assembler does not crash on garbage", "[assembler][errors]") {
    std::string garbage;
    for (int i = 0; i < 2000; ++i) {
        garbage += static_cast<char>((i * 7919) % 256);
    }
    const auto result = sal::assemble(garbage);
    CHECK_FALSE(result.ok());
    CHECK_FALSE(result.errors().empty());
}

TEST_CASE("assemble_file reports a file that does not exist", "[assembler][errors]") {
    const auto result = sal::assemble_file("/no/such/dir/program.sal");
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.errors().size() == 1);
    CHECK_THAT(result.errors()[0].message, ContainsSubstring("cannot open file"));
}
