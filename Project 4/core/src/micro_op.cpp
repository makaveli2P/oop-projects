#include "sal/micro_op.hpp"

#include <sstream>
#include <type_traits>

namespace sal {

namespace {

// The "overloaded" idiom: one visitor made from several lambdas.
template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

std::string memory_name(std::uint8_t address, const std::optional<std::string>& symbol) {
    std::string text = "Memory[" + std::to_string(address) + "]";
    if (symbol) {
        text += " (" + *symbol + ")";
    }
    return text;
}

const char* bit(bool value) noexcept { return value ? "1" : "0"; }

}  // namespace

std::string_view to_string(Register reg) noexcept {
    return reg == Register::A ? "A" : "B";
}

std::string_view to_string(Flag flag) noexcept {
    return flag == Flag::Z ? "Z" : "V";
}

std::string_view to_string(PcCause cause) noexcept {
    switch (cause) {
        case PcCause::Increment: return "increment";
        case PcCause::Jump: return "jump";
        case PcCause::BranchTaken: return "branch taken";
        case PcCause::BranchNotTaken: return "branch not taken";
        case PcCause::Halt: return "halt";
    }
    return "unknown";
}

std::string_view to_string(HaltReason reason) noexcept {
    switch (reason) {
        case HaltReason::HltExecuted: return "HLT executed";
        case HaltReason::RanPastEnd: return "ran past end of program";
        case HaltReason::RuntimeFault: return "runtime fault";
    }
    return "unknown";
}

std::string describe(const MicroOp& op) {
    std::ostringstream out;
    std::visit(
        Overloaded{
            [&](const Fetch& m) {
                out << "Fetch: Memory[" << int{m.address} << "] -> IR: \"" << m.text << "\"";
            },
            [&](const Decode& m) {
                out << "Decode: opcode " << m.opcode;
                if (!m.operand.empty()) {
                    out << ", operand " << m.operand;
                }
            },
            [&](const RegRead& m) { out << "Read " << to_string(m.reg) << ": " << m.value; },
            [&](const RegWrite& m) {
                out << "Write " << to_string(m.reg) << ": " << m.old_value << " -> " << m.new_value;
            },
            [&](const FlagRead& m) { out << "Test " << to_string(m.flag) << ": " << bit(m.value); },
            [&](const FlagWrite& m) {
                out << "Set " << to_string(m.flag) << ": " << bit(m.old_value) << " -> "
                    << bit(m.new_value);
            },
            [&](const MemRead& m) {
                out << "Read " << memory_name(m.address, m.symbol) << ": " << m.value;
            },
            [&](const MemWrite& m) {
                out << "Write " << memory_name(m.address, m.symbol) << ": " << m.old_value
                    << " -> " << m.new_value;
            },
            [&](const AluAdd& m) {
                out << "ALU: " << m.a << " + " << m.b << " = " << m.wide_sum;
                if (m.wide_sum != m.result) {
                    out << ", wraps to " << m.result;
                }
            },
            [&](const PcWrite& m) {
                out << "PC: " << int{m.old_value} << " -> " << int{m.new_value} << " ("
                    << to_string(m.cause) << ")";
            },
            [&](const SymbolInfo& m) {
                out << "Symbol " << m.name << " is at address " << int{m.address};
            },
            [&](const Halt& m) {
                out << "Halt: " << to_string(m.reason);
                if (m.fault_address) {
                    out << " (address " << int{*m.fault_address} << " is not a data address)";
                }
            },
        },
        op);
    return out.str();
}

}  // namespace sal
