// render.cpp - the --render developer mode: one frame of the UI as text.
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "load.hpp"
#include "sal_tui/app.hpp"
#include "sal_tui/controller.hpp"
#include "sal_tui/keys.hpp"
#include "views.hpp"

namespace sal::tui {

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
    // Remove the spaces at the end of each line.
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
    const LoadedFile loaded = load_file(request.file);
    ftxui::Element frame;
    if (!loaded.result) {
        frame = error_screen(loaded.file_name, loaded.result.errors(), loaded.lines, request.width,
                             request.height);
    } else {
        Controller controller{loaded.result.program()};
        for (std::size_t i = 0; i < request.steps; ++i) controller.step_instruction();
        for (std::size_t i = 0; i < request.ops; ++i) controller.next_micro_op();
        for (const char key : request.keys) {
            const std::string name = (key == '\n' || key == 'E') ? "Enter" : std::string(1, key);
            if (handle_key(controller, name) == KeyResult::Quit) break;
        }
        frame = main_screen(controller, ViewContext{loaded.file_name, nullptr}, request.width,
                            request.height);
    }
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(request.width),
                                        ftxui::Dimension::Fixed(request.height));
    ftxui::Render(screen, frame);
    const std::string output = screen.ToString();
    return request.color ? output + "\n" : plain_text(output);
}

}  // namespace sal::tui
