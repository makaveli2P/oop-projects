#include "strset_tui/format.h"

#include <algorithm>

namespace strset::tui {

namespace {

bool starts_glyph(char ch) noexcept { return (static_cast<unsigned char>(ch) & 0xC0U) != 0x80U; }

}  // namespace

std::size_t display_width(std::string_view text) noexcept {
    return static_cast<std::size_t>(std::count_if(text.begin(), text.end(), starts_glyph));
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
    for (const char ch : text) {
        if (starts_glyph(ch)) {
            if (columns + 1 == width) {
                break;
            }
            ++columns;
        }
        result += ch;
    }
    return result + "…";
}

std::string pad_right(std::string_view text, std::size_t width) {
    const std::size_t used = display_width(text);
    return used >= width ? std::string{text} : std::string{text} + std::string(width - used, ' ');
}

std::string pad_left(std::string_view text, std::size_t width) {
    const std::size_t used = display_width(text);
    return used >= width ? std::string{text} : std::string(width - used, ' ') + std::string{text};
}

std::vector<std::string> glyphs(std::string_view text) {
    std::vector<std::string> result;
    for (const char ch : text) {
        const auto byte = static_cast<unsigned char>(ch);
        if (starts_glyph(ch) || result.empty()) {
            if (byte < 0x20 || byte == 0x7F) {
                result.emplace_back("·");
                continue;
            }
            result.emplace_back(1, ch);
        } else {
            result.back() += ch;
        }
    }
    return result;
}

std::string drop_last_glyph(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    std::size_t end = text.size() - 1;
    while (end > 0 && !starts_glyph(text[end])) {
        --end;
    }
    return std::string{text.substr(0, end)};
}

std::string speed_text(std::chrono::milliseconds interval) {
    const auto ms = std::max<std::chrono::milliseconds::rep>(interval.count(), 1);
    const auto tenths = (10000 + ms / 2) / ms;  // The rate in tenths, rounded.
    if (tenths % 10 == 0 || tenths >= 100) {
        return std::to_string((tenths + 5) / 10);
    }
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}

}  // namespace strset::tui
