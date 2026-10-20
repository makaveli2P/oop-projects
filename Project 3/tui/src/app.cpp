// app.cpp - the interactive visualizer: the FTXUI loop, the keys and the
// animation ticker.
//
// Threads: the FTXUI loop runs on the main thread and owns the controller.
// The ticker thread never touches the controller. It only calls
// App::PostEvent() with a private tick event, which is thread-safe. The loop
// receives the event on the main thread and calls Controller::tick(). On
// quit, the loop exits first, then the ticker stops and joins, and then the
// objects are destroyed in reverse order.
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "strset_tui/app.h"
#include "strset_tui/controller.h"
#include "strset_tui/keys.h"
#include "strset_tui/ticker.h"
#include "views.h"

namespace strset::tui {

namespace {

using namespace ftxui;

/// The event that the ticker posts. It has its own name, so that no other
/// event advances the run.
const ftxui::Event& tick_event() {
    static const ftxui::Event event = ftxui::Event::Special("\x1b[strset-tick]");
    return event;
}

/// The name that handle_key() uses for an FTXUI event, or "" for no key.
std::string key_name(const ftxui::Event& event) {
    if (event == ftxui::Event::Return) return "Enter";
    if (event == ftxui::Event::Escape) return "Escape";
    if (event == ftxui::Event::Backspace) return "Backspace";
    if (event == ftxui::Event::ArrowLeft) return "Left";
    if (event == ftxui::Event::ArrowRight) return "Right";
    if (event == ftxui::Event::End) return "End";
    if (event.is_character()) return event.character();
    return {};
}

}  // namespace

int run() {
    Controller controller;
    auto screen = App::Fullscreen();
    // The ticker is declared after the screen, so it is destroyed first.
    Ticker ticker{[&screen] { screen.PostEvent(tick_event()); }};
    const auto sync_ticker = [&] { ticker.set_active(controller.running(), controller.tick_interval()); };

    Component view = Renderer([&] {
        const Dimensions size = Terminal::Size();
        return main_screen(controller, size.dimx, size.dimy);
    });
    view |= CatchEvent([&](const ftxui::Event& event) {
        if (event == tick_event()) {
            controller.tick();
            sync_ticker();
            return true;
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

}  // namespace strset::tui
