// render.cpp - the --render developer mode: one frame of the UI as text.
#include <stdexcept>

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "strset/command.h"
#include "strset_tui/app.h"
#include "strset_tui/controller.h"
#include "strset_tui/keys.h"
#include "views.h"

namespace strset::tui {

namespace {

/// Removes the ANSI escape sequences (CSI and OSC) and the spaces at the
/// ends of the lines.
std::string plain_text(const std::string& input) {
    std::string out;
    for (std::size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (ch == '\x1b' && i + 1 < input.size()) {
            if (input[i + 1] == '[') {
                i += 2;
                while (i < input.size() && !(input[i] >= '@' && input[i] <= '~')) ++i;
                continue;
            }
            if (input[i + 1] == ']') {
                i += 2;
                while (i < input.size() && input[i] != '\x07' &&
                       !(input[i] == '\x1b' && i + 1 < input.size() && input[i + 1] == '\\')) {
                    ++i;
                }
                if (i < input.size() && input[i] == '\x1b') ++i;
                continue;
            }
        }
        if (ch == '\r') continue;
        out += ch;
    }
    std::string result;
    std::size_t start = 0;
    while (start <= out.size()) {
        const std::size_t end = out.find('\n', start);
        std::string line = out.substr(start, end == std::string::npos ? std::string::npos : end - start);
        while (!line.empty() && line.back() == ' ') line.pop_back();
        result += line;
        if (end == std::string::npos) break;
        result += '\n';
        start = end + 1;
    }
    return result + "\n";
}

}  // namespace

std::string render_frame(const RenderRequest& request) {
    Controller controller;
    for (const std::string& line : request.setup) {
        const ParseResult parsed = parse_command(line);
        if (const auto* error = std::get_if<ParseError>(&parsed)) {
            throw std::invalid_argument("setup command '" + line + "': " + error->message);
        }
        if (const auto* command = std::get_if<Command>(&parsed)) {
            controller.run_quietly(*command);
        }
    }
    for (const std::string& key : parse_key_script(request.keys)) {
        if (handle_key(controller, key) == KeyResult::Quit) break;
    }
    for (std::size_t i = 0; i < request.ticks; ++i) controller.tick();

    const ftxui::Element frame = main_screen(controller, request.width, request.height);
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(request.width), ftxui::Dimension::Fixed(request.height));
    ftxui::Render(screen, frame);
    const std::string output = screen.ToString();
    return request.color ? output + "\n" : plain_text(output);
}

}  // namespace strset::tui
