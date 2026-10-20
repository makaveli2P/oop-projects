#include "sal_tui/format.hpp"

#include <algorithm>
#include <cstdio>

namespace sal::tui {

std::string format_word(std::int32_t value, NumberBase base) {
    if (base == NumberBase::Decimal) {
        return std::to_string(value);
    }
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "0x%08X", static_cast<unsigned>(static_cast<std::uint32_t>(value)));
    return buffer;
}

std::string format_address(std::uint8_t value, NumberBase base) {
    if (base == NumberBase::Decimal) {
        return std::to_string(value);
    }
    char buffer[8];
    std::snprintf(buffer, sizeof buffer, "0x%02X", unsigned{value});
    return buffer;
}

std::size_t display_width(std::string_view text) noexcept {
    std::size_t width = 0;
    for (const char c : text) {
        // Count the bytes that start a code point; skip continuation bytes.
        if ((static_cast<unsigned char>(c) & 0xC0U) != 0x80U) {
            ++width;
        }
    }
    return width;
}

std::string truncate(std::string_view text, std::size_t width) {
    if (display_width(text) <= width) {
        return std::string{text};
    }
    if (width == 0) {
        return {};
    }
    std::string result;
    std::size_t columns = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const bool starts = (static_cast<unsigned char>(text[i]) & 0xC0U) != 0x80U;
        if (starts) {
            if (columns + 1 == width) {
                break;
            }
            ++columns;
        }
        result += text[i];
    }
    return result + "…";
}

std::string pad_left(std::string_view text, std::size_t width) {
    const std::size_t used = display_width(text);
    return used >= width ? std::string{text} : std::string(width - used, ' ') + std::string{text};
}

std::string pad_right(std::string_view text, std::size_t width) {
    const std::size_t used = display_width(text);
    return used >= width ? std::string{text} : std::string{text} + std::string(width - used, ' ');
}

}  // namespace sal::tui

namespace sal::tui {

std::string speed_text(std::chrono::milliseconds interval) {
    const auto ms = std::max<std::chrono::milliseconds::rep>(interval.count(), 1);
    const auto tenths = (10000 + ms / 2) / ms;  // The rate in tenths, rounded.
    if (tenths % 10 == 0 || tenths >= 100) {
        return std::to_string((tenths + 5) / 10);
    }
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}

}  // namespace sal::tui
