// main.cpp - the entry point of "strset", the string set calculator.
#include <iostream>
#include <string_view>
#include <vector>

#include "strset/classic.h"
#include "tui_mode.h"

namespace {

void print_usage(std::ostream& out) {
    out << "Usage: strset [--plain]\n"
           "  --plain   Use the classic command loop of the course project.\n"
           "Without --plain, strset starts the terminal visualizer.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    // The hidden developer mode: render one frame of the visualizer as text.
    if (argc > 1 && std::string_view{argv[1]} == "--render") {
        return strset_app::run_render(std::vector<std::string_view>(argv + 2, argv + argc));
    }
    bool plain = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--plain") {
            plain = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            return 0;
        } else {
            std::cerr << "strset: unknown argument '" << arg << "'\n";
            print_usage(std::cerr);
            return 2;
        }
    }
    if (plain) {
        return strset::run_classic(std::cin, std::cout);
    }
    return strset_app::run_tui();
}
