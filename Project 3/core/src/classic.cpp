#include "strset/classic.h"

#include <istream>
#include <optional>
#include <ostream>
#include <string>

#include "strset/calculator.h"
#include "strset/command.h"

namespace strset {

namespace {

std::optional<std::string> read_line(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::nullopt;
    }
    return line;
}

void print_sets(const SetCalculator& calc, std::ostream& out) {
    out << "S1 = " << format_set(calc.s1()) << '\n' << "S2 = " << format_set(calc.s2()) << '\n';
}

void quit(const SetCalculator& calc, std::ostream& out) {
    print_sets(calc, out);
    out << "Quitting the set calculator.\n";
}

}  // namespace

void print_commands(std::ostream& out) {
    static constexpr CommandKind kinds[] = {
        CommandKind::Erase, CommandKind::Switch, CommandKind::Subset,       CommandKind::Copy,
        CommandKind::List,  CommandKind::Add,    CommandKind::Union,        CommandKind::Intersection,
        CommandKind::Quit,  CommandKind::Help,
    };
    out << "Commands:\n";
    for (const CommandKind kind : kinds) {
        out << "  " << letter_of(kind) << (kind == CommandKind::Add ? " <string>" : "         ") << "  "
            << summary_of(kind) << '\n';
    }
}

int run_classic(std::istream& in, std::ostream& out) {
    SetCalculator calc;
    out << "String set calculator. S1 and S2 are sets of strings in binary search trees.\n";
    print_commands(out);
    while (true) {
        out << "\nCommand: " << std::flush;
        const std::optional<std::string> line = read_line(in);
        if (!line) {
            out << "\nEnd of input.\n";
            quit(calc, out);
            return 0;
        }
        const ParseResult parsed = parse_command(*line);
        if (std::holds_alternative<std::monostate>(parsed)) {
            continue;
        }
        if (const auto* error = std::get_if<ParseError>(&parsed)) {
            out << error->message << '\n';
            continue;
        }
        Command command = std::get<Command>(parsed);
        if (command.kind == CommandKind::Quit) {
            quit(calc, out);
            return 0;
        }
        if (command.kind == CommandKind::Help) {
            print_commands(out);
            continue;
        }
        if (command.kind == CommandKind::Add && command.argument.empty()) {
            out << "String to add: " << std::flush;
            const std::optional<std::string> value = read_line(in);
            if (!value) {
                out << "\nEnd of input.\n";
                quit(calc, out);
                return 0;
            }
            command.argument = std::string{trim(*value)};
        }
        out << calc.run(command).message << '\n';
        print_sets(calc, out);
    }
}

}  // namespace strset
