// tui_mode.cpp - connects "ali" to the visualizer library sal_tui.
#include "tui_mode.hpp"

#include <charconv>
#include <iostream>
#include <string_view>

#include "sal_tui/app.hpp"

namespace ali {

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

}  // namespace

int run_tui(const std::optional<std::string>& file) { return sal::tui::run(file); }

int run_render(const std::vector<std::string_view>& args) {
    sal::tui::RenderRequest request;
    bool have_file = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        const bool has_value = i + 1 < args.size();
        if (arg == "--ops" && has_value && parse_size(args[i + 1], request.ops)) {
            ++i;
        } else if (arg == "--steps" && has_value && parse_size(args[i + 1], request.steps)) {
            ++i;
        } else if (arg == "--keys" && has_value) {
            request.keys = std::string{args[++i]};
        } else if (arg == "--size" && has_value && parse_dimensions(args[i + 1], request.width, request.height)) {
            ++i;
        } else if (arg == "--color") {
            request.color = true;
        } else if (!have_file && !arg.empty() && arg.front() != '-') {
            request.file = std::string{arg};
            have_file = true;
        } else {
            std::cerr << "ali: bad --render argument '" << arg << "'\n";
            return 2;
        }
    }
    if (!have_file) {
        std::cerr << "ali: --render needs a file\n";
        return 2;
    }
    std::cout << sal::tui::render_frame(request);
    return 0;
}

}  // namespace ali
