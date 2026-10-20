// tui_mode_stub.cpp - used when STRSET_BUILD_TUI is OFF: no visualizer.
#include <iostream>

#include "tui_mode.h"

namespace strset_app {

int run_tui() {
    std::cerr << "strset: this build has no terminal visualizer (STRSET_BUILD_TUI is OFF).\n"
                 "Use the classic mode: strset --plain\n";
    return 2;
}

int run_render(const std::vector<std::string_view>& /*args*/) { return run_tui(); }

}  // namespace strset_app
