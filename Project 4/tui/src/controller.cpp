#include "sal_tui/controller.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <type_traits>
#include <utility>

namespace sal::tui {

namespace {

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

/// The delay between two micro-operations at each speed level.
constexpr std::array<int, 8> kDelaysMs{1000, 600, 350, 200, 120, 70, 40, 20};

const std::vector<sal::MicroOp> kNoMicroOps;

std::string halt_sentence(sal::HaltReason reason) {
    return "The machine is halted (" + std::string{sal::to_string(reason)} + "). Press R to reset.";
}

}  // namespace

void apply_micro_op(sal::MachineState& state, const sal::MicroOp& op) {
    std::visit(Overloaded{
                   [&](const sal::RegWrite& m) {
                       (m.reg == sal::Register::A ? state.a : state.b) = m.new_value;
                   },
                   [&](const sal::FlagWrite& m) {
                       (m.flag == sal::Flag::Z ? state.z : state.v) = m.new_value;
                   },
                   [&](const sal::MemWrite& m) { state.memory[m.address] = sal::DataCell{m.new_value}; },
                   [&](const sal::PcWrite& m) { state.pc = m.new_value; },
                   [&](const sal::Halt& m) { state.halt_reason = m.reason; },
                   // The other micro-operations read state; they change nothing.
                   [](const auto&) {},
               },
               op);
}

Controller::Controller(std::shared_ptr<const sal::Program> program, std::size_t history_limit)
    : machine_(std::move(program)), history_limit_(std::max<std::size_t>(history_limit, 1)) {
    update_view();
}

// ---- Commands ----------------------------------------------------------------

bool Controller::at_boundary() const noexcept {
    return history_.empty() || cursor_ == history_.back().step.micro_ops.size();
}

bool Controller::begin_instruction() {
    if (machine_.halted()) {
        set_halt_message();
        return false;
    }
    Entry entry{machine_, {}};
    entry.step = machine_.step();
    history_.push_back(std::move(entry));
    if (history_.size() > history_limit_) {
        history_.pop_front();
    }
    cursor_ = 0;
    return true;
}

void Controller::next_micro_op() {
    message_.clear();
    if (!at_boundary()) {
        ++cursor_;
    } else if (begin_instruction()) {
        cursor_ = 1;
    }
    update_view();
}

void Controller::step_instruction() {
    message_.clear();
    if (!at_boundary() || begin_instruction()) {
        cursor_ = history_.back().step.micro_ops.size();
    }
    update_view();
}

void Controller::back() {
    message_.clear();
    pause();
    if (history_.empty()) {
        message_ = machine_.steps() == 0 ? "This is the start of the program."
                                         : "The history holds no earlier steps.";
        return;
    }
    machine_ = history_.back().before;
    history_.pop_back();
    cursor_ = history_.empty() ? 0 : history_.back().step.micro_ops.size();
    update_view();
}

void Controller::reset() {
    pause();
    machine_.reset();
    history_.clear();
    cursor_ = 0;
    message_ = "The machine is reset.";
    update_view();
}

RunToEndResult Controller::run_to_end(std::size_t limit) {
    message_.clear();
    pause();
    RunToEndResult result;
    // The run does not stop at the breakpoint where it starts. A run that
    // starts mid-instruction starts at no breakpoint, so it checks the next one.
    const bool started_at_boundary = at_boundary();
    if (!started_at_boundary) {
        cursor_ = history_.back().step.micro_ops.size();  // Complete the current instruction.
    }
    if (machine_.halted()) {
        result.stop = RunToEndResult::Stop::AlreadyHalted;
        set_halt_message();
        update_view();
        return result;
    }
    while (!machine_.halted()) {
        const bool at_start = started_at_boundary && result.steps == 0;
        if (!at_start && has_breakpoint(machine_.pc())) {
            result.stop = RunToEndResult::Stop::Breakpoint;
            message_ = "Stopped at the breakpoint at address " + std::to_string(machine_.pc()) + ".";
            update_view();
            return result;
        }
        if (result.steps == limit) {
            result.stop = RunToEndResult::Stop::StepLimit;
            message_ = "ALI ran " + std::to_string(limit) +
                       " instructions and the machine did not halt. Press g to run " +
                       std::to_string(limit) + " more.";
            update_view();
            return result;
        }
        begin_instruction();
        cursor_ = history_.back().step.micro_ops.size();
        ++result.steps;
    }
    result.stop = RunToEndResult::Stop::Halted;
    set_halt_message();
    update_view();
    return result;
}

void Controller::toggle_run() {
    message_.clear();
    if (running_) {
        pause();
        return;
    }
    if (at_boundary() && machine_.halted()) {
        set_halt_message();
        return;
    }
    running_ = true;
    // Only a run that starts at an instruction boundary starts "at" the
    // breakpoint of the next instruction. A run that starts mid-instruction
    // must stop at the next breakpoint.
    run_started_ = at_boundary();
}

bool Controller::tick() {
    if (!running_) {
        return false;
    }
    if (at_boundary()) {
        if (machine_.halted()) {
            pause();
            set_halt_message();
            return true;
        }
        // A breakpoint stops the run before its instruction executes. The run
        // does not stop at the breakpoint where it starts.
        if (!run_started_ && has_breakpoint(machine_.pc())) {
            pause();
            message_ = "Stopped at the breakpoint at address " + std::to_string(machine_.pc()) + ".";
            return true;
        }
        begin_instruction();
        cursor_ = 1;
        run_started_ = false;
    } else {
        ++cursor_;
    }
    if (at_boundary() && machine_.halted()) {
        pause();
        set_halt_message();
    }
    update_view();
    return true;
}

void Controller::faster() noexcept {
    if (speed_ + 1 < kDelaysMs.size()) {
        ++speed_;
    }
}

void Controller::slower() noexcept {
    if (speed_ > 0) {
        --speed_;
    }
}

std::size_t Controller::speed_level_count() noexcept { return kDelaysMs.size(); }

std::chrono::milliseconds Controller::tick_interval() const noexcept {
    return std::chrono::milliseconds{kDelaysMs[speed_]};
}

void Controller::toggle_breakpoint(std::uint8_t address) {
    if (address >= program().size()) {
        return;
    }
    if (!breakpoints_.erase(address)) {
        breakpoints_.insert(address);
    }
}

void Controller::move_selection(int delta) noexcept {
    const int last = static_cast<int>(program().size()) - 1;
    const int next = std::clamp(int{selection_} + delta, 0, last);
    selection_ = static_cast<std::uint8_t>(next);
}

void Controller::select(std::uint8_t address) noexcept {
    if (address < program().size()) {
        selection_ = address;
    }
}

void Controller::toggle_base() noexcept {
    base_ = base_ == NumberBase::Decimal ? NumberBase::Hex : NumberBase::Decimal;
}

// ---- Observation ---------------------------------------------------------------

std::optional<std::uint8_t> Controller::instruction_address() const noexcept {
    if (history_.empty()) {
        return std::nullopt;
    }
    return history_.back().step.address;
}

const std::vector<sal::MicroOp>& Controller::micro_ops() const noexcept {
    return history_.empty() ? kNoMicroOps : history_.back().step.micro_ops;
}

const sal::MicroOp* Controller::active_micro_op() const noexcept {
    if (cursor_ == 0) {
        return nullptr;
    }
    return &micro_ops()[cursor_ - 1];
}

bool Controller::instruction_complete() const noexcept {
    return has_instruction() && at_boundary();
}

std::uint64_t Controller::step_number() const noexcept {
    return history_.empty() ? 0 : history_.back().before.steps() + 1;
}

void Controller::set_halt_message() {
    if (const auto reason = machine_.halt_reason()) {
        message_ = halt_sentence(*reason);
    }
}

void Controller::update_view() {
    if (history_.empty()) {
        view_ = machine_.state();
        previous_ = view_;
        return;
    }
    const Entry& entry = history_.back();
    previous_ = entry.before.state();
    if (at_boundary()) {
        view_ = machine_.state();
        return;
    }
    view_ = previous_;
    for (std::size_t i = 0; i < cursor_; ++i) {
        apply_micro_op(view_, entry.step.micro_ops[i]);
    }
}

}  // namespace sal::tui
