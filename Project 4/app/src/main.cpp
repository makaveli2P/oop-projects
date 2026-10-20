// main.cpp - the entry point of "ali", the SAL assembly language interpreter.
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "plain_mode.hpp"
#include "tui_mode.hpp"

namespace {

void print_usage(std::ostream& out) {
    out << "Usage: ali [--plain] [file]\n"
           "  --plain   Use the classic text mode (s = step, a = run all, q = quit).\n"
           "  file      A SAL program. Without a file, ali shows the programs in ./programs\n"
           "            (the classic mode asks for a file name).\n"
           "Without --plain, ali starts the terminal visualizer.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    // The hidden developer mode: render one frame of the visualizer as text.
    if (argc > 1 && std::string_view{argv[1]} == "--render") {
        return ali::run_render(std::vector<std::string_view>(argv + 2, argv + argc));
    }
    bool plain = false;
    std::optional<std::string> file;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--plain") {
            plain = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            return 0;
        } else if (arg.size() > 1 && arg.front() == '-') {
            std::cerr << "ali: unknown option '" << arg << "'\n";
            print_usage(std::cerr);
            return 2;
        } else if (!file) {
            file = std::string{arg};
        } else {
            std::cerr << "ali: give only one file\n";
            return 2;
        }
    }
    if (plain) {
        return ali::run_plain(file, std::cin, std::cout, std::cerr);
    }
    return ali::run_tui(file);
}
