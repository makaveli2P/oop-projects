#include "theme.hpp"

namespace sal::tui::theme {

using namespace ftxui;

Color muted() { return Color::GrayDark; }
Color source() { return Color::Cyan; }
Color target() { return Color::Yellow; }
Color ok() { return Color::Green; }
Color alert() { return Color::Red; }

Element pane(const std::string& title, Element content) {
    // borderStyled() colors only the frame cells, so the content keeps the
    // default color of the terminal. The title is drawn over the top edge.
    Element frame = std::move(content) | borderStyled(ROUNDED, muted());
    // A dbox child gets the full box of the pane, so the vbox keeps the
    // colored heading in the top row only.
    Element heading = vbox({hbox({text("╭─ " + title + " ")}) | color(muted()), filler()});
    return dbox({std::move(frame), std::move(heading)});
}

Element faint(const std::string& value) { return text(value) | color(muted()); }

}  // namespace sal::tui::theme
