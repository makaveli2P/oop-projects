// app.cpp - the interactive visualizer: the FTXUI loop, the keys, the mouse
// and the animation ticker.
//
// Threads: the FTXUI loop runs on the main thread and owns the controller.
// The ticker thread never touches the controller. It only calls
// App::PostEvent() with a private tick event, which is thread-safe. The loop receives
// the event on the main thread and calls Controller::tick(). On quit, the
// loop exits first, then the ticker stops and joins, and then the objects
// are destroyed in reverse order.
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "load.hpp"
#include "sal_tui/app.hpp"
#include "sal_tui/controller.hpp"
#include "sal_tui/keys.hpp"
#include "sal_tui/ticker.hpp"
#include "theme.hpp"
#include "views.hpp"

namespace sal::tui {

namespace {

using namespace ftxui;

/// The event that the ticker posts. It has its own name, so that no other
/// event (FTXUI posts Event::Custom itself in some cases) advances the run.
const Event& tick_event() {
    static const Event event = Event::Special("\x1b[sal-tick]");
    return event;
}

/// The name that handle_key() uses for an FTXUI event, or "" for no key.
std::string key_name(const Event& event) {
    if (event == Event::Return) return "Enter";
    if (event == Event::Escape) return "Escape";
    if (event == Event::ArrowUp) return "Up";
    if (event == Event::ArrowDown) return "Down";
    if (event.is_character()) return event.character();
    return {};
}

/// The .sal files in ./programs and in the current folder, sorted.
std::vector<std::string> find_programs() {
    std::vector<std::string> files;
    for (const char* folder : {"programs", "."}) {
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator{folder, error}) {
            if (entry.is_regular_file(error) && entry.path().extension() == ".sal") {
                files.push_back(entry.path().lexically_normal().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

/// Shows a list of programs. Returns the chosen file, or "" if the user quits.
std::string pick_program(const std::vector<std::string>& files) {
    auto screen = App::Fullscreen();
    int selected = 0;
    std::string chosen;
    MenuOption option = MenuOption::Vertical();
    option.on_enter = [&] {
        chosen = files[static_cast<std::size_t>(selected)];
        screen.Exit();
    };
    Component menu = Menu(&files, &selected, option);
    Component view = Renderer(menu, [&] {
        Element card = vbox({
            text("ALI") | bold,
            theme::faint("Choose a SAL program."),
            text(""),
            menu->Render() | vscroll_indicator | yframe | size(HEIGHT, LESS_THAN, 20),
            text(""),
            hbox({text("⏎") | bold, theme::faint(" open   "), text("q") | bold, theme::faint(" quit")}),
        });
        return card | size(WIDTH, GREATER_THAN, 40) | center;
    });
    view |= CatchEvent([&](const Event& event) {
        if (event == Event::Character('q') || event == Event::Escape) {
            screen.Exit();
            return true;
        }
        return false;
    });
    screen.Loop(view);
    return chosen;
}

int show_errors(const LoadedFile& loaded) {
    auto screen = App::Fullscreen();
    Component view = Renderer([&] {
        const Dimensions size = Terminal::Size();
        return error_screen(loaded.file_name, loaded.result.errors(), loaded.lines, size.dimx, size.dimy);
    });
    view |= CatchEvent([&](const Event& event) {
        if (event == Event::Character('q') || event == Event::Escape || event == Event::Return) {
            screen.Exit();
            return true;
        }
        return false;
    });
    screen.Loop(view);
    // Also print the errors, so that they stay in the terminal after exit.
    for (const sal::ParseError& e : loaded.result.errors()) {
        std::cerr << loaded.file_name << ": " << sal::to_string(e) << '\n';
    }
    return 1;
}

int visualize(const LoadedFile& loaded) {
    Controller controller{loaded.result.program()};
    std::vector<Box> source_rows;
    const ViewContext context{loaded.file_name, &source_rows};

    auto screen = App::Fullscreen();
    // The ticker is declared after the screen, so it is destroyed first.
    Ticker ticker{[&screen] { screen.PostEvent(tick_event()); }};
    const auto sync_ticker = [&] { ticker.set_active(controller.running(), controller.tick_interval()); };

    Component view = Renderer([&] {
        const Dimensions size = Terminal::Size();
        return main_screen(controller, context, size.dimx, size.dimy);
    });
    view |= CatchEvent([&](Event event) {
        if (event == tick_event()) {
            controller.tick();
            sync_ticker();
            return true;
        }
        if (event.is_mouse()) {
            const Mouse& mouse = event.mouse();
            if (mouse.button != Mouse::Left || mouse.motion != Mouse::Pressed) {
                return false;
            }
            for (std::size_t address = 0; address < source_rows.size(); ++address) {
                if (source_rows[address].Contain(mouse.x, mouse.y)) {
                    const auto byte = static_cast<std::uint8_t>(address);
                    controller.select(byte);
                    controller.toggle_breakpoint(byte);
                    return true;
                }
            }
            return false;
        }
        const std::string key = key_name(event);
        if (key.empty()) {
            return false;
        }
        const KeyResult result = handle_key(controller, key);
        sync_ticker();
        if (result == KeyResult::Quit) {
            ticker.set_active(false, controller.tick_interval());
            screen.Exit();
        }
        return result != KeyResult::Ignored;
    });

    screen.Loop(view);
    ticker.stop();  // Join the thread before the controller and the screen go away.
    return 0;
}

}  // namespace

int run(const std::optional<std::string>& file) {
    std::string path;
    if (file) {
        path = *file;
    } else {
        const std::vector<std::string> programs = find_programs();
        if (!programs.empty()) {
            path = pick_program(programs);
            if (path.empty()) {
                return 0;
            }
        } else {
            std::cout << "Enter the name of a SAL program file: " << std::flush;
            if (!std::getline(std::cin, path) || path.empty()) {
                std::cerr << "ali: no file name\n";
                return 1;
            }
        }
    }
    const LoadedFile loaded = load_file(path);
    if (!loaded.result) {
        return show_errors(loaded);
    }
    return visualize(loaded);
}

}  // namespace sal::tui
