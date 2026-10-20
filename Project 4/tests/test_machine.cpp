#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <utility>

#include "helpers.hpp"

TEST_CASE("A new machine holds the program in code memory", "[machine]") {
    const auto m = test::load("DEC X\nLDI 1\nHLT\n");
    CHECK(m.pc() == 0);
    CHECK_FALSE(m.halted());
    CHECK(m.steps() == 0);
    REQUIRE(std::holds_alternative<sal::CodeCell>(m.cell(0)));
    CHECK(std::get<sal::CodeCell>(m.cell(2)).line->text == "HLT");
    CHECK(std::holds_alternative<sal::EmptyCell>(m.cell(3)));
    CHECK(std::holds_alternative<sal::EmptyCell>(m.cell(127)));
    CHECK(m.data(128) == 0);
    CHECK(m.data(255) == 0);
    CHECK_FALSE(m.data(0).has_value());
    CHECK_FALSE(m.value_of("Y").has_value());
}

TEST_CASE("The machine halts when the PC moves past the last instruction", "[machine]") {
    auto m = test::load("LDI 1\nLDI 2\n");
    m.step();
    CHECK_FALSE(m.halted());
    const auto last = m.step();
    CHECK(m.halted());
    CHECK(m.halt_reason() == sal::HaltReason::RanPastEnd);
    CHECK(m.pc() == 2);
    REQUIRE(std::holds_alternative<sal::Halt>(last.micro_ops.back()));
    CHECK(std::get<sal::Halt>(last.micro_ops.back()).reason == sal::HaltReason::RanPastEnd);
}

TEST_CASE("A jump to the last instruction + 1 is a load error, not a runtime problem",
          "[machine]") {
    CHECK_FALSE(sal::assemble("JMP 1\n").ok());
}

TEST_CASE("Stepping a halted machine does nothing", "[machine]") {
    auto m = test::load("HLT\n");
    REQUIRE(m.step().executed());
    const auto snapshot = m.state();
    const auto again = m.step();
    CHECK(again.status == sal::StepResult::Status::AlreadyHalted);
    CHECK(again.micro_ops.empty());
    CHECK(m.steps() == 1);
    CHECK(m.pc() == snapshot.pc);
    CHECK(m.run().steps == 0);
}

TEST_CASE("run stops at the step limit and can continue", "[machine]") {
    auto m = test::load("JMP 0\n");
    const auto first = m.run(10);
    CHECK(first.steps == 10);
    CHECK(first.stop == sal::RunResult::Stop::StepLimit);
    CHECK_FALSE(m.halted());
    CHECK(m.steps() == 10);
    const auto second = m.run();
    CHECK(second.steps == sal::kDefaultStepLimit);
    CHECK(m.steps() == 10 + sal::kDefaultStepLimit);
}

TEST_CASE("run stops at a halt", "[machine]") {
    auto m = test::load("LDI 1\nLDI 2\nHLT\nLDI 3\n");
    const auto result = m.run();
    CHECK(result.steps == 3);
    CHECK(result.stop == sal::RunResult::Stop::Halted);
    CHECK(m.a() == 2);
}

TEST_CASE("A machine copy is an independent snapshot", "[machine]") {
    auto m = test::load("DEC X\nLDI 5\nSTR X\nXCH\nLDI -5\nADD\nHLT\n");
    test::steps(m, 2);
    const sal::Machine snapshot = m;  // Copy before STR.
    m.run();

    CHECK(m.halted());
    CHECK(m.value_of("X") == 5);
    CHECK(m.z());

    CHECK_FALSE(snapshot.halted());
    CHECK(snapshot.pc() == 2);
    CHECK(snapshot.a() == 5);
    CHECK(snapshot.value_of("X") == 0);
    CHECK_FALSE(snapshot.z());
    CHECK(&snapshot.program() == &m.program());  // The program is shared, not copied.

    sal::Machine resumed = snapshot;
    resumed.run();
    CHECK(resumed.state().memory == m.state().memory);
    CHECK(resumed.a() == m.a());
}

TEST_CASE("reset puts the machine back in the state after load", "[machine]") {
    auto m = test::load("DEC X\nLDI 5\nSTR X\nHLT\n");
    const auto fresh = m.state();
    m.run();
    m.reset();
    CHECK(m.state().memory == fresh.memory);
    CHECK(m.pc() == 0);
    CHECK(m.a() == 0);
    CHECK_FALSE(m.halted());
    CHECK(m.steps() == 0);
}

namespace {

/// Makes a code line that holds `instruction`.
template <class I, class... Args>
sal::CodeLine line_of(Args&&... args) {
    return sal::CodeLine{std::make_unique<const I>(std::forward<Args>(args)...), "test", 1};
}

std::vector<sal::CodeLine> xch_lines(std::size_t count) {
    std::vector<sal::CodeLine> code;
    for (std::size_t i = 0; i < count; ++i) {
        code.push_back(line_of<sal::Xch>());
    }
    return code;
}

}  // namespace

TEST_CASE("The Program constructor enforces its invariants", "[machine][program]") {
    using sal::Program;
    using sal::Symbol;
    CHECK_THROWS_AS(Program(xch_lines(0), {}), std::invalid_argument);
    CHECK_THROWS_AS(Program(xch_lines(sal::kCodeSize + 1), {}), std::invalid_argument);
    CHECK_NOTHROW(Program(xch_lines(sal::kCodeSize), {}));

    std::vector<sal::CodeLine> with_null;
    with_null.push_back(sal::CodeLine{nullptr, "null", 1});
    CHECK_THROWS_AS(Program(std::move(with_null), {}), std::invalid_argument);

    // Symbol i must be at address 128 + i, and each name must be unique.
    CHECK_NOTHROW(Program(xch_lines(1), {Symbol{"X", 128}, Symbol{"Y", 129}}));
    CHECK_THROWS_AS(Program(xch_lines(1), {Symbol{"X", 129}}), std::invalid_argument);
    CHECK_THROWS_AS(Program(xch_lines(1), {Symbol{"X", 128}, Symbol{"X", 129}}),
                    std::invalid_argument);
    CHECK_THROWS_AS(Program(xch_lines(1), {Symbol{"", 128}}), std::invalid_argument);

    std::vector<Symbol> too_many;
    for (std::size_t i = 0; i <= sal::kDataSize; ++i) {
        too_many.push_back(Symbol{std::string(i + 1, 'S'), static_cast<std::uint8_t>(128 + (i % 128))});
    }
    CHECK_THROWS_AS(Program(xch_lines(1), std::move(too_many)), std::invalid_argument);
}

TEST_CASE("A runtime fault stops the instruction and records the access", "[machine]") {
    // The assembler cannot make this program. The public API can: LDA and STR
    // with a code address (5 is not a data address).
    auto make = [](auto line) {
        std::vector<sal::CodeLine> code;
        code.push_back(line_of<sal::Ldi>(42));
        code.push_back(std::move(line));
        code.push_back(line_of<sal::Hlt>());
        return sal::Machine{std::make_shared<const sal::Program>(std::move(code), std::vector<sal::Symbol>{})};
    };

    SECTION("LDA does not write A") {
        sal::Machine m = make(line_of<sal::Lda>("X", std::uint8_t{1}));
        m.step();
        const sal::StepResult fault = m.step();
        CHECK(m.state().a == 42);
        CHECK(m.halt_reason() == sal::HaltReason::RuntimeFault);
        CHECK(m.pc() == 1);
        CHECK(test::kinds(fault) == std::vector<std::string>{"Fetch", "Decode", "PcWrite", "Halt"});
        const auto& halt = std::get<sal::Halt>(fault.micro_ops.back());
        REQUIRE(halt.fault_address.has_value());
        CHECK(*halt.fault_address == 1);
        CHECK(sal::describe(halt) == "Halt: runtime fault (address 1 is not a data address)");
    }
    SECTION("LDB does not write B") {
        sal::Machine m = make(line_of<sal::Ldb>("X", std::uint8_t{0}));
        m.step();
        const sal::StepResult fault = m.step();
        CHECK(m.state().b == 0);
        CHECK(test::kinds(fault) == std::vector<std::string>{"Fetch", "Decode", "PcWrite", "Halt"});
    }
    SECTION("STR reads A and writes no memory") {
        sal::Machine m = make(line_of<sal::Str>("X", std::uint8_t{2}));
        const sal::MachineState before = m.state();
        m.step();
        const sal::StepResult fault = m.step();
        CHECK(m.state().memory == before.memory);
        CHECK(test::kinds(fault) ==
              std::vector<std::string>{"Fetch", "Decode", "RegRead", "PcWrite", "Halt"});
        CHECK(std::get<sal::Halt>(fault.micro_ops.back()).fault_address == std::uint8_t{2});
    }
}
