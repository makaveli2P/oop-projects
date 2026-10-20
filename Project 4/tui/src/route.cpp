#include "sal_tui/route.hpp"

#include <algorithm>
#include <type_traits>

namespace sal::tui {

namespace {

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

Unit unit_of(sal::Register reg) noexcept { return reg == sal::Register::A ? Unit::A : Unit::B; }
Unit unit_of(sal::Flag flag) noexcept { return flag == sal::Flag::Z ? Unit::Z : Unit::V; }

template <class T>
bool holds(const sal::MicroOp& op) noexcept {
    return std::holds_alternative<T>(op);
}

/// Where a register write gets its value. The rules follow the order of the
/// Datapath calls in each instruction (see core/src/instructions.cpp).
Unit write_source(const std::vector<sal::MicroOp>& ops, std::size_t index, sal::Register target) {
    const auto begin = ops.begin();
    const auto end = begin + static_cast<std::ptrdiff_t>(index);
    if (std::any_of(begin, end, holds<sal::AluAdd>)) {
        return Unit::ALU;  // ADD: the ALU result goes to A.
    }
    if (std::any_of(begin, end, holds<sal::MemRead>)) {
        return Unit::Memory;  // LDA, LDB.
    }
    const bool other_read = std::any_of(begin, end, [target](const sal::MicroOp& op) {
        const auto* read = std::get_if<sal::RegRead>(&op);
        return read != nullptr && read->reg != target;
    });
    if (other_read) {
        return target == sal::Register::A ? Unit::B : Unit::A;  // XCH.
    }
    return Unit::Decoder;  // LDI: the immediate value comes from the instruction.
}

/// Where a register read sends its value: the first later micro-operation
/// that is not a register read tells it.
Unit read_target(const std::vector<sal::MicroOp>& ops, std::size_t index, sal::Register source) {
    for (std::size_t i = index + 1; i < ops.size(); ++i) {
        if (holds<sal::AluAdd>(ops[i])) return Unit::ALU;
        if (holds<sal::MemWrite>(ops[i])) return Unit::Memory;
        if (holds<sal::RegWrite>(ops[i])) return source == sal::Register::A ? Unit::B : Unit::A;
        if (!holds<sal::RegRead>(ops[i])) break;
    }
    return Unit::ALU;
}

}  // namespace

const char* to_string(Unit unit) noexcept {
    switch (unit) {
        case Unit::PC: return "PC";
        case Unit::Memory: return "Memory";
        case Unit::IR: return "IR";
        case Unit::Decoder: return "Decoder";
        case Unit::A: return "A";
        case Unit::B: return "B";
        case Unit::ALU: return "ALU";
        case Unit::Z: return "Z";
        case Unit::V: return "V";
    }
    return "?";
}

bool Route::is_source(Unit unit) const noexcept {
    return std::find(sources.begin(), sources.end(), unit) != sources.end();
}

bool Route::is_target(Unit unit) const noexcept {
    return std::find(targets.begin(), targets.end(), unit) != targets.end();
}

Route route_of(const std::vector<sal::MicroOp>& ops, std::size_t index, NumberBase base) {
    const auto word = [base](std::int32_t value) { return format_word(value, base); };
    const auto address = [base](std::uint8_t value) { return format_address(value, base); };
    return std::visit(
        Overloaded{
            [&](const sal::Fetch& m) {
                return Route{{Unit::PC, Unit::Memory}, {Unit::IR}, m.text};
            },
            [&](const sal::Decode& m) {
                return Route{{Unit::IR}, {Unit::Decoder}, m.opcode};
            },
            [&](const sal::RegRead& m) {
                return Route{{unit_of(m.reg)}, {read_target(ops, index, m.reg)}, word(m.value)};
            },
            [&](const sal::RegWrite& m) {
                return Route{{write_source(ops, index, m.reg)}, {unit_of(m.reg)}, word(m.new_value)};
            },
            [&](const sal::FlagRead& m) {
                return Route{{unit_of(m.flag)}, {Unit::Decoder},
                             std::string{to_string(m.flag)} + " = " + (m.value ? "1" : "0")};
            },
            [&](const sal::FlagWrite& m) {
                return Route{{Unit::ALU}, {unit_of(m.flag)}, m.new_value ? "1" : "0"};
            },
            [&](const sal::MemRead& m) {
                // The register that receives the word is in the next micro-operation.
                Unit target = Unit::A;
                if (index + 1 < ops.size()) {
                    if (const auto* write = std::get_if<sal::RegWrite>(&ops[index + 1])) {
                        target = unit_of(write->reg);
                    }
                }
                return Route{{Unit::Memory}, {target}, word(m.value)};
            },
            [&](const sal::MemWrite& m) {
                return Route{{Unit::A}, {Unit::Memory}, word(m.new_value)};
            },
            [&](const sal::AluAdd& m) {
                return Route{{Unit::A, Unit::B}, {Unit::ALU}, word(m.result)};
            },
            [&](const sal::PcWrite& m) {
                if (m.cause == sal::PcCause::Jump || m.cause == sal::PcCause::BranchTaken) {
                    return Route{{Unit::Decoder}, {Unit::PC}, address(m.new_value)};
                }
                return Route{{}, {Unit::PC}, address(m.new_value)};
            },
            [&](const sal::SymbolInfo& m) {
                return Route{{Unit::Decoder}, {Unit::Memory}, std::to_string(m.address)};
            },
            [&](const sal::Halt&) { return Route{}; },
        },
        ops.at(index));
}

}  // namespace sal::tui
