// Golden tests: the sample programs in programs/ and their exact final state.
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "helpers.hpp"

namespace {

sal::Machine load_program(const std::string& name) {
    const auto result = sal::assemble_file(std::string{SAL_PROGRAMS_DIR} + "/" + name);
    if (!result.ok()) {
        for (const auto& e : result.errors()) UNSCOPED_INFO(sal::to_string(e));
    }
    REQUIRE(result.ok());
    return sal::Machine{result.program()};
}

}  // namespace

TEST_CASE("test1.sal: repeated addition ends by overflow", "[programs]") {
    auto m = load_program("test1.sal");
    const auto result = m.run();
    CHECK(result.stop == sal::RunResult::Stop::Halted);
    CHECK(result.steps == 20);
    CHECK(m.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(m.pc() == 12);
    CHECK(m.a() == -1293967296);  // 3001000000 wrapped to 32 bits.
    CHECK(m.b() == 1000000000);
    CHECK(m.v());
    CHECK_FALSE(m.z());
    CHECK(m.value_of("INITIAL") == 2001000000);
    CHECK(m.value_of("ADDER") == 1000000000);
}

TEST_CASE("test2.sal: 342 / 3 by repeated addition of -3", "[programs]") {
    auto m = load_program("test2.sal");
    // The program needs 1154 steps, more than one block of the default limit.
    const auto first = m.run();
    CHECK(first.stop == sal::RunResult::Stop::StepLimit);
    CHECK(first.steps == 1000);
    const auto second = m.run();
    CHECK(second.stop == sal::RunResult::Stop::Halted);
    CHECK(second.steps == 154);
    CHECK(m.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(m.pc() == 22);
    CHECK(m.value_of("RESULT") == 114);
    CHECK(m.value_of("INITIAL") == 0);
    CHECK(m.value_of("SUB") == -3);
    CHECK(m.value_of("INCREMENTOR") == 1);
    CHECK(m.a() == 0);
    CHECK(m.b() == 3);
    CHECK(m.z());
    CHECK_FALSE(m.v());
}

TEST_CASE("test3.sal: Fibonacci loop, NTHTERM counts down from 43", "[programs]") {
    // Each loop pass does (PREV, RESULT) <- (RESULT, PREV + RESULT), starting
    // at (F0, F1) = (0, 1). The loop runs 42 times (NTHTERM 42 down to 1), so
    // PREV = F42 = 267914296 and RESULT = F43 = 433494437. No ADD overflows.
    // Steps: 12 set-up + 42 * 12 loop + 6 exit (LDA, LDB, ADD, STR, JZS, HLT).
    auto m = load_program("test3.sal");
    const auto result = m.run();
    CHECK(result.stop == sal::RunResult::Stop::Halted);
    CHECK(result.steps == 522);
    CHECK(m.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(m.pc() == 24);
    CHECK(m.value_of("PREV") == 267914296);
    CHECK(m.value_of("RESULT") == 433494437);
    CHECK(m.value_of("NTHTERM") == 0);
    CHECK(m.value_of("DECREMENTOR") == -1);
    CHECK(m.a() == 0);
    CHECK(m.b() == -1);
    CHECK(m.z());
    CHECK_FALSE(m.v());
}

TEST_CASE("countdown.sal: counts 5 down to 0 with JZS", "[programs]") {
    auto m = load_program("countdown.sal");
    m.run();
    CHECK(m.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(m.value_of("COUNT") == 0);
    CHECK(m.value_of("LOOPS") == 5);
    CHECK(m.z());
}

TEST_CASE("overflow.sal: INT32_MAX + 1 sets V and JVS jumps", "[programs]") {
    auto m = load_program("overflow.sal");
    m.run();
    CHECK(m.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(m.value_of("SUM") == -2147483647 - 1);
    CHECK(m.value_of("FLAG") == 1);
    CHECK(m.v());
}

TEST_CASE("swap.sal: XCH and comments", "[programs]") {
    auto m = load_program("swap.sal");
    m.run();
    CHECK(m.halt_reason() == sal::HaltReason::RanPastEnd);
    CHECK(m.value_of("FIRST") == 20);
    CHECK(m.value_of("SECOND") == 10);
}
