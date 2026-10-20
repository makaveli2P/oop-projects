// route.hpp - which hardware units a micro-operation uses.
//
// The datapath view draws one route for the active micro-operation: the
// source units, the target units and a label with the value that moves.
// This file has no FTXUI code, so the tests can examine every route.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "sal/micro_op.hpp"
#include "sal_tui/format.hpp"

namespace sal::tui {

/// The hardware units in the datapath diagram.
enum class Unit : std::uint8_t { PC, Memory, IR, Decoder, A, B, ALU, Z, V };

inline constexpr std::size_t kUnitCount = 9;

[[nodiscard]] const char* to_string(Unit unit) noexcept;

/// The data movement of one micro-operation.
struct Route {
    std::vector<Unit> sources;  ///< The units that drive the value.
    std::vector<Unit> targets;  ///< The units that receive or latch the value.
    std::string label;          ///< The value that moves, for example "1000000".

    [[nodiscard]] bool is_source(Unit unit) const noexcept;
    [[nodiscard]] bool is_target(Unit unit) const noexcept;
    [[nodiscard]] bool involves(Unit unit) const noexcept { return is_source(unit) || is_target(unit); }
};

/// Returns the route of `ops[index]`. The function looks at the other
/// micro-operations of the same instruction to find where a value comes from
/// or goes to. For example, the RegWrite of LDA gets its value from memory,
/// and the RegWrite of ADD gets its value from the ALU.
/// Precondition: index < ops.size().
[[nodiscard]] Route route_of(const std::vector<sal::MicroOp>& ops, std::size_t index,
                             NumberBase base = NumberBase::Decimal);

}  // namespace sal::tui
