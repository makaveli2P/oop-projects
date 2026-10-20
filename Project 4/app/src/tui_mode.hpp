// tui_mode.hpp - the default mode: the terminal visualizer.
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ali {

/// Runs the terminal visualizer. Returns the process exit code.
int run_tui(const std::optional<std::string>& file);

/// The hidden developer mode "ali --render <file> [--steps N] [--ops N]
/// [--keys KEYS] [--size WxH] [--color]". It prints one frame of the UI as
/// text and exits. `args` are the arguments after "--render".
int run_render(const std::vector<std::string_view>& args);

}  // namespace ali
