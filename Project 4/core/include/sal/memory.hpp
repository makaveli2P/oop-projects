// memory.hpp - the 256-word SAL memory and the machine state.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

#include "sal/micro_op.hpp"

namespace sal {

struct CodeLine;  // Defined in program.hpp.

inline constexpr std::size_t kMemorySize = 256;    ///< Words in memory.
inline constexpr std::size_t kCodeSize = 128;      ///< Addresses 0-127 hold code.
inline constexpr std::uint8_t kDataBase = 128;     ///< Addresses 128-255 hold data.
inline constexpr std::size_t kDataSize = 128;      ///< Words for data.
inline constexpr std::size_t kDefaultStepLimit = 1000;

/// A memory cell that holds nothing (code space after the last instruction).
struct EmptyCell {
    friend constexpr bool operator==(EmptyCell, EmptyCell) noexcept { return true; }
};

/// A memory cell in code space. It points at the decoded instruction, the
/// source text and the source line number. The pointer is non-owning: the
/// shared, immutable Program owns the line and outlives every machine.
struct CodeCell {
    const CodeLine* line;
    friend constexpr bool operator==(CodeCell l, CodeCell r) noexcept { return l.line == r.line; }
};

/// A memory cell in data space. It holds one 32-bit word.
struct DataCell {
    std::int32_t value;
    friend constexpr bool operator==(DataCell l, DataCell r) noexcept { return l.value == r.value; }
};

using Cell = std::variant<EmptyCell, CodeCell, DataCell>;

/// One array of 256 cells. A std::uint8_t address cannot go out of bounds.
using Memory = std::array<Cell, kMemorySize>;

[[nodiscard]] constexpr bool is_data_address(std::uint8_t address) noexcept {
    return address >= kDataBase;
}

/// The complete mutable state of the machine. It is a plain value: a copy
/// is a snapshot. The TUI keeps snapshots to step back.
struct MachineState {
    Memory memory{};
    std::int32_t a = 0;
    std::int32_t b = 0;
    std::uint8_t pc = 0;
    bool z = false;
    bool v = false;
    std::optional<HaltReason> halt_reason;  ///< Has a value when the machine halted.
    std::uint64_t steps = 0;                ///< Number of instructions executed.
};

}  // namespace sal
