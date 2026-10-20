// keys.hpp - the key bindings of the visualizer.
//
// The function maps a key name to a controller command. The TUI and the
// --render developer flag use the same function, and the tests use it too.
#pragma once

#include <cstdint>
#include <string_view>

#include "sal_tui/controller.hpp"

namespace sal::tui {

enum class KeyResult : std::uint8_t { Ignored, Handled, Quit };

/// Applies one key. `key` is one character ("n", " ", "R", "+") or one of
/// the names "Enter", "Escape", "Up" and "Down".
KeyResult handle_key(Controller& controller, std::string_view key);

}  // namespace sal::tui
