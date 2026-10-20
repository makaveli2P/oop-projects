#include "sal_tui/keys.hpp"

namespace sal::tui {

KeyResult handle_key(Controller& c, std::string_view key) {
    if (key == " " || key == "n") {
        c.pause();
        c.next_micro_op();
    } else if (key == "s") {
        c.pause();
        c.step_instruction();
    } else if (key == "b") {
        c.back();
    } else if (key == "r") {
        c.toggle_run();
    } else if (key == "+" || key == "=") {
        c.faster();
    } else if (key == "-" || key == "_") {
        c.slower();
    } else if (key == "R") {
        c.reset();
    } else if (key == "g") {
        c.run_to_end();
    } else if (key == "x") {
        c.toggle_base();
    } else if (key == "Enter" || key == "\n" || key == "\r") {
        c.toggle_breakpoint(c.selection());
    } else if (key == "Up" || key == "k") {
        c.move_selection(-1);
    } else if (key == "Down" || key == "j") {
        c.move_selection(+1);
    } else if (key == "q" || key == "Escape") {
        c.pause();
        return KeyResult::Quit;
    } else {
        return KeyResult::Ignored;
    }
    return KeyResult::Handled;
}

}  // namespace sal::tui
