// tui_mode.h - the default mode: the terminal visualizer.
#pragma once

#include <string_view>
#include <vector>

namespace strset_app {

/// Runs the terminal visualizer. Returns the process exit code.
int run_tui();

/// The hidden developer mode "strset --render [--setup CMDS] [--keys KEYS]
/// [--ticks N] [--size WxH] [--color]". It prints one frame of the UI as
/// text and exits. `args` are the arguments after "--render".
int run_render(const std::vector<std::string_view>& args);

}  // namespace strset_app
