// views.hpp - the screens and panes of the visualizer. The views only read
// the controller; they never change it.
#pragma once

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "sal/assembler.hpp"
#include "sal_tui/controller.hpp"

namespace sal::tui {

/// The smallest terminal that the main screen accepts.
inline constexpr int kMinWidth = 100;
inline constexpr int kMinHeight = 30;

struct ViewContext {
    std::string file_name;
    /// When it is not null, the source view stores the screen box of each
    /// source line here, so that a mouse click can find the line.
    std::vector<ftxui::Box>* source_rows = nullptr;
};

/// The complete main screen for a terminal of width x height.
[[nodiscard]] ftxui::Element main_screen(const Controller& controller, const ViewContext& context,
                                         int width, int height);

/// The screen for a program that did not load.
[[nodiscard]] ftxui::Element error_screen(const std::string& file_name,
                                          const std::vector<sal::ParseError>& errors,
                                          const std::vector<std::string>& source_lines, int width,
                                          int height);

/// The screen for a terminal that is too small.
[[nodiscard]] ftxui::Element small_screen(int width, int height);

}  // namespace sal::tui
