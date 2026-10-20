// diagram.hpp - the datapath diagram: the hardware units on one internal bus.
//
//   ┌ PC ──┐   ┌ Memory ──┐   ┌ IR ──────┐   ┌ Decoder ─┐
//   │    3 │   │ [128] X  │   │ LDA X    │   │ LDA X    │
//   └──┬───┘   └────┬─────┘   └────┬─────┘   └────┬─────┘
//      │            │              │              │
//   ───┴────────────┴─── 7 ▶ ──┬───┴──────────────┴───   (the bus)
//      │            │          │       │      │
//   ┌──┴───┐   ┌────┴────┐   ┌─┴──┐   ┌┴┐    ┌┴┐
//   │    7 │   │       0 │   │ +  │   │0│    │0│
//   └ A ───┘   └ B ──────┘   └ALU─┘   └Z┘    └V┘
//
// The active micro-operation colors its source units (cyan), its target units
// (amber), and the bus between them, and writes the moving value on the bus.
#pragma once

#include <ftxui/dom/elements.hpp>

#include "sal_tui/controller.hpp"

namespace sal::tui {

/// The number of rows of the diagram.
inline constexpr int kDiagramHeight = 11;

/// The smallest width that the diagram needs.
inline constexpr int kDiagramMinWidth = 48;

/// Draws the diagram for the current state of `controller` in `width` columns.
[[nodiscard]] ftxui::Element datapath_diagram(const Controller& controller, int width);

}  // namespace sal::tui
