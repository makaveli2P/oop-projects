// views.h - the screens of the visualizer. The views only read the
// controller; they never change it.
#pragma once

#include <ftxui/dom/elements.hpp>

#include "strset_tui/controller.h"

namespace strset::tui {

/// The smallest terminal that the main screen accepts.
inline constexpr int kMinWidth = 80;
inline constexpr int kMinHeight = 20;

/// The complete main screen for a terminal of width x height.
[[nodiscard]] ftxui::Element main_screen(const Controller& controller, int width, int height);

/// The screen for a terminal that is too small.
[[nodiscard]] ftxui::Element small_screen(int width, int height);

}  // namespace strset::tui
