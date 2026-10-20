// tui_mode.cpp - connects "strset" to the visualizer library strset_tui.
#include "tui_mode.h"

#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string>

#include "strset_tui/app.h"

namespace strset_app {

namespace {

bool parse_size(std::string_view text, std::size_t& value) {
    const auto* end = text.data() + text.size();
    const auto [ptr, error] = std::from_chars(text.data(), end, value);
    return error == std::errc{} && ptr == end;
}

bool parse_dimensions(std::string_view text, int& width, int& height) {
    const auto x = text.find('x');
    std::size_t w = 0;
    std::size_t h = 0;
    if (x == std::string_view::npos || !parse_size(text.substr(0, x), w) || !parse_size(text.substr(x + 1), h) ||
        w < 10 || h < 5 || w > 1000 || h > 1000) {
        return false;
    }
    width = static_cast<int>(w);
    height = static_cast<int>(h);
    return true;
}

/// Splits "a pear;a apple;s" at each ';'.
std::vector<std::string> split_commands(std::string_view text) {
    std::vector<std::string> commands;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(';', start);
        commands.emplace_back(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return commands;
}

}  // namespace

int run_tui() { return strset::tui::run(); }

int run_render(const std::vector<std::string_view>& args) {
    strset::tui::RenderRequest request;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        const bool has_value = i + 1 < args.size();
        if (arg == "--setup" && has_value) {
            for (std::string& command : split_commands(args[++i])) request.setup.push_back(std::move(command));
        } else if (arg == "--keys" && has_value) {
            request.keys += std::string{args[++i]};
        } else if (arg == "--ticks" && has_value && parse_size(args[i + 1], request.ticks)) {
            ++i;
        } else if (arg == "--size" && has_value && parse_dimensions(args[i + 1], request.width, request.height)) {
            ++i;
        } else if (arg == "--color") {
            request.color = true;
        } else {
            std::cerr << "strset: bad --render argument '" << arg << "'\n";
            return 2;
        }
    }
    try {
        std::cout << strset::tui::render_frame(request);
    } catch (const std::invalid_argument& error) {
        std::cerr << "strset: " << error.what() << '\n';
        return 2;
    }
    return 0;
}

}  // namespace strset_app
