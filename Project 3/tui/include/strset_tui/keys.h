// keys.h - the key bindings of the visualizer.
//
// The function maps a key name to a controller command. The TUI and the
// --render developer flag use the same function, and the tests use it too.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "strset_tui/controller.h"

namespace strset::tui {

enum class KeyResult : std::uint8_t { Ignored, Handled, Quit };

/// Applies one key. `key` is one UTF-8 character ("a", " ", "+") or one of
/// the names "Enter", "Escape", "Backspace", "Left", "Right" and "End".
KeyResult handle_key(Controller& controller, std::string_view key);

/// Splits a --render key script into key names. "{enter}", "{esc}",
/// "{bs}", "{left}", "{right}", "{end}" and "{space}" are named keys. Each
/// other UTF-8 character is one key. Throws std::invalid_argument for an
/// unknown "{name}".
[[nodiscard]] std::vector<std::string> parse_key_script(std::string_view script);

}  // namespace strset::tui
