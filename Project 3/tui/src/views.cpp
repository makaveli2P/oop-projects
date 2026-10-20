#include "views.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "strset_tui/format.h"
#include "strset_tui/tree_layout.h"
#include "theme.h"

namespace strset::tui {

namespace {

using namespace ftxui;

constexpr int kHeaderRows = 1;
constexpr int kBottomRows = 5;  // Narration, progress, a blank row, the prompt, the keys.

Element spacer(int columns) { return text(std::string(static_cast<std::size_t>(std::max(columns, 0)), ' ')); }

// ---- Header ----------------------------------------------------------------------

Element status_view(const Controller& c) {
    if (c.running()) {
        return text("● Running") | color(theme::color_of(Tone::Target)) | bold;
    }
    if (!c.has_trace()) {
        return text("Ready") | bold;
    }
    return text(c.at_end() ? "Done" : "Paused") | bold;
}

Element header(const Controller& c, int width) {
    // The subtitle is the command on the screen, or a short description.
    Element subtitle = theme::faint(" · string sets in binary search trees");
    if (c.has_trace() && c.command_kind()) {
        const auto room = static_cast<std::size_t>(std::max(width - 50, 10));
        const std::string command = truncate(c.command_text(), room / 2);
        const std::string summary = summary_of(*c.command_kind());
        subtitle = hbox({theme::faint(" · "), text(command) | bold,
                         theme::faint(truncate("   " + summary, room - display_width(command)))});
    }
    return hbox({text(" Sets") | bold, std::move(subtitle), filler(),
                 theme::faint("speed " + speed_text(c.tick_interval()) + " steps/s"), spacer(4), status_view(c),
                 spacer(1)});
}

// ---- Tree panes ------------------------------------------------------------------

/// One canvas row as runs of text with the same style.
Element canvas_row(const Canvas& canvas, int y) {
    Elements runs;
    std::string run;
    Tone tone = Tone::Plain;
    bool bold_run = false;
    const auto flush = [&] {
        if (!run.empty()) runs.push_back(theme::styled(run, tone, bold_run));
        run.clear();
    };
    for (int x = 0; x < canvas.width(); ++x) {
        const Cell& cell = canvas.at(x, y);
        const Tone cell_tone = cell.glyph == " " ? Tone::Plain : cell.tone;
        const bool cell_bold = cell.glyph != " " && cell.bold;
        if (cell_tone != tone || cell_bold != bold_run) {
            flush();
            tone = cell_tone;
            bold_run = cell_bold;
        }
        run += cell.glyph;
    }
    flush();
    return hbox(std::move(runs));
}

Element canvas_view(const Canvas& canvas) {
    Elements rows;
    for (int y = 0; y < canvas.height(); ++y) rows.push_back(canvas_row(canvas, y));
    return vbox(std::move(rows));
}

/// True if this step swaps the roots: the pane titles show it.
bool swapping(const Controller& c) {
    const Step* s = c.step();
    return s != nullptr && std::holds_alternative<event::Swap>(s->event);
}

Element tree_title(const Controller& c, SetName set, const Shape& shape) {
    const std::size_t n = shape.nodes.size();
    std::string details = " · " + std::to_string(n) + (n == 1 ? " string" : " strings");
    if (c.command_kind() == CommandKind::Switch || c.command_kind() == CommandKind::Copy) {
        details += shape.empty() ? " · root: none" : " · root #" + std::to_string(*shape.root());
    }
    const Tone name_tone = swapping(c) ? Tone::Target : Tone::Plain;
    return hbox({theme::styled(to_string(set), name_tone, true), theme::faint(details)});
}

Element tree_pane(const Controller& c, SetName set, int width, int height) {
    const Shape& shape = c.trees().of(set);
    const int canvas_width = width - 4;    // The frame and one column of padding on each side.
    const int canvas_height = height - 3;  // The frame and the hint row.
    Element body;
    std::string hint;
    if (shape.empty()) {
        body = theme::faint("empty") | center | flex;
    } else {
        const TreeMarks marks = c.marks(set);
        const TreeDrawing drawing =
            draw_tree(shape, marks, canvas_width, canvas_height, c.allow_top_down(set, canvas_width, canvas_height),
                      c.layout_reference(set));
        body = canvas_view(drawing.canvas);
        if (drawing.style == TreeStyle::Sideways) {
            hint = canvas_width >= 50 ? "left child ↑   right child ↓   sorted from the top"
                                      : "↑ left  ↓ right  sorted from the top";
        }
    }
    Element content = vbox({
        hbox({spacer(1), std::move(body) | size(WIDTH, EQUAL, canvas_width) | size(HEIGHT, EQUAL, canvas_height),
              spacer(1)}),
        hbox({spacer(1), theme::faint(truncate(hint, static_cast<std::size_t>(std::max(canvas_width, 0))))}),
    });
    return theme::pane(tree_title(c, set, shape), std::move(content)) | size(WIDTH, EQUAL, width) |
           size(HEIGHT, EQUAL, height);
}

// ---- Bottom rows -----------------------------------------------------------------

Tone narration_tone(const Step& step) {
    if (const auto* done = std::get_if<event::Done>(&step.event)) {
        if (done->verdict) return *done->verdict ? Tone::Ok : Tone::Alert;
        return Tone::Plain;
    }
    if (std::holds_alternative<event::Missing>(step.event)) return Tone::Alert;
    return Tone::Plain;
}

Element narration_row(const Controller& c, int width) {
    const Step* s = c.step();
    std::string position;
    if (s != nullptr) {
        position = "step " + std::to_string(c.cursor() + 1) + " of " + std::to_string(c.trace().steps.size());
    }
    const auto room = static_cast<std::size_t>(std::max(width - 4 - static_cast<int>(display_width(position)) - 4, 10));
    if (s == nullptr) {
        return hbox({spacer(2), text(truncate("Press a command key to start. Press a to add a string to S1.", room))});
    }
    const std::size_t max_value = std::clamp<std::size_t>(room / 5, 8, 24);
    const bool done = std::holds_alternative<event::Done>(s->event);
    const Tone tone = narration_tone(*s);
    return hbox({
        spacer(2),
        theme::styled(done ? "■ " : "▸ ", done ? tone : Tone::Target, true),
        theme::styled(truncate(describe(s->event, max_value), room), tone, done),
        filler(),
        theme::faint(position),
        spacer(2),
    });
}

Element progress_row(const Controller& c, int width) {
    const int room = width - 6;
    if (!c.notice().empty()) {
        return hbox({spacer(4), theme::styled(truncate(c.notice(), static_cast<std::size_t>(room)), Tone::Target)});
    }
    const std::vector<Token> tokens = c.progress();
    const auto width_of = [](const Token& token) { return static_cast<int>(display_width(token.text)); };
    int total = 0;
    for (const Token& token : tokens) total += width_of(token);

    // If the row is too long, keep the first token (the label, for example
    // "Checked: ") and the newest tokens at the end, with "… " between them.
    std::size_t first_kept = 1;
    if (total > room && tokens.size() > 1) {
        int used = width_of(tokens.front()) + 2;
        first_kept = tokens.size();
        while (first_kept > 1 && used + width_of(tokens[first_kept - 1]) <= room) {
            used += width_of(tokens[--first_kept]);
        }
        // No separator after "…".
        if (first_kept < tokens.size() && (tokens[first_kept].text == ", " || tokens[first_kept].text == " → ")) {
            ++first_kept;
        }
    }
    Elements parts{spacer(4)};
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0 && i < first_kept) {
            if (i == 1) parts.push_back(theme::faint("… "));
            continue;
        }
        const Token& token = tokens[i];
        parts.push_back(theme::styled(truncate(token.text, static_cast<std::size_t>(std::max(room, 1))), token.tone,
                                      token.bold));
    }
    return hbox(std::move(parts));
}

Element prompt_row(const Controller& c, int width) {
    if (c.prompt_open()) {
        return hbox({
            spacer(2),
            text("Add to S1 › ") | bold,
            text(c.prompt_text()),
            text("▏") | color(theme::color_of(Tone::Target)),
            filler(),
            theme::faint("⏎ add   esc cancel"),
            spacer(2),
        });
    }
    const std::string gap = width >= 90 ? "   " : "  ";
    const auto item = [&gap](const char* key, const char* word) {
        return hbox({text(key) | bold, theme::faint(std::string{" "} + word + gap)});
    };
    return hbox({
        spacer(2),
        theme::faint("› "),
        item("a", "add"),
        item("e", "erase"),
        item("s", "switch"),
        item("b", "subset"),
        item("c", "copy"),
        item("l", "list"),
        item("u", "union"),
        item("i", "intersect"),
    });
}

Element keys_row() {
    return hbox({spacer(2), theme::faint("space step   ← back   r run   g end   +/- speed   ? help   q quit")});
}

/// The help replaces the two tree panes while it is open.
Element help_pane(int width, int height) {
    const auto row = [](const std::string& key, const std::string& meaning) {
        return hbox({text(pad_right(key, 10)) | bold, theme::faint(meaning)});
    };
    const bool roomy = height >= 26;
    Elements rows{
        text("Commands") | bold,
        row("a", "Add a string to S1. Type it, then press Enter."),
        row("e", "Erase S1: free its nodes, leaves first."),
        row("s", "Switch S1 and S2: swap the root pointers."),
        row("b", "Is S2 a proper subset of S1?"),
        row("c", "Copy S1 into S2, node by node (a deep copy)."),
        row("l", "List S1 and S2 in alphabetical order."),
        row("u", "Union: S1 = S1 ∪ S2."),
        row("i", "Intersection: S1 = S1 ∩ S2."),
        text(""),
        text("Playback") | bold,
        row("space  →", "Show the next step."),
        row("←", "Show the previous step."),
        row("r", "Run or pause the animation."),
        row("g", "Go to the last step."),
        row("+  -", "Faster or slower."),
        row("q", "Quit."),
        text(""),
        theme::faint("Press a key to close this help."),
    };
    if (roomy) {
        rows.insert(rows.begin() + 11, text(""));
        rows.insert(rows.begin() + 1, text(""));
    } else {
        // A small terminal: the key row below shows the playback keys.
        rows.erase(rows.begin() + 9, rows.begin() + 17);
    }
    return theme::pane(text("Help") | bold, vbox(std::move(rows)) | center) | size(WIDTH, EQUAL, width) |
           size(HEIGHT, EQUAL, height);
}

}  // namespace

Element main_screen(const Controller& c, int width, int height) {
    if (width < kMinWidth || height < kMinHeight) {
        return small_screen(width, height);
    }
    const int pane_height = height - kHeaderRows - kBottomRows;
    const int left = width / 2;
    const int right = width - left;
    Element panes = c.help_open() ? help_pane(width, pane_height)
                                  : hbox({tree_pane(c, SetName::S1, left, pane_height),
                                          tree_pane(c, SetName::S2, right, pane_height)});
    Element screen = vbox({
        header(c, width),
        std::move(panes),
        narration_row(c, width),
        progress_row(c, width),
        text(""),
        prompt_row(c, width),
        keys_row(),
    });
    return screen | size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

Element small_screen(int width, int height) {
    Element card = vbox({
        text("Please enlarge the terminal.") | bold | hcenter,
        text(""),
        theme::faint("Sets needs " + std::to_string(kMinWidth) + " × " + std::to_string(kMinHeight) +
                     ". This terminal is " + std::to_string(width) + " × " + std::to_string(height) + ".") |
            hcenter,
        theme::faint("q quit") | hcenter,
    });
    return card | center | size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

}  // namespace strset::tui
