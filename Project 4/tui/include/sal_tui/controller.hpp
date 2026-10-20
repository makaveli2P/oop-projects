// controller.hpp - the state and the commands of the visualizer.
//
// The controller has no FTXUI code and no threads. The views read it, and the
// key handler calls its commands. The tests drive it without a terminal.
//
// The controller shows one instruction at a time, one micro-operation at a
// time. To do this, it executes the complete instruction on a sal::Machine,
// keeps a snapshot of the machine before the instruction, and applies the
// first k micro-operations of the trace to a copy of that snapshot. The copy
// is the "view state" that the screen shows.
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "sal/machine.hpp"
#include "sal/memory.hpp"
#include "sal/micro_op.hpp"
#include "sal/program.hpp"
#include "sal_tui/format.hpp"

namespace sal::tui {

/// The maximum number of instructions that the back command can undo.
inline constexpr std::size_t kDefaultHistoryLimit = 4096;

/// Applies one micro-operation to a machine state. The result is the state
/// that the hardware has after that micro-operation.
void apply_micro_op(sal::MachineState& state, const sal::MicroOp& op);

/// The result of the run-to-end command.
struct RunToEndResult {
    enum class Stop : std::uint8_t { Halted, Breakpoint, StepLimit, AlreadyHalted };
    std::size_t steps = 0;  ///< The number of instructions that started.
    Stop stop = Stop::Halted;
};

class Controller {
public:
    explicit Controller(std::shared_ptr<const sal::Program> program,
                        std::size_t history_limit = kDefaultHistoryLimit);

    // ---- Commands ------------------------------------------------------------

    /// Shows the next micro-operation. At the end of an instruction, it
    /// executes the next instruction and shows its first micro-operation.
    void next_micro_op();
    /// Completes the current instruction. If it is complete, executes the
    /// next instruction and shows all of its micro-operations.
    void step_instruction();
    /// Goes back to the state after the previous instruction.
    void back();
    /// Puts the machine back in the state after load. Keeps the breakpoints.
    void reset();
    /// Executes complete instructions until a halt, a breakpoint or `limit`.
    RunToEndResult run_to_end(std::size_t limit = sal::kDefaultStepLimit);

    /// Starts or pauses the animated run.
    void toggle_run();
    void pause() noexcept { running_ = false; }
    [[nodiscard]] bool running() const noexcept { return running_; }
    /// One animation tick: the run shows one more micro-operation, or stops.
    /// Returns false if the run is paused.
    bool tick();

    /// The animation speed. Level 0 is the slowest.
    void faster() noexcept;
    void slower() noexcept;
    [[nodiscard]] std::size_t speed_level() const noexcept { return speed_; }
    [[nodiscard]] static std::size_t speed_level_count() noexcept;
    [[nodiscard]] std::chrono::milliseconds tick_interval() const noexcept;

    void toggle_breakpoint(std::uint8_t address);
    [[nodiscard]] bool has_breakpoint(std::uint8_t address) const noexcept {
        return breakpoints_.count(address) != 0;
    }
    [[nodiscard]] const std::set<std::uint8_t>& breakpoints() const noexcept { return breakpoints_; }

    /// The selected source line. Enter toggles its breakpoint.
    [[nodiscard]] std::uint8_t selection() const noexcept { return selection_; }
    void move_selection(int delta) noexcept;
    void select(std::uint8_t address) noexcept;

    void toggle_base() noexcept;
    [[nodiscard]] NumberBase base() const noexcept { return base_; }

    // ---- Observation ---------------------------------------------------------

    [[nodiscard]] const sal::Program& program() const noexcept { return machine_.program(); }
    /// The state that the screen shows: after the visible micro-operations.
    [[nodiscard]] const sal::MachineState& state() const noexcept { return view_; }
    /// The state before the current instruction. The views compare it with
    /// state() to highlight the values that changed in this step.
    [[nodiscard]] const sal::MachineState& previous_state() const noexcept { return previous_; }

    /// True when an instruction is on the screen (executed or partly shown).
    [[nodiscard]] bool has_instruction() const noexcept { return !history_.empty(); }
    /// The address of the instruction on the screen.
    [[nodiscard]] std::optional<std::uint8_t> instruction_address() const noexcept;
    /// The micro-operations of the instruction on the screen. Empty if none.
    [[nodiscard]] const std::vector<sal::MicroOp>& micro_ops() const noexcept;
    /// The number of micro-operations on the screen (0 to micro_ops().size()).
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }
    /// The last visible micro-operation, or nullptr.
    [[nodiscard]] const sal::MicroOp* active_micro_op() const noexcept;
    [[nodiscard]] bool instruction_complete() const noexcept;
    /// The 1-based number of the instruction on the screen. 0 if none.
    [[nodiscard]] std::uint64_t step_number() const noexcept;

    [[nodiscard]] bool halted() const noexcept { return view_.halt_reason.has_value(); }
    [[nodiscard]] std::optional<sal::HaltReason> halt_reason() const noexcept { return view_.halt_reason; }

    /// A short sentence about the last command, for example a breakpoint stop.
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] std::size_t history_size() const noexcept { return history_.size(); }

private:
    struct Entry {
        sal::Machine before;    ///< The machine before the instruction.
        sal::StepResult step;   ///< The trace of the instruction.
    };

    bool begin_instruction();
    void update_view();
    void set_halt_message();
    [[nodiscard]] bool at_boundary() const noexcept;

    sal::Machine machine_;      ///< The machine after the last executed instruction.
    std::deque<Entry> history_;
    std::size_t history_limit_;
    std::size_t cursor_ = 0;

    sal::MachineState view_;
    sal::MachineState previous_;

    bool running_ = false;
    bool run_started_ = false;  ///< True from a run start at a boundary until its first instruction starts.
    std::size_t speed_ = 3;
    std::set<std::uint8_t> breakpoints_;
    std::uint8_t selection_ = 0;
    NumberBase base_ = NumberBase::Decimal;
    std::string message_;
};

}  // namespace sal::tui
