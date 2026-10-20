// format.hpp - number and text formats for the visualizer.
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace sal::tui {

/// The base that the visualizer uses to show words. The x key changes it.
enum class NumberBase : std::uint8_t { Decimal, Hex };

/// A 32-bit word: "-1293967296" or "0xB2DE1200" (two's complement).
[[nodiscard]] std::string format_word(std::int32_t value, NumberBase base);

/// An 8-bit address: "12" or "0x0C".
[[nodiscard]] std::string format_address(std::uint8_t value, NumberBase base);

/// The number of terminal columns in UTF-8 text. Each code point counts as
/// one column. This is correct for the text that the visualizer uses.
[[nodiscard]] std::size_t display_width(std::string_view text) noexcept;

/// Cuts `text` to `width` columns. A cut text ends with "…".
[[nodiscard]] std::string truncate(std::string_view text, std::size_t width);

/// Adds spaces on the left (right-aligns) or on the right (left-aligns).
[[nodiscard]] std::string pad_left(std::string_view text, std::size_t width);
[[nodiscard]] std::string pad_right(std::string_view text, std::size_t width);

/// The animation rate for a tick interval, in micro-operations each second:
/// "5" for 200 ms, "1.7" for 600 ms. A rate below 10 that is not a whole
/// number shows one decimal place, so each speed level has its own label.
[[nodiscard]] std::string speed_text(std::chrono::milliseconds interval);

}  // namespace sal::tui
