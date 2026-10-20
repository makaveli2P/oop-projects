// datapath.hpp - the only door from an instruction to the machine state.
//
// An instruction never touches a machine field directly. It calls the
// Datapath methods. Each method changes the state AND appends one
// micro-operation to the trace of the current step. This class is the single
// hook point for the visualizer: if a change is not in the trace, it did not
// occur.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "sal/alu.hpp"
#include "sal/memory.hpp"
#include "sal/micro_op.hpp"

namespace sal {

class Machine;
class Program;

class Datapath {
public:
    /// Reads a register. Appends RegRead.
    std::int32_t read_reg(Register reg);
    /// Writes a register. Appends RegWrite.
    void write_reg(Register reg, std::int32_t value);

    /// Reads a status bit. Appends FlagRead.
    bool read_flag(Flag flag);
    /// Writes a status bit. Appends FlagWrite.
    void write_flag(Flag flag, bool value);

    /// Reads a data word. Appends MemRead. The address must be a data
    /// address (128-255). For any other address, read_mem appends nothing
    /// and throws a Fault. The fault stops the instruction at once, so the
    /// instruction changes no more state. Machine::step catches the fault
    /// and halts the machine with HaltReason::RuntimeFault.
    std::int32_t read_mem(std::uint8_t address);
    /// Writes a data word. Appends MemWrite. The address rule of read_mem applies.
    void write_mem(std::uint8_t address, std::int32_t value);

    /// Adds two words in the ALU. Appends AluAdd. It does not write A, Z or V.
    AddResult alu_add(std::int32_t a, std::int32_t b);

    /// Reports the address of a symbol. Appends SymbolInfo. No state changes.
    void report_symbol(const std::string& name, std::uint8_t address);

    /// The address of the instruction that executes now. No trace entry.
    [[nodiscard]] std::uint8_t current_address() const noexcept { return state_.pc; }

    Datapath(const Datapath&) = delete;
    Datapath& operator=(const Datapath&) = delete;

private:
    friend class Machine;  // Only the machine makes a datapath for a step.

    /// An illegal memory access. Only Machine can name this type.
    struct Fault {
        std::uint8_t address;
    };

    Datapath(MachineState& state, const Program& program, std::vector<MicroOp>& trace) noexcept
        : state_(state), program_(program), trace_(trace) {}

    [[nodiscard]] std::optional<std::string> symbol_name(std::uint8_t address) const;

    MachineState& state_;
    const Program& program_;
    std::vector<MicroOp>& trace_;
};

}  // namespace sal
