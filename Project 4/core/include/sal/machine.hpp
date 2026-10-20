// machine.hpp - the SAL machine: state, single step and run.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "sal/memory.hpp"
#include "sal/micro_op.hpp"
#include "sal/program.hpp"

namespace sal {

/// The result of one call to Machine::step().
struct StepResult {
    enum class Status : std::uint8_t {
        Executed,       ///< One instruction executed.
        AlreadyHalted,  ///< The machine was halted. Nothing changed.
    };
    Status status = Status::AlreadyHalted;
    std::uint8_t address = 0;         ///< The address of the instruction.
    std::vector<MicroOp> micro_ops;   ///< The trace, in order. Empty if AlreadyHalted.

    [[nodiscard]] bool executed() const noexcept { return status == Status::Executed; }
};

/// The result of one call to Machine::run().
struct RunResult {
    enum class Stop : std::uint8_t { Halted, StepLimit };
    std::size_t steps = 0;
    Stop stop = Stop::Halted;
};

/// A SAL machine. It is a value type: copy it to get an independent snapshot.
/// The copy shares the immutable Program and copies only MachineState.
class Machine {
public:
    /// Loads the program into code memory and resets the machine.
    /// Precondition: `program` is not null.
    explicit Machine(std::shared_ptr<const Program> program);

    /// Executes one instruction: fetch -> decode -> execute -> PC update.
    /// On a halted machine it changes nothing and returns AlreadyHalted.
    StepResult step();

    /// Executes steps until the machine halts or `limit` steps run.
    RunResult run(std::size_t limit = kDefaultStepLimit);

    /// Puts the machine back in the state after load.
    void reset();

    // ---- Observation (no trace entries) --------------------------------------
    [[nodiscard]] const Program& program() const noexcept { return *program_; }
    [[nodiscard]] const MachineState& state() const noexcept { return state_; }
    [[nodiscard]] std::int32_t a() const noexcept { return state_.a; }
    [[nodiscard]] std::int32_t b() const noexcept { return state_.b; }
    [[nodiscard]] std::uint8_t pc() const noexcept { return state_.pc; }
    [[nodiscard]] bool z() const noexcept { return state_.z; }
    [[nodiscard]] bool v() const noexcept { return state_.v; }
    [[nodiscard]] bool halted() const noexcept { return state_.halt_reason.has_value(); }
    [[nodiscard]] std::optional<HaltReason> halt_reason() const noexcept { return state_.halt_reason; }
    [[nodiscard]] std::uint64_t steps() const noexcept { return state_.steps; }
    [[nodiscard]] const Cell& cell(std::uint8_t address) const noexcept { return state_.memory[address]; }
    /// The value of a data cell. Returns nothing for a code or empty cell.
    [[nodiscard]] std::optional<std::int32_t> data(std::uint8_t address) const noexcept;
    /// The value at the address of a symbol. Returns nothing for an unknown name.
    [[nodiscard]] std::optional<std::int32_t> value_of(std::string_view symbol) const noexcept;

private:
    std::shared_ptr<const Program> program_;
    MachineState state_;
};

}  // namespace sal
