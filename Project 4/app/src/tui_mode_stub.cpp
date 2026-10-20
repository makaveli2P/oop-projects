// tui_mode_stub.cpp - used when SAL_BUILD_TUI is OFF: no visualizer.
#include <iostream>

#include "tui_mode.hpp"

namespace ali {

int run_tui(const std::optional<std::string>& /*file*/) {
    std::cerr << "ali: this build has no terminal visualizer (SAL_BUILD_TUI is OFF).\n"
                 "Use the classic mode: ali --plain [file]\n";
    return 2;
}

int run_render(const std::vector<std::string_view>& /*args*/) { return run_tui(std::nullopt); }

}  // namespace ali
