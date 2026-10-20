// micro_op.hpp - the hardware-level trace that one SAL step produces.
//
// Each SAL instruction executes as a short sequence of micro-operations.
// A micro-operation names one hardware component (memory, PC, IR, decoder,
// A, B, ALU, Z, V) and one data movement. The plain mode prints the
// micro-operations as sentences. The visualizer highlights one component
// or one datapath edge for each micro-operation.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace sal {

/// The two general-purpose registers.
enum class Register : std::uint8_t { A, B };

/// The two status bits. Only ADD writes them.
enum class Flag : std::uint8_t { Z, V };

/// The cause of a write to the program counter.
enum class PcCause : std::uint8_t {
    Increment,       ///< Normal sequential flow: PC + 1.
    Jump,            ///< JMP: an unconditional jump.
    BranchTaken,     ///< JZS or JVS, and the flag is set.
    BranchNotTaken,  ///< JZS or JVS, and the flag is clear: PC + 1.
    Halt,            ///< HLT: the PC keeps its value.
};

/// The reason why a machine stopped.
enum class HaltReason : std::uint8_t {
    HltExecuted,     ///< The program executed HLT.
    RanPastEnd,      ///< The PC moved past the last instruction.
    RuntimeFault,    ///< An instruction made an illegal access.
};

// ---- The micro-operations --------------------------------------------------

/// The control unit reads the instruction at `address` into the IR.
struct Fetch {
    std::uint8_t pc;
    std::uint8_t address;
    std::string text;  ///< The source text of the instruction.
};

/// The decoder splits the IR into an opcode and an operand.
struct Decode {
    std::string opcode;
    std::string operand;  ///< Empty when the instruction has no operand.
};

/// A register drives its value onto the datapath.
struct RegRead {
    Register reg;
    std::int32_t value;
};

/// A register latches a new value.
struct RegWrite {
    Register reg;
    std::int32_t old_value;
    std::int32_t new_value;
};

/// A branch instruction tests a status bit.
struct FlagRead {
    Flag flag;
    bool value;
};

/// A status bit latches a new value.
struct FlagWrite {
    Flag flag;
    bool old_value;
    bool new_value;
};

/// Memory drives the word at `address` onto the datapath.
struct MemRead {
    std::uint8_t address;
    std::optional<std::string> symbol;
    std::int32_t value;
};

/// Memory latches a new word at `address`.
struct MemWrite {
    std::uint8_t address;
    std::optional<std::string> symbol;
    std::int32_t old_value;
    std::int32_t new_value;
};

/// The ALU adds two words. `wide_sum` is the exact 64-bit sum;
/// `result` is the 32-bit two's-complement result.
struct AluAdd {
    std::int32_t a;
    std::int32_t b;
    std::int64_t wide_sum;
    std::int32_t result;
};

/// The PC latches a new value.
struct PcWrite {
    std::uint8_t old_value;
    std::uint8_t new_value;
    PcCause cause;
};

/// DEC reports the data address that the assembler gave to a symbol.
struct SymbolInfo {
    std::string name;
    std::uint8_t address;
};

/// The machine stops.
struct Halt {
    HaltReason reason;
    /// For a runtime fault: the address that the instruction tried to use.
    std::optional<std::uint8_t> fault_address = std::nullopt;
};

/// One micro-operation. The order of the alternatives has no meaning.
using MicroOp = std::variant<Fetch, Decode, RegRead, RegWrite, FlagRead, FlagWrite,
                             MemRead, MemWrite, AluAdd, PcWrite, SymbolInfo, Halt>;

// ---- Text helpers ----------------------------------------------------------

[[nodiscard]] std::string_view to_string(Register reg) noexcept;
[[nodiscard]] std::string_view to_string(Flag flag) noexcept;
[[nodiscard]] std::string_view to_string(PcCause cause) noexcept;
[[nodiscard]] std::string_view to_string(HaltReason reason) noexcept;

/// Returns one short English sentence for a micro-operation, for example
/// "Memory[130] (X) -> A: 7". The plain mode and the TUI use this function.
[[nodiscard]] std::string describe(const MicroOp& op);

}  // namespace sal
