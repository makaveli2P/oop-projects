// theme.hpp - the few colors of the visualizer and what each one means.
//
// - Muted gray: frames, labels and inactive parts.
// - Cyan: the source of the active data movement (a unit or a cell that is read).
// - Amber (yellow): the target of the active data movement, the path, and
//   the values that changed in this step.
// - Green: the machine halted normally (HLT).
// - Red: breakpoints and runtime faults.
// All other text uses the default color of the terminal.
#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace sal::tui::theme {

[[nodiscard]] ftxui::Color muted();
[[nodiscard]] ftxui::Color source();
[[nodiscard]] ftxui::Color target();
[[nodiscard]] ftxui::Color ok();
[[nodiscard]] ftxui::Color alert();

/// A pane: a rounded, muted frame with a title.
[[nodiscard]] ftxui::Element pane(const std::string& title, ftxui::Element content);

/// Text in the muted color.
[[nodiscard]] ftxui::Element faint(const std::string& text);

}  // namespace sal::tui::theme
