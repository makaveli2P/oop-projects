#include "sal/machine.hpp"

#include <cassert>

#include "sal/datapath.hpp"

namespace sal {

Machine::Machine(std::shared_ptr<const Program> program) : program_(std::move(program)) {
    assert(program_ != nullptr);
    reset();
}

void Machine::reset() {
    state_ = MachineState{};
    const auto& code = program_->code();
    for (std::size_t address = 0; address < code.size(); ++address) {
        state_.memory[address] = CodeCell{&code[address]};
    }
    for (std::size_t address = kDataBase; address < kMemorySize; ++address) {
        state_.memory[address] = DataCell{0};
    }
}

StepResult Machine::step() {
    StepResult result;
    result.address = state_.pc;
    if (halted()) {
        return result;  // AlreadyHalted, no micro-operations.
    }
    result.status = StepResult::Status::Executed;
    auto& trace = result.micro_ops;

    // Fetch. The PC always points at a code cell while the machine runs.
    const CodeLine& line = program_->at(state_.pc);
    trace.emplace_back(Fetch{state_.pc, state_.pc, line.text});

    // Decode.
    const Instruction& instruction = *line.instruction;
    trace.emplace_back(Decode{std::string{instruction.mnemonic()}, instruction.operand()});

    // Execute.
    // A fault stops the instruction before it changes more state.
    Datapath dp{state_, *program_, trace};
    NextPc next;
    std::optional<std::uint8_t> fault_address;
    try {
        next = instruction.execute(dp);
    } catch (const Datapath::Fault& fault) {
        fault_address = fault.address;
    }
    ++state_.steps;

    // PC update.
    const std::uint8_t old_pc = state_.pc;
    if (fault_address) {
        trace.emplace_back(PcWrite{old_pc, old_pc, PcCause::Halt});
        trace.emplace_back(Halt{HaltReason::RuntimeFault, fault_address});
        state_.halt_reason = HaltReason::RuntimeFault;
        return result;
    }
    switch (next.cause) {
        case PcCause::Halt:
            trace.emplace_back(PcWrite{old_pc, old_pc, PcCause::Halt});
            trace.emplace_back(Halt{HaltReason::HltExecuted});
            state_.halt_reason = HaltReason::HltExecuted;
            return result;
        case PcCause::Jump:
        case PcCause::BranchTaken:
            state_.pc = next.target;
            break;
        case PcCause::Increment:
        case PcCause::BranchNotTaken:
            // old_pc <= 127, so old_pc + 1 fits in std::uint8_t.
            state_.pc = static_cast<std::uint8_t>(old_pc + 1U);
            break;
    }
    trace.emplace_back(PcWrite{old_pc, state_.pc, next.cause});
    if (state_.pc >= program_->size()) {
        trace.emplace_back(Halt{HaltReason::RanPastEnd});
        state_.halt_reason = HaltReason::RanPastEnd;
    }
    return result;
}

RunResult Machine::run(std::size_t limit) {
    RunResult result;
    while (!halted()) {
        if (result.steps >= limit) {
            result.stop = RunResult::Stop::StepLimit;
            return result;
        }
        step();
        ++result.steps;
    }
    result.stop = RunResult::Stop::Halted;
    return result;
}

std::optional<std::int32_t> Machine::data(std::uint8_t address) const noexcept {
    if (const auto* cell = std::get_if<DataCell>(&state_.memory[address])) {
        return cell->value;
    }
    return std::nullopt;
}

std::optional<std::int32_t> Machine::value_of(std::string_view symbol) const noexcept {
    if (const auto address = program_->address_of(symbol)) {
        return data(*address);
    }
    return std::nullopt;
}

}  // namespace sal
