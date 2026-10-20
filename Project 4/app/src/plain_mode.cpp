// plain_mode.cpp - the classic text mode of the original course project.
#include "plain_mode.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "sal/sal.hpp"

namespace ali {

namespace {

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

/// Reads one line. Returns nothing at end of input.
std::optional<std::string> read_line(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::nullopt;
    }
    return std::string{trim(line)};
}

void print_registers(const sal::Machine& m, std::ostream& out) {
    out << "Registers: A = " << m.a() << "  B = " << m.b() << "  PC = " << int{m.pc()}
        << "  Z = " << m.z() << "  V = " << m.v() << '\n';
    if (const auto reason = m.halt_reason()) {
        out << "The machine is halted: " << sal::to_string(*reason) << ".\n";
    }
}

void print_code(const sal::Machine& m, std::ostream& out) {
    out << "Code memory:\n";
    for (const sal::CodeLine& line : m.program().code()) {
        const auto address = static_cast<std::size_t>(&line - m.program().code().data());
        const bool current = !m.halted() && address == m.pc();
        out << (current ? "  -> " : "     ") << std::setw(3) << address << "  " << std::left
            << std::setw(24) << line.text << std::right << " (line " << line.line << ")\n";
    }
}

void print_data(const sal::Machine& m, std::ostream& out) {
    out << "Data memory:\n";
    if (m.program().symbols().empty()) {
        out << "     (no symbols)\n";
    }
    for (const sal::Symbol& symbol : m.program().symbols()) {
        out << "     " << std::setw(3) << int{symbol.address} << "  " << std::left << std::setw(12)
            << symbol.name << std::right << " = " << m.data(symbol.address).value_or(0) << '\n';
    }
}

void do_step(sal::Machine& m, std::ostream& out) {
    const sal::StepResult step = m.step();
    if (!step.executed()) {
        out << "The machine is already halted. Nothing to execute.\n";
        return;
    }
    const sal::CodeLine& line = m.program().at(step.address);
    out << "\n[step " << m.steps() << "] address " << int{step.address} << ": " << line.text
        << "  -- " << line.instruction->describe() << '\n';
    for (const sal::MicroOp& op : step.micro_ops) {
        out << "    " << sal::describe(op) << '\n';
    }
    print_registers(m, out);
    print_code(m, out);
    print_data(m, out);
}

/// Runs to a halt. After each block of `kDefaultStepLimit` steps, it asks to continue.
void do_run_all(sal::Machine& m, std::istream& in, std::ostream& out) {
    if (m.halted()) {
        out << "The machine is already halted. Nothing to execute.\n";
        return;
    }
    std::size_t total = 0;
    for (;;) {
        const sal::RunResult result = m.run(sal::kDefaultStepLimit);
        total += result.steps;
        if (result.stop == sal::RunResult::Stop::Halted) {
            break;
        }
        out << "The program ran " << total << " steps and did not halt. Continue? [y/n] "
            << std::flush;
        const auto answer = read_line(in);
        if (!answer || answer->empty() || (answer->front() != 'y' && answer->front() != 'Y')) {
            out << '\n';
            break;
        }
    }
    out << "\nExecuted " << total << " steps.\n";
    print_registers(m, out);
    print_code(m, out);
    print_data(m, out);
}

}  // namespace

int run_plain(const std::optional<std::string>& file, std::istream& in, std::ostream& out,
              std::ostream& err) {
    std::string path;
    if (file) {
        path = *file;
    } else {
        out << "Enter the name of a SAL program file: " << std::flush;
        const auto answer = read_line(in);
        if (!answer || answer->empty()) {
            err << "ali: no file name\n";
            return 1;
        }
        path = *answer;
    }

    const sal::AssembleResult loaded = sal::assemble_file(path);
    if (!loaded) {
        for (const sal::ParseError& e : loaded.errors()) {
            err << path << ": " << sal::to_string(e) << '\n';
        }
        return 1;
    }

    sal::Machine machine{loaded.program()};
    out << "Loaded " << machine.program().size() << " instructions and "
        << machine.program().symbols().size() << " symbols from " << path << ".\n";
    print_code(machine, out);

    for (;;) {
        out << "\nCommand (s = step, a = run all, q = quit): " << std::flush;
        const auto command = read_line(in);
        if (!command || *command == "q" || *command == "Q") {
            out << '\n';
            return 0;
        }
        if (*command == "s" || *command == "S") {
            do_step(machine, out);
        } else if (*command == "a" || *command == "A") {
            do_run_all(machine, in, out);
        } else if (!command->empty()) {
            out << "Unknown command '" << *command << "'. Use s, a or q.\n";
        }
    }
}

}  // namespace ali
