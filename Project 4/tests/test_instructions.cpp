#include <catch2/catch_test_macros.hpp>

#include "helpers.hpp"

TEST_CASE("DEC changes no data and only advances the PC", "[instructions]") {
    auto m = test::load("DEC X\nDEC Y\nHLT\n");
    const auto before = m.state().memory;
    m.step();
    CHECK(m.pc() == 1);
    CHECK(m.value_of("X") == 0);
    CHECK(m.a() == 0);
    CHECK(m.state().memory == before);
}

TEST_CASE("LDI loads a value into A", "[instructions]") {
    auto m = test::load("LDI -42\nHLT\n");
    m.step();
    CHECK(m.a() == -42);
    CHECK(m.b() == 0);
    CHECK(m.pc() == 1);
}

TEST_CASE("STR stores A, LDA and LDB load from memory", "[instructions]") {
    auto m = test::load("DEC X\nLDI 17\nSTR X\nLDI 0\nLDB X\nLDA X\nHLT\n");
    test::steps(m, 3);
    CHECK(m.value_of("X") == 17);
    CHECK(m.data(128) == 17);
    m.step();
    CHECK(m.a() == 0);
    m.step();
    CHECK(m.b() == 17);
    CHECK(m.a() == 0);
    m.step();
    CHECK(m.a() == 17);
}

TEST_CASE("XCH swaps A and B", "[instructions]") {
    auto m = test::load("LDI 1\nXCH\nLDI 2\nXCH\nHLT\n");
    test::steps(m, 2);
    CHECK(m.a() == 0);
    CHECK(m.b() == 1);
    test::steps(m, 2);
    CHECK(m.a() == 1);
    CHECK(m.b() == 2);
}

TEST_CASE("ADD adds B to A and sets Z and V", "[instructions]") {
    auto m = test::load("LDI 5\nXCH\nLDI -5\nADD\nADD\nHLT\n");
    test::steps(m, 4);
    CHECK(m.a() == 0);
    CHECK(m.b() == 5);
    CHECK(m.z());
    CHECK_FALSE(m.v());
    m.step();
    CHECK(m.a() == 5);
    CHECK_FALSE(m.z());
}

TEST_CASE("ADD overflow: INT32_MAX + 1", "[instructions]") {
    auto m = test::load("LDI 1\nXCH\nLDI 2147483647\nADD\nHLT\n");
    test::steps(m, 4);
    CHECK(m.a() == -2147483647 - 1);
    CHECK(m.v());
    CHECK_FALSE(m.z());
}

TEST_CASE("ADD: INT32_MIN + INT32_MIN gives 0 with Z and V", "[instructions]") {
    auto m = test::load("LDI -2147483648\nXCH\nLDI -2147483648\nADD\nHLT\n");
    test::steps(m, 4);
    CHECK(m.a() == 0);
    CHECK(m.z());
    CHECK(m.v());
}

TEST_CASE("Only ADD changes Z and V", "[instructions]") {
    auto m = test::load("DEC X\nLDI 0\nXCH\nLDI 0\nADD\nLDI 9\nSTR X\nLDA X\nLDB X\nXCH\nJMP 11\nHLT\n");
    test::steps(m, 5);
    REQUIRE(m.z());
    test::steps(m, 6);
    CHECK(m.z());
    CHECK_FALSE(m.v());
}

TEST_CASE("JMP jumps", "[instructions]") {
    auto m = test::load("JMP 2\nHLT\nLDI 3\nHLT\n");
    m.step();
    CHECK(m.pc() == 2);
}

TEST_CASE("JZS jumps only if Z is set", "[instructions]") {
    auto not_taken = test::load("JZS 2\nHLT\nHLT\n");
    not_taken.step();
    CHECK(not_taken.pc() == 1);

    auto taken = test::load("LDI 0\nADD\nJZS 4\nHLT\nHLT\n");
    test::steps(taken, 3);
    CHECK(taken.pc() == 4);
}

TEST_CASE("JVS jumps only if V is set", "[instructions]") {
    auto not_taken = test::load("JVS 2\nHLT\nHLT\n");
    not_taken.step();
    CHECK(not_taken.pc() == 1);

    auto taken = test::load("LDI 2147483647\nXCH\nLDI 1\nADD\nJVS 6\nHLT\nHLT\n");
    test::steps(taken, 5);
    CHECK(taken.pc() == 6);
}

TEST_CASE("HLT halts and keeps the PC", "[instructions]") {
    auto m = test::load("LDI 1\nHLT\nLDI 2\n");
    test::steps(m, 2);
    CHECK(m.halted());
    CHECK(m.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(m.pc() == 1);
    CHECK(m.a() == 1);
}

TEST_CASE("Instructions describe themselves", "[instructions]") {
    const auto m = test::load("DEC X\nLDA X\nLDB X\nLDI -3\nSTR X\nXCH\nJMP 0\nJZS 1\nJVS 2\nADD\nHLT\n");
    auto text = [&](std::size_t address) { return m.program().at(address).instruction->describe(); };
    CHECK(text(0) == "Declare symbol X (address 128)");
    CHECK(text(1) == "Load A from X (address 128)");
    CHECK(text(2) == "Load B from X (address 128)");
    CHECK(text(3) == "Load the value -3 into A");
    CHECK(text(4) == "Store A into X (address 128)");
    CHECK(text(5) == "Exchange A and B");
    CHECK(text(6) == "Jump to address 0");
    CHECK(text(7) == "Jump to address 1 if Z is set");
    CHECK(text(8) == "Jump to address 2 if V is set");
    CHECK(text(9) == "Add B to A; set Z and V");
    CHECK(text(10) == "Halt the machine");
    CHECK(m.program().at(3).instruction->operand() == "-3");
    CHECK(m.program().at(5).instruction->operand().empty());
}
