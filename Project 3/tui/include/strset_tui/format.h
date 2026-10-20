// format.h - text helpers for the visualizer. No FTXUI code.
#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace strset::tui {

/// The number of terminal columns in UTF-8 text. Each code point counts as
/// one column. Wide characters (for example CJK) are not handled.
[[nodiscard]] std::size_t display_width(std::string_view text) noexcept;

/// Cuts `text` to `width` columns. A cut text ends with "…".
[[nodiscard]] std::string truncate(std::string_view text, std::size_t width);

/// Adds spaces on the right (left-aligns) or on the left (right-aligns).
[[nodiscard]] std::string pad_right(std::string_view text, std::size_t width);
[[nodiscard]] std::string pad_left(std::string_view text, std::size_t width);

/// The code points of UTF-8 text, one string each. A control character
/// becomes "·", so that it cannot change the terminal.
[[nodiscard]] std::vector<std::string> glyphs(std::string_view text);

/// `text` without its last code point.
[[nodiscard]] std::string drop_last_glyph(std::string_view text);

/// The animation rate for a tick interval, in steps each second: "2" for
/// 500 ms, "1.4" for 700 ms.
[[nodiscard]] std::string speed_text(std::chrono::milliseconds interval);

}  // namespace strset::tui
