// app.h - the entry points of the visualizer. The header has no FTXUI types.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace strset::tui {

/// Runs the interactive visualizer. Returns the process exit code.
int run();

/// The options of the --render developer mode.
struct RenderRequest {
    /// Command lines to run first, without animation, for example "a pear".
    std::vector<std::string> setup;
    /// Then, a key script (see parse_key_script()).
    std::string keys;
    /// Then, animation ticks: each tick shows one more step of a run.
    std::size_t ticks = 0;
    int width = 120;
    int height = 36;
    bool color = false;  ///< Keep the ANSI escape codes.
};

/// Renders one frame of the complete UI and returns it as text. Throws
/// std::invalid_argument for a bad setup command or a bad key script.
[[nodiscard]] std::string render_frame(const RenderRequest& request);

}  // namespace strset::tui
