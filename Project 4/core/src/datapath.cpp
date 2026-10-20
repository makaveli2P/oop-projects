#include "sal/datapath.hpp"

#include "sal/program.hpp"

namespace sal {

std::int32_t Datapath::read_reg(Register reg) {
    const std::int32_t value = reg == Register::A ? state_.a : state_.b;
    trace_.emplace_back(RegRead{reg, value});
    return value;
}

void Datapath::write_reg(Register reg, std::int32_t value) {
    std::int32_t& target = reg == Register::A ? state_.a : state_.b;
    trace_.emplace_back(RegWrite{reg, target, value});
    target = value;
}

bool Datapath::read_flag(Flag flag) {
    const bool value = flag == Flag::Z ? state_.z : state_.v;
    trace_.emplace_back(FlagRead{flag, value});
    return value;
}

void Datapath::write_flag(Flag flag, bool value) {
    bool& target = flag == Flag::Z ? state_.z : state_.v;
    trace_.emplace_back(FlagWrite{flag, target, value});
    target = value;
}

std::int32_t Datapath::read_mem(std::uint8_t address) {
    const auto* cell = std::get_if<DataCell>(&state_.memory[address]);
    if (cell == nullptr) {
        throw Fault{address};
    }
    trace_.emplace_back(MemRead{address, symbol_name(address), cell->value});
    return cell->value;
}

void Datapath::write_mem(std::uint8_t address, std::int32_t value) {
    auto* cell = std::get_if<DataCell>(&state_.memory[address]);
    if (cell == nullptr) {
        throw Fault{address};
    }
    trace_.emplace_back(MemWrite{address, symbol_name(address), cell->value, value});
    cell->value = value;
}

AddResult Datapath::alu_add(std::int32_t a, std::int32_t b) {
    const AddResult sum = add32(a, b);
    trace_.emplace_back(AluAdd{a, b, sum.wide_sum, sum.result});
    return sum;
}

void Datapath::report_symbol(const std::string& name, std::uint8_t address) {
    trace_.emplace_back(SymbolInfo{name, address});
}

std::optional<std::string> Datapath::symbol_name(std::uint8_t address) const {
    if (const Symbol* symbol = program_.symbol_at(address)) {
        return symbol->name;
    }
    return std::nullopt;
}

}  // namespace sal
