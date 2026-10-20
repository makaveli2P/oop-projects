// app.hpp - the entry points of the visualizer. The header has no FTXUI types.
#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace sal::tui {

/// Runs the interactive visualizer. Without a file, the visualizer shows
/// the .sal files in ./programs (or asks for a file name). Returns the
/// process exit code.
int run(const std::optional<std::string>& file);

/// The options of the --render developer mode.
struct RenderRequest {
    std::string file;
    std::size_t steps = 0;   ///< Complete instructions to execute first.
    std::size_t ops = 0;     ///< Then, micro-operations to show.
    std::string keys;        ///< Then, keys to apply, one character each.
    int width = 120;
    int height = 36;
    bool color = false;      ///< Keep the ANSI escape codes.
};

/// Renders one frame of the complete UI and returns it as text.
[[nodiscard]] std::string render_frame(const RenderRequest& request);

}  // namespace sal::tui
