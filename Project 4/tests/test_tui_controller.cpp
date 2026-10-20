// Tests for the visualizer controller: the micro-operation cursor, the
// history, the breakpoints, the animated run and the run-to-end limit.
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <set>
#include <string>

#include "helpers.hpp"
#include "sal_tui/controller.hpp"
#include "sal_tui/keys.hpp"

using sal::tui::Controller;
using sal::tui::KeyResult;
using sal::tui::RunToEndResult;

namespace {

/// Compares the visible state with a reference machine.
void require_same(const sal::MachineState& view, const sal::Machine& reference) {
    const sal::MachineState& ref = reference.state();
    REQUIRE(view.a == ref.a);
    REQUIRE(view.b == ref.b);
    REQUIRE(view.pc == ref.pc);
    REQUIRE(view.z == ref.z);
    REQUIRE(view.v == ref.v);
    REQUIRE(view.halt_reason == ref.halt_reason);
    REQUIRE(view.memory == ref.memory);
}

/// Runs the animation until it pauses. Returns the number of ticks.
int run_until_pause(Controller& c, int max_ticks = 100000) {
    int ticks = 0;
    while (c.running() && ticks < max_ticks) {
        c.tick();
        ++ticks;
    }
    return ticks;
}

}  // namespace

TEST_CASE("A new controller shows the state after load", "[tui][controller]") {
    Controller c{test::program("LDI 5\nHLT\n")};
    CHECK_FALSE(c.has_instruction());
    CHECK(c.cursor() == 0);
    CHECK(c.active_micro_op() == nullptr);
    CHECK(c.step_number() == 0);
    CHECK(c.micro_ops().empty());
    CHECK(c.state().pc == 0);
    CHECK_FALSE(c.halted());
}

TEST_CASE("next_micro_op reveals one micro-operation at a time", "[tui][controller]") {
    Controller c{test::program("LDI 5\nHLT\n")};
    // LDI: Fetch, Decode, RegWrite A, PcWrite.
    c.next_micro_op();
    CHECK(c.step_number() == 1);
    CHECK(c.cursor() == 1);
    CHECK(c.micro_ops().size() == 4);
    REQUIRE(c.active_micro_op() != nullptr);
    CHECK(std::holds_alternative<sal::Fetch>(*c.active_micro_op()));
    CHECK(c.state().a == 0);

    c.next_micro_op();  // Decode.
    CHECK(c.state().a == 0);
    c.next_micro_op();  // RegWrite A.
    CHECK(c.state().a == 5);
    CHECK(c.state().pc == 0);  // The PC changes only in the last micro-operation.
    CHECK(c.previous_state().a == 0);
    CHECK_FALSE(c.instruction_complete());

    c.next_micro_op();  // PcWrite.
    CHECK(c.state().pc == 1);
    CHECK(c.instruction_complete());

    c.next_micro_op();  // The next instruction starts: HLT.
    CHECK(c.step_number() == 2);
    CHECK(c.cursor() == 1);
    CHECK(*c.instruction_address() == 1);
    CHECK(c.previous_state().a == 5);
    CHECK_FALSE(c.halted());  // The Halt micro-operation is not visible yet.

    c.step_instruction();
    CHECK(c.halted());
    CHECK(c.halt_reason() == sal::HaltReason::HltExecuted);

    c.next_micro_op();  // Nothing more to execute.
    CHECK(c.step_number() == 2);
    CHECK(c.message().find("halted") != std::string::npos);
}

TEST_CASE("The visible state matches the machine at each instruction end", "[tui][controller]") {
    const auto program = test::sample("test3.sal");
    Controller c{program};
    sal::Machine reference{program};
    while (!reference.halted()) {
        reference.step();
        // Show the instruction one micro-operation at a time.
        do {
            c.next_micro_op();
        } while (!c.instruction_complete());
        require_same(c.state(), reference);
    }
    CHECK(c.step_number() == 522);
}

TEST_CASE("step_instruction completes the instruction on the screen", "[tui][controller]") {
    Controller c{test::program("LDI 1\nLDI 2\nHLT\n")};
    c.next_micro_op();
    c.step_instruction();  // Completes LDI 1; does not start LDI 2.
    CHECK(c.step_number() == 1);
    CHECK(c.instruction_complete());
    CHECK(c.state().a == 1);
    c.step_instruction();  // Executes LDI 2.
    CHECK(c.step_number() == 2);
    CHECK(c.state().a == 2);
    CHECK(c.cursor() == c.micro_ops().size());
}

TEST_CASE("back restores the state after the previous instruction", "[tui][controller]") {
    const auto program = test::program("LDI 1\nLDI 2\nLDI 3\nHLT\n");
    Controller c{program};

    c.back();
    CHECK(c.message() == "This is the start of the program.");

    c.step_instruction();
    c.step_instruction();
    c.step_instruction();
    CHECK(c.state().a == 3);

    c.back();
    CHECK(c.state().a == 2);
    CHECK(c.state().pc == 2);
    CHECK(c.step_number() == 2);
    CHECK(c.instruction_complete());

    // Back in the middle of an instruction: the partial instruction goes away.
    c.next_micro_op();
    c.next_micro_op();
    c.back();
    CHECK(c.state().a == 2);
    CHECK(c.step_number() == 2);

    c.back();
    c.back();
    CHECK_FALSE(c.has_instruction());
    CHECK(c.state().a == 0);
    CHECK(c.state().pc == 0);

    // Forward again gives the same result.
    c.step_instruction();
    CHECK(c.state().a == 1);
}

TEST_CASE("back can undo halt", "[tui][controller]") {
    Controller c{test::program("LDI 7\n")};
    c.step_instruction();
    CHECK(c.halt_reason() == sal::HaltReason::RanPastEnd);
    c.back();
    CHECK_FALSE(c.halted());
    c.step_instruction();
    CHECK(c.halted());
}

TEST_CASE("The history keeps only the newest instructions", "[tui][controller]") {
    Controller c{test::program("LDI 1\nLDI 2\nLDI 3\nLDI 4\nLDI 5\nHLT\n"), 3};
    for (int i = 0; i < 5; ++i) c.step_instruction();
    CHECK(c.history_size() == 3);
    c.back();
    c.back();
    c.back();
    CHECK(c.state().a == 2);  // The state after the second instruction.
    c.back();
    CHECK(c.message() == "The history holds no earlier steps.");
    CHECK(c.state().a == 2);
}

TEST_CASE("reset goes back to the start and keeps the breakpoints", "[tui][controller]") {
    Controller c{test::sample("test1.sal")};
    c.toggle_breakpoint(7);
    c.run_to_end();
    c.reset();
    CHECK_FALSE(c.has_instruction());
    CHECK_FALSE(c.halted());
    CHECK(c.state().a == 0);
    CHECK(c.has_breakpoint(7));
    CHECK(c.history_size() == 0);
}

TEST_CASE("Breakpoints toggle only on instruction addresses", "[tui][controller]") {
    Controller c{test::program("LDI 1\nHLT\n")};
    c.toggle_breakpoint(1);
    CHECK(c.has_breakpoint(1));
    c.toggle_breakpoint(1);
    CHECK_FALSE(c.has_breakpoint(1));
    c.toggle_breakpoint(2);    // Past the end of the program.
    c.toggle_breakpoint(200);  // Data space.
    CHECK(c.breakpoints().empty());
}

TEST_CASE("The animated run stops at a breakpoint before its instruction", "[tui][controller]") {
    Controller c{test::sample("test1.sal")};
    c.toggle_breakpoint(7);  // ADD
    c.toggle_run();
    CHECK(c.running());
    run_until_pause(c);
    CHECK_FALSE(c.running());
    CHECK(c.state().pc == 7);
    CHECK(c.step_number() == 7);  // Addresses 0-6 executed; ADD did not start.
    CHECK(c.instruction_complete());
    CHECK(c.message() == "Stopped at the breakpoint at address 7.");

    // The next run starts at the breakpoint, so it does not stop there at once.
    // It stops at the next visit of address 7 (after JMP 7).
    c.toggle_run();
    run_until_pause(c);
    CHECK(c.state().pc == 7);
    CHECK(c.step_number() == 12);

    // Without the breakpoint, the run continues to the halt.
    c.toggle_breakpoint(7);
    c.toggle_run();
    run_until_pause(c);
    CHECK(c.halt_reason() == sal::HaltReason::HltExecuted);
    CHECK(c.state().a == -1293967296);
    CHECK(c.state().v);
    CHECK(c.step_number() == 20);
}

TEST_CASE("Each tick of the run shows one micro-operation", "[tui][controller]") {
    Controller c{test::program("LDI 5\nHLT\n")};
    c.toggle_run();
    c.tick();
    CHECK(c.cursor() == 1);
    c.tick();
    CHECK(c.cursor() == 2);
    c.pause();
    CHECK_FALSE(c.tick());
    CHECK(c.cursor() == 2);
}

TEST_CASE("The run stops when the machine runs past the end", "[tui][controller]") {
    Controller c{test::sample("swap.sal")};
    c.toggle_run();
    run_until_pause(c);
    CHECK(c.halt_reason() == sal::HaltReason::RanPastEnd);
    CHECK(c.message().find("ran past end of program") != std::string::npos);
    // A halted machine does not start a new run.
    c.toggle_run();
    CHECK_FALSE(c.running());
}

TEST_CASE("run_to_end respects the step limit and offers to continue", "[tui][controller]") {
    Controller c{test::sample("test2.sal")};
    const RunToEndResult first = c.run_to_end();
    CHECK(first.stop == RunToEndResult::Stop::StepLimit);
    CHECK(first.steps == 1000);
    CHECK_FALSE(c.halted());
    CHECK(c.message().find("Press g to run 1000 more") != std::string::npos);

    const RunToEndResult second = c.run_to_end();
    CHECK(second.stop == RunToEndResult::Stop::Halted);
    CHECK(second.steps == 154);
    CHECK(c.step_number() == 1154);
    CHECK(c.halt_reason() == sal::HaltReason::HltExecuted);

    const sal::Program& program = c.program();
    CHECK(std::get<sal::DataCell>(c.state().memory[*program.address_of("RESULT")]).value == 114);
    CHECK(std::get<sal::DataCell>(c.state().memory[*program.address_of("INITIAL")]).value == 0);

    CHECK(c.run_to_end().stop == RunToEndResult::Stop::AlreadyHalted);
}

TEST_CASE("run_to_end completes a partial instruction and stops at breakpoints", "[tui][controller]") {
    Controller c{test::sample("test1.sal")};
    c.next_micro_op();
    c.toggle_breakpoint(8);  // JVS
    const RunToEndResult result = c.run_to_end();
    CHECK(result.stop == RunToEndResult::Stop::Breakpoint);
    CHECK(c.state().pc == 8);
    CHECK(c.instruction_complete());
    CHECK(c.step_number() == 8);
}

TEST_CASE("A run that starts mid-instruction stops at the next breakpoint", "[tui][controller]") {
    // The run completes instruction 0. Then the breakpoint at address 1 must
    // stop it, because the run did not start at address 1.
    const auto source = "LDI 1\nLDI 2\nLDI 3\nHLT\n";

    SECTION("animated run") {
        Controller c{test::program(source)};
        c.next_micro_op();
        c.toggle_breakpoint(1);
        c.toggle_run();
        run_until_pause(c);
        CHECK_FALSE(c.halted());
        CHECK(c.state().pc == 1);
        CHECK(c.instruction_complete());
        CHECK(c.message() == "Stopped at the breakpoint at address 1.");
    }
    SECTION("run_to_end") {
        Controller c{test::program(source)};
        c.next_micro_op();
        c.toggle_breakpoint(1);
        const RunToEndResult result = c.run_to_end();
        CHECK(result.stop == RunToEndResult::Stop::Breakpoint);
        CHECK(result.steps == 0);
        CHECK(c.state().pc == 1);
        CHECK(c.instruction_complete());
    }
    SECTION("a run that starts at the boundary still passes its own breakpoint") {
        Controller c{test::program(source)};
        c.step_instruction();
        c.toggle_breakpoint(1);
        c.toggle_run();
        run_until_pause(c);
        CHECK(c.halt_reason() == sal::HaltReason::HltExecuted);
        CHECK(c.run_to_end().stop == RunToEndResult::Stop::AlreadyHalted);
        c.reset();
        c.step_instruction();
        CHECK(c.run_to_end().stop == RunToEndResult::Stop::Halted);
    }
}

TEST_CASE("The speed level stays in range", "[tui][controller]") {
    Controller c{test::program("HLT\n")};
    const auto start = c.tick_interval();
    c.faster();
    CHECK(c.tick_interval() < start);
    for (std::size_t i = 0; i < 20; ++i) c.faster();
    CHECK(c.speed_level() == Controller::speed_level_count() - 1);
    for (std::size_t i = 0; i < 20; ++i) c.slower();
    CHECK(c.speed_level() == 0);
    CHECK(c.tick_interval().count() == 1000);
}

TEST_CASE("Each speed level has its own correct label", "[tui][controller]") {
    using namespace std::chrono_literals;
    CHECK(sal::tui::speed_text(1000ms) == "1");
    CHECK(sal::tui::speed_text(600ms) == "1.7");
    CHECK(sal::tui::speed_text(350ms) == "2.9");
    CHECK(sal::tui::speed_text(200ms) == "5");
    CHECK(sal::tui::speed_text(120ms) == "8.3");
    CHECK(sal::tui::speed_text(70ms) == "14");
    CHECK(sal::tui::speed_text(20ms) == "50");

    Controller c{test::program("HLT\n")};
    for (std::size_t i = 0; i < Controller::speed_level_count(); ++i) c.slower();
    std::set<std::string> labels;
    for (std::size_t i = 0; i < Controller::speed_level_count(); ++i) {
        labels.insert(sal::tui::speed_text(c.tick_interval()));
        c.faster();
    }
    CHECK(labels.size() == Controller::speed_level_count());
}

TEST_CASE("The keys map to the commands", "[tui][keys]") {
    Controller c{test::program("LDI 1\nLDI 2\nHLT\n")};
    CHECK(handle_key(c, "n") == KeyResult::Handled);
    CHECK(c.cursor() == 1);
    CHECK(handle_key(c, " ") == KeyResult::Handled);
    CHECK(c.cursor() == 2);
    handle_key(c, "s");
    CHECK(c.instruction_complete());
    handle_key(c, "b");
    CHECK_FALSE(c.has_instruction());

    handle_key(c, "j");
    CHECK(c.selection() == 1);
    handle_key(c, "Enter");
    CHECK(c.has_breakpoint(1));
    handle_key(c, "Down");
    handle_key(c, "Down");  // Stays at the last instruction.
    CHECK(c.selection() == 2);
    handle_key(c, "Up");
    CHECK(c.selection() == 1);

    handle_key(c, "x");
    CHECK(c.base() == sal::tui::NumberBase::Hex);
    handle_key(c, "r");
    CHECK(c.running());
    handle_key(c, "r");
    CHECK_FALSE(c.running());
    handle_key(c, "g");
    CHECK(c.state().pc == 1);  // Stopped at the breakpoint.
    handle_key(c, "R");
    CHECK_FALSE(c.has_instruction());

    CHECK(handle_key(c, "?") == KeyResult::Ignored);
    CHECK(handle_key(c, "q") == KeyResult::Quit);
    CHECK(handle_key(c, "Escape") == KeyResult::Quit);
}

TEST_CASE("apply_micro_op changes only what the micro-operation writes", "[tui][controller]") {
    sal::MachineState state;
    state.memory[130] = sal::DataCell{0};
    sal::tui::apply_micro_op(state, sal::RegWrite{sal::Register::B, 0, 9});
    sal::tui::apply_micro_op(state, sal::FlagWrite{sal::Flag::V, false, true});
    sal::tui::apply_micro_op(state, sal::MemWrite{130, std::string{"X"}, 0, -4});
    sal::tui::apply_micro_op(state, sal::PcWrite{0, 5, sal::PcCause::Jump});
    sal::tui::apply_micro_op(state, sal::MemRead{130, std::nullopt, -4});
    CHECK(state.b == 9);
    CHECK(state.a == 0);
    CHECK(state.v);
    CHECK_FALSE(state.z);
    CHECK(std::get<sal::DataCell>(state.memory[130]).value == -4);
    CHECK(state.pc == 5);
    CHECK_FALSE(state.halt_reason);
    sal::tui::apply_micro_op(state, sal::Halt{sal::HaltReason::RanPastEnd});
    CHECK(state.halt_reason == sal::HaltReason::RanPastEnd);
}
