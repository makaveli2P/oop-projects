// instructions.cpp - the behavior of each SAL instruction.
//
// Each execute() uses only the Datapath API. The order of the calls is the
// order of the micro-operations in the trace.
#include <string>

#include "sal/datapath.hpp"
#include "sal/instruction.hpp"

namespace sal {

namespace {

std::string at_address(const std::string& symbol, std::uint8_t address) {
    return symbol + " (address " + std::to_string(address) + ")";
}

}  // namespace

std::string_view to_string(Opcode opcode) noexcept {
    switch (opcode) {
        case Opcode::DEC: return "DEC";
        case Opcode::LDA: return "LDA";
        case Opcode::LDB: return "LDB";
        case Opcode::LDI: return "LDI";
        case Opcode::STR: return "STR";
        case Opcode::XCH: return "XCH";
        case Opcode::JMP: return "JMP";
        case Opcode::JZS: return "JZS";
        case Opcode::JVS: return "JVS";
        case Opcode::ADD: return "ADD";
        case Opcode::HLT: return "HLT";
    }
    return "???";
}

// ---- DEC ---------------------------------------------------------------------

std::string Dec::describe() const {
    return "Declare symbol " + at_address(symbol(), address());
}

NextPc Dec::execute(Datapath& dp) const {
    dp.report_symbol(symbol(), address());
    return {};
}

// ---- LDA / LDB / STR -------------------------------------------------------------

std::string Lda::describe() const { return "Load A from " + at_address(symbol(), address()); }

NextPc Lda::execute(Datapath& dp) const {
    dp.write_reg(Register::A, dp.read_mem(address()));
    return {};
}

std::string Ldb::describe() const { return "Load B from " + at_address(symbol(), address()); }

NextPc Ldb::execute(Datapath& dp) const {
    dp.write_reg(Register::B, dp.read_mem(address()));
    return {};
}

std::string Str::describe() const { return "Store A into " + at_address(symbol(), address()); }

NextPc Str::execute(Datapath& dp) const {
    dp.write_mem(address(), dp.read_reg(Register::A));
    return {};
}

// ---- LDI -----------------------------------------------------------------------

std::string Ldi::describe() const { return "Load the value " + std::to_string(value_) + " into A"; }

NextPc Ldi::execute(Datapath& dp) const {
    dp.write_reg(Register::A, value_);
    return {};
}

// ---- XCH / ADD / HLT -----------------------------------------------------------

std::string Xch::describe() const { return "Exchange A and B"; }

NextPc Xch::execute(Datapath& dp) const {
    const std::int32_t a = dp.read_reg(Register::A);
    const std::int32_t b = dp.read_reg(Register::B);
    dp.write_reg(Register::A, b);
    dp.write_reg(Register::B, a);
    return {};
}

std::string Add::describe() const { return "Add B to A; set Z and V"; }

NextPc Add::execute(Datapath& dp) const {
    const std::int32_t a = dp.read_reg(Register::A);
    const std::int32_t b = dp.read_reg(Register::B);
    const AddResult sum = dp.alu_add(a, b);
    dp.write_reg(Register::A, sum.result);
    dp.write_flag(Flag::Z, sum.zero);
    dp.write_flag(Flag::V, sum.overflow);
    return {};
}

std::string Hlt::describe() const { return "Halt the machine"; }

NextPc Hlt::execute(Datapath& /*dp*/) const { return NextPc{PcCause::Halt, 0}; }

// ---- Jumps -----------------------------------------------------------------------

NextPc JumpInstruction::branch_if(Datapath& dp, Flag flag) const {
    if (dp.read_flag(flag)) {
        return NextPc{PcCause::BranchTaken, target_};
    }
    return NextPc{PcCause::BranchNotTaken, 0};
}

std::string Jmp::describe() const { return "Jump to address " + operand(); }

NextPc Jmp::execute(Datapath& /*dp*/) const { return NextPc{PcCause::Jump, target()}; }

std::string Jzs::describe() const { return "Jump to address " + operand() + " if Z is set"; }

NextPc Jzs::execute(Datapath& dp) const { return branch_if(dp, Flag::Z); }

std::string Jvs::describe() const { return "Jump to address " + operand() + " if V is set"; }

NextPc Jvs::execute(Datapath& dp) const { return branch_if(dp, Flag::V); }

}  // namespace sal
