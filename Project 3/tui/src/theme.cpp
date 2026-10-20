#include "theme.h"

#include <utility>

namespace strset::tui::theme {

using namespace ftxui;

Color color_of(Tone tone) {
    switch (tone) {
        case Tone::Muted: return Color::GrayDark;
        case Tone::Source: return Color::Cyan;
        case Tone::Target: return Color::Yellow;
        case Tone::Ok: return Color::Green;
        case Tone::Alert: return Color::Red;
        case Tone::Plain: break;
    }
    return Color::Default;
}

Element styled(const std::string& value, Tone tone, bool bold_text) {
    Element element = text(value);
    if (tone != Tone::Plain) {
        element = element | color(color_of(tone));
    }
    if (bold_text) {
        element = element | bold;
    }
    return element;
}

Element faint(const std::string& value) { return text(value) | color(Color::GrayDark); }

Element pane(Element title, Element content) {
    // borderStyled() colors only the frame cells, so the content keeps its
    // own colors. The title is drawn over the top edge. A dbox child gets the
    // full box of the pane, so the vbox keeps the title in the top row only.
    Element frame = std::move(content) | borderStyled(ROUNDED, Color::GrayDark);
    Element heading = vbox({hbox({faint("╭─ "), std::move(title), text(" ")}), filler()});
    return dbox({std::move(frame), std::move(heading)});
}

}  // namespace strset::tui::theme
