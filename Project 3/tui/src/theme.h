// theme.h - the few colors of the visualizer and what each one means.
//
// - Muted gray: frames, connectors, labels and help text.
// - Cyan: the node that the current step examines or reads.
// - Amber (yellow): a new node, the search path and the changed links.
// - Green: a lookup found its string; a "yes" verdict.
// - Red: a node that is freed or removed; a "no" verdict.
// All other text uses the default color of the terminal.
#pragma once

#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "strset_tui/tree_layout.h"

namespace strset::tui::theme {

[[nodiscard]] ftxui::Color color_of(Tone tone);

/// Text in a tone. Plain text keeps the default color of the terminal.
[[nodiscard]] ftxui::Element styled(const std::string& text, Tone tone, bool bold = false);

/// Text in the muted color.
[[nodiscard]] ftxui::Element faint(const std::string& text);

/// A pane: a rounded, muted frame. `title` is drawn over the top edge.
[[nodiscard]] ftxui::Element pane(ftxui::Element title, ftxui::Element content);

}  // namespace strset::tui::theme
