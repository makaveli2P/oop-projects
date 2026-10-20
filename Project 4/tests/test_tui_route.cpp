// Tests for the routes: the source and target units of each micro-operation.
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "helpers.hpp"
#include "sal_tui/route.hpp"

using sal::tui::Route;
using sal::tui::Unit;

namespace {

/// The routes of the first instruction after `skip` instructions.
std::vector<Route> routes(std::string_view source, int skip = 0,
                          sal::tui::NumberBase base = sal::tui::NumberBase::Decimal) {
    sal::Machine m = test::load(source);
    test::steps(m, skip);
    const sal::StepResult step = m.step();
    std::vector<Route> result;
    for (std::size_t i = 0; i < step.micro_ops.size(); ++i) {
        result.push_back(sal::tui::route_of(step.micro_ops, i, base));
    }
    return result;
}

bool is(const Route& r, std::vector<Unit> sources, std::vector<Unit> targets) {
    return r.sources == sources && r.targets == targets;
}

}  // namespace

TEST_CASE("Fetch and decode routes", "[tui][route]") {
    const auto r = routes("LDI 5\n");
    CHECK(is(r[0], {Unit::PC, Unit::Memory}, {Unit::IR}));
    CHECK(r[0].label == "LDI 5");
    CHECK(is(r[1], {Unit::IR}, {Unit::Decoder}));
    CHECK(r[1].label == "LDI");
}

TEST_CASE("LDI writes A from the decoder", "[tui][route]") {
    const auto r = routes("LDI -3\n");
    CHECK(is(r[2], {Unit::Decoder}, {Unit::A}));
    CHECK(r[2].label == "-3");
    CHECK(is(r[3], {}, {Unit::PC}));  // PC + 1 has no source unit.
}

TEST_CASE("LDA and LDB move a word from memory to a register", "[tui][route]") {
    const char* source = "DEC X\nLDI 7\nSTR X\nLDA X\nLDB X\n";
    const auto lda = routes(source, 3);
    CHECK(is(lda[2], {Unit::Memory}, {Unit::A}));
    CHECK(lda[2].label == "7");
    CHECK(is(lda[3], {Unit::Memory}, {Unit::A}));
    const auto ldb = routes(source, 4);
    CHECK(is(ldb[2], {Unit::Memory}, {Unit::B}));
    CHECK(is(ldb[3], {Unit::Memory}, {Unit::B}));
}

TEST_CASE("STR moves A to memory", "[tui][route]") {
    const auto r = routes("DEC X\nLDI 9\nSTR X\n", 2);
    CHECK(is(r[2], {Unit::A}, {Unit::Memory}));
    CHECK(is(r[3], {Unit::A}, {Unit::Memory}));
    CHECK(r[3].label == "9");
}

TEST_CASE("XCH moves each register to the other", "[tui][route]") {
    const auto r = routes("LDI 1\nXCH\n", 1);
    CHECK(is(r[2], {Unit::A}, {Unit::B}));
    CHECK(is(r[3], {Unit::B}, {Unit::A}));
    CHECK(is(r[4], {Unit::B}, {Unit::A}));  // RegWrite A gets the value of B.
    CHECK(is(r[5], {Unit::A}, {Unit::B}));  // RegWrite B gets the value of A.
}

TEST_CASE("ADD routes go through the ALU", "[tui][route]") {
    const auto r = routes("LDI 2147483647\nXCH\nLDI 1\nADD\n", 3);
    CHECK(is(r[2], {Unit::A}, {Unit::ALU}));
    CHECK(is(r[3], {Unit::B}, {Unit::ALU}));
    CHECK(is(r[4], {Unit::A, Unit::B}, {Unit::ALU}));
    CHECK(r[4].label == "-2147483648");
    CHECK(is(r[5], {Unit::ALU}, {Unit::A}));
    CHECK(is(r[6], {Unit::ALU}, {Unit::Z}));
    CHECK(is(r[7], {Unit::ALU}, {Unit::V}));
    CHECK(r[7].label == "1");
}

TEST_CASE("Jumps write the PC from the decoder", "[tui][route]") {
    const auto jmp = routes("JMP 1\nHLT\n");
    CHECK(is(jmp[2], {Unit::Decoder}, {Unit::PC}));
    CHECK(jmp[2].label == "1");

    const auto jzs = routes("JZS 1\nHLT\n");  // Z is 0: not taken.
    CHECK(is(jzs[2], {Unit::Z}, {Unit::Decoder}));
    CHECK(jzs[2].label == "Z = 0");
    CHECK(is(jzs[3], {}, {Unit::PC}));
}

TEST_CASE("DEC and HLT routes", "[tui][route]") {
    const auto dec = routes("DEC X\n");
    CHECK(is(dec[2], {Unit::Decoder}, {Unit::Memory}));
    CHECK(dec[2].label == "128");

    const auto hlt = routes("HLT\n");
    CHECK(is(hlt[2], {}, {Unit::PC}));  // The PC keeps its value.
    CHECK(hlt[3].sources.empty());
    CHECK(hlt[3].targets.empty());
}

TEST_CASE("Route labels follow the number base", "[tui][route]") {
    const auto r = routes("LDI -1\n", 0, sal::tui::NumberBase::Hex);
    CHECK(r[2].label == "0xFFFFFFFF");
}

TEST_CASE("Number formats", "[tui][format]") {
    using sal::tui::NumberBase;
    CHECK(sal::tui::format_word(-1293967296, NumberBase::Decimal) == "-1293967296");
    CHECK(sal::tui::format_word(-1293967296, NumberBase::Hex) == "0xB2DFA040");
    CHECK(sal::tui::format_word(0, NumberBase::Hex) == "0x00000000");
    CHECK(sal::tui::format_address(12, NumberBase::Hex) == "0x0C");
    CHECK(sal::tui::truncate("DECREMENTOR", 5) == "DECR…");
    CHECK(sal::tui::truncate("ADD", 5) == "ADD");
    CHECK(sal::tui::display_width("a▶b") == 3);
    CHECK(sal::tui::pad_left("7", 3) == "  7");
    CHECK(sal::tui::pad_right("7", 3) == "7  ");
}
