#include "views.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include "diagram.hpp"
#include "sal_tui/format.hpp"
#include "sal_tui/route.hpp"
#include "theme.hpp"

namespace sal::tui {

namespace {

using namespace ftxui;

constexpr int kHelpRows = 4;  // Registers, narration, message, keys.
constexpr int kHeaderRows = 1;

Element spacer(int columns) { return text(std::string(static_cast<std::size_t>(columns), ' ')); }

// ---- Header ---------------------------------------------------------------------

Element status_view(const Controller& c) {
    if (const auto reason = c.halt_reason()) {
        const Color tone = *reason == sal::HaltReason::HltExecuted ? theme::ok()
                           : *reason == sal::HaltReason::RanPastEnd ? theme::target()
                                                                    : theme::alert();
        return text("■ Halted: " + std::string{sal::to_string(*reason)}) | color(tone) | bold;
    }
    if (c.running()) {
        return text("● Running") | color(theme::target()) | bold;
    }
    return text(c.has_instruction() ? "Paused" : "Ready") | bold;
}

Element speed_view(const Controller& c) {
    return theme::faint("speed " + speed_text(c.tick_interval()) + " micro-ops/s");
}

Element header(const Controller& c, const std::string& file_name) {
    return hbox({text(" ALI ") | bold, theme::faint("· " + file_name), filler(), speed_view(c), spacer(4),
                 status_view(c), spacer(1)});
}

// ---- Source pane -------------------------------------------------------------------

enum class Marker : std::uint8_t { None, Next, Executing, Halted };

struct MarkerAt {
    Marker kind = Marker::None;
    std::size_t address = 0;  ///< program().size() means the "end" row.
};

MarkerAt marker_of(const Controller& c) {
    if (c.has_instruction() && !c.instruction_complete()) {
        return {Marker::Executing, *c.instruction_address()};
    }
    if (const auto reason = c.halt_reason()) {
        if (*reason == sal::HaltReason::RanPastEnd) {
            return {Marker::Halted, c.program().size()};
        }
        return {Marker::Halted, c.state().pc};
    }
    return {Marker::Next, c.state().pc};
}

Element source_view(const Controller& c, const ViewContext& context, int width) {
    const sal::Program& program = c.program();
    const MarkerAt marker = marker_of(c);
    const int text_width = std::max(4, width - 2 - 9);
    if (context.source_rows != nullptr) {
        context.source_rows->assign(program.size(), Box{});
    }

    Elements rows;
    rows.push_back(text(""));
    for (std::size_t address = 0; address <= program.size(); ++address) {
        const bool end_row = address == program.size();
        const bool marked = marker.kind != Marker::None && marker.address == address;
        std::string arrow = " ";
        Color arrow_color = Color::Default;
        if (marked) {
            switch (marker.kind) {
                case Marker::Executing: arrow = "▶"; arrow_color = theme::target(); break;
                case Marker::Next: arrow = "▶"; break;
                case Marker::Halted:
                    arrow = "■";
                    arrow_color = c.halt_reason() == sal::HaltReason::HltExecuted ? theme::ok()
                                  : c.halt_reason() == sal::HaltReason::RanPastEnd ? theme::target()
                                                                                   : theme::alert();
                    break;
                case Marker::None: break;
            }
        }
        const auto byte = static_cast<std::uint8_t>(address);
        const bool breakpoint = !end_row && c.has_breakpoint(byte);
        const std::string number = pad_left(std::to_string(address), 3);
        const std::string body =
            end_row ? "end" : truncate(program.at(address).text, static_cast<std::size_t>(text_width));

        Element line_text = text(body);
        Element number_text = text(number);
        if (end_row) {
            line_text = line_text | color(theme::muted());
            number_text = number_text | color(theme::muted());
        } else if (marked && marker.kind == Marker::Executing) {
            line_text = line_text | color(theme::target()) | bold;
            number_text = number_text | color(theme::target());
        } else if (marked) {
            line_text = line_text | bold;
        } else {
            number_text = number_text | color(theme::muted());
        }

        Element row = hbox({
            spacer(1),
            text(arrow) | color(arrow_color) | bold,
            text(breakpoint ? "●" : " ") | color(theme::alert()),
            number_text,
            spacer(2),
            line_text,
        });
        row = row | xflex;
        if (!end_row && c.selection() == address) {
            row = row | bgcolor(Color::Grey23);
        }
        if (marked) {
            row = row | focus;
        }
        if (!end_row && context.source_rows != nullptr) {
            row = row | reflect((*context.source_rows)[address]);
        }
        rows.push_back(std::move(row));
    }
    return vbox(std::move(rows)) | yframe | yflex;
}

// ---- Datapath pane ---------------------------------------------------------------

Element progress_dots(const Controller& c) {
    Elements dots;
    const std::size_t total = c.micro_ops().size();
    for (std::size_t i = 0; i < total; ++i) {
        if (i > 0) dots.push_back(text(" "));
        if (i + 1 == c.cursor()) {
            dots.push_back(text("●") | color(theme::target()) | bold);
        } else if (i < c.cursor()) {
            dots.push_back(text("●") | color(theme::muted()));
        } else {
            dots.push_back(text("○") | color(theme::muted()));
        }
    }
    return hbox(std::move(dots));
}

Element instruction_card(const Controller& c) {
    if (!c.has_instruction()) {
        return vbox({text("No instruction has executed yet.") | bold,
                     theme::faint("Press space to show the first micro-operation.")});
    }
    const sal::CodeLine& line = c.program().at(*c.instruction_address());
    return vbox({
        hbox({text(line.text) | bold, filler(), progress_dots(c)}),
        text(line.instruction->describe()),
        theme::faint("address " + std::to_string(*c.instruction_address()) + ", source line " +
                     std::to_string(line.line)),
    });
}

Element datapath_view(const Controller& c, int width) {
    const int inner = width - 2;
    const int diagram_width = std::max(kDiagramMinWidth, inner - 3);
    return vbox({
        filler(),
        hbox({spacer(2), datapath_diagram(c, diagram_width)}),
        text(""),
        text(""),
        hbox({spacer(2), instruction_card(c) | xflex, spacer(2)}),
        filler(),
    });
}

// ---- Memory pane -------------------------------------------------------------------

Element memory_view(const Controller& c, int width) {
    const sal::Program& program = c.program();
    const int inner = width - 2 - 2;  // Frame and padding.
    const sal::MicroOp* op = c.active_micro_op();
    const auto* read = op != nullptr ? std::get_if<sal::MemRead>(op) : nullptr;
    const auto* write = op != nullptr ? std::get_if<sal::MemWrite>(op) : nullptr;
    const auto* declared = op != nullptr ? std::get_if<sal::SymbolInfo>(op) : nullptr;

    Elements rows;
    rows.push_back(text(""));
    rows.push_back(hbox({spacer(1), theme::faint("DATA")}));
    // A row is: space, marker, address (3), space, name, gap, value, space.
    // If the name and the widest possible value do not fit on one row, the
    // value goes on a second row. The decision depends only on the name, so
    // the layout does not jump when a value changes.
    const int row_room = inner + 2 - 7;  // The columns for the name, the gap and the value.
    const int widest_value = static_cast<int>(std::max(format_word(std::numeric_limits<std::int32_t>::min(), c.base()).size(),
                                                       format_word(std::numeric_limits<std::int32_t>::max(), c.base()).size()));
    for (const sal::Symbol& symbol : program.symbols()) {
        const std::int32_t value = std::get<sal::DataCell>(c.state().memory[symbol.address]).value;
        const std::int32_t before = std::get<sal::DataCell>(c.previous_state().memory[symbol.address]).value;
        const bool is_read = read != nullptr && read->address == symbol.address;
        const bool is_written = write != nullptr && write->address == symbol.address;
        // DEC changes no data. It only tells the address of the symbol.
        const bool is_declared = declared != nullptr && declared->address == symbol.address;
        const bool changed = value != before;

        const std::string number = std::to_string(symbol.address);
        const std::string shown = format_word(value, c.base());
        const std::size_t number_width = 3;
        const int name_width = static_cast<int>(display_width(symbol.name));
        const bool two_rows = name_width + 1 + widest_value > row_room;
        const int name_room = two_rows ? row_room : row_room - 1 - static_cast<int>(shown.size());
        Element marker = text(" ");
        Element name = text(truncate(symbol.name, static_cast<std::size_t>(std::max(3, name_room))));
        Element number_text = text(pad_left(number, number_width)) | color(theme::muted());
        if (is_read) {
            marker = text("▸") | color(theme::source()) | bold;
            name = name | color(theme::source()) | bold;
        } else if (is_written) {
            marker = text("◂") | color(theme::target()) | bold;
            name = name | color(theme::target()) | bold;
        } else if (is_declared) {
            marker = text("•") | color(theme::source()) | bold;
            name = name | color(theme::source()) | bold;
            number_text = text(pad_left(number, number_width)) | color(theme::source()) | bold;
        }
        Element value_text = text(shown);
        if (changed || is_written) {
            value_text = value_text | color(theme::target()) | bold;
        } else if (is_read) {
            value_text = value_text | color(theme::source()) | bold;
        }
        const bool active = is_read || is_written || is_declared;
        if (two_rows) {
            Element head = hbox({spacer(1), marker, number_text, spacer(1), name, filler()});
            Element tail = hbox({filler(), value_text, spacer(1)});
            Element both = vbox({std::move(head), std::move(tail)});
            if (active) both = both | focus;
            rows.push_back(std::move(both));
        } else {
            Element row = hbox({spacer(1), marker, number_text, spacer(1), name, filler(), value_text, spacer(1)});
            if (active) row = row | focus;
            rows.push_back(std::move(row));
        }
    }
    if (program.symbols().empty()) {
        rows.push_back(hbox({spacer(2), theme::faint("no symbols")}));
    }
    const std::size_t first_free = sal::kDataBase + program.symbols().size();
    if (first_free < sal::kMemorySize) {
        rows.push_back(hbox({spacer(1), theme::faint(" " + std::to_string(first_free) + "–255  free")}));
    }

    rows.push_back(text(""));
    rows.push_back(hbox({spacer(1), theme::faint("CODE")}));
    const std::size_t last = program.size() - 1;
    const bool fetching = op != nullptr && std::holds_alternative<sal::Fetch>(*op);
    Element code = text("   0–" + std::to_string(last) + "  program");
    if (fetching) {
        const auto& fetch = std::get<sal::Fetch>(*op);
        rows.push_back(hbox({spacer(1), code}));
        rows.push_back(hbox({spacer(1), text("▸") | color(theme::source()) | bold,
                             text(pad_left(std::to_string(fetch.address), 3)) | color(theme::source()),
                             spacer(1),
                             text(truncate(fetch.text, static_cast<std::size_t>(std::max(4, inner - 6)))) |
                                 color(theme::source()) | bold}));
    } else {
        rows.push_back(hbox({spacer(1), code}));
    }
    if (program.size() < sal::kCodeSize) {
        rows.push_back(hbox({spacer(1), theme::faint(pad_left(std::to_string(program.size()), 4) + "–127  empty")}));
    }
    return vbox(std::move(rows)) | yframe | yflex;
}

// ---- Bottom lines -------------------------------------------------------------------

Element registers_strip(const Controller& c) {
    const sal::MachineState& now = c.state();
    const sal::MachineState& before = c.previous_state();
    const auto item = [](const std::string& name, const std::string& value, bool changed) {
        Element shown = text(value);
        if (changed) shown = shown | color(theme::target()) | bold;
        return hbox({text(name + " ") | color(theme::muted()), shown, spacer(5)});
    };
    return hbox({
        spacer(2),
        item("A", format_word(now.a, c.base()), now.a != before.a),
        item("B", format_word(now.b, c.base()), now.b != before.b),
        item("PC", format_address(now.pc, c.base()), now.pc != before.pc),
        item("Z", now.z ? "1" : "0", now.z != before.z),
        item("V", now.v ? "1" : "0", now.v != before.v),
        filler(),
        theme::faint(c.base() == NumberBase::Hex ? "hex" : "decimal"),
        spacer(1),
    });
}

Element narration(const Controller& c) {
    Element sentence;
    if (const sal::MicroOp* op = c.active_micro_op()) {
        sentence = hbox({text("▸ ") | color(theme::target()) | bold, text(sal::describe(*op)) | bold});
    } else {
        sentence = theme::faint("Press space to execute the program one micro-operation at a time.");
    }
    Element position = text("");
    if (c.has_instruction()) {
        position = theme::faint("step " + std::to_string(c.step_number()) + "  ·  micro-op " +
                                std::to_string(c.cursor()) + " of " + std::to_string(c.micro_ops().size()));
    }
    return hbox({spacer(2), sentence, filler(), position, spacer(1)});
}

Element message_line(const Controller& c) {
    if (c.message().empty()) {
        return text("");
    }
    return hbox({spacer(2), text(c.message()) | color(theme::target())});
}

Element key_help(const Controller& c) {
    const auto key = [](const std::string& name, const std::string& action) {
        return hbox({text(name) | bold, text(" " + action) | color(theme::muted()), spacer(2)});
    };
    return hbox({
        spacer(2),
        hbox({text("space") | color(theme::target()) | bold, text(" next") | color(theme::muted()), spacer(2)}),
        key("s", "step"),
        key("b", "back"),
        key("r", c.running() ? "pause" : "run"),
        key("g", "to end"),
        key("+/-", "speed"),
        key("R", "reset"),
        key("x", "hex"),
        key("⏎", "breakpoint"),
        key("q", "quit"),
    });
}

}  // namespace

Element main_screen(const Controller& c, const ViewContext& context, int width, int height) {
    if (width < kMinWidth || height < kMinHeight) {
        return small_screen(width, height);
    }
    const int extra = width - kMinWidth;
    const int source_width = std::min(32, 22 + extra * 3 / 10);
    const int memory_width = std::min(34, 24 + extra * 3 / 10);
    const int datapath_width = width - source_width - memory_width;
    const int pane_height = height - kHeaderRows - kHelpRows;

    Element panes = hbox({
        theme::pane("Program", source_view(c, context, source_width)) | size(WIDTH, EQUAL, source_width),
        theme::pane("Datapath", datapath_view(c, datapath_width)) | size(WIDTH, EQUAL, datapath_width),
        theme::pane("Memory", memory_view(c, memory_width)) | size(WIDTH, EQUAL, memory_width),
    });
    return vbox({
        header(c, context.file_name),
        panes | size(HEIGHT, EQUAL, pane_height),
        registers_strip(c),
        narration(c),
        message_line(c),
        key_help(c),
    });
}

Element error_screen(const std::string& file_name, const std::vector<sal::ParseError>& errors,
                     const std::vector<std::string>& source_lines, int width, int height) {
    const int card_width = std::clamp(width - 8, 40, 96);
    Elements rows;
    const std::string count = std::to_string(errors.size()) + (errors.size() == 1 ? " error" : " errors");
    rows.push_back(text("The program has " + count + ". ALI cannot run it.") | bold);
    rows.push_back(text(""));
    // One group of rows for each error: the message, the source line, the
    // column marker and a blank row.
    std::vector<Elements> groups;
    for (const sal::ParseError& e : errors) {
        Elements group;
        std::string where = e.line > 0 ? "line " + std::to_string(e.line) : "file";
        if (e.column > 0) where += ", column " + std::to_string(e.column);
        group.push_back(hbox({text(pad_right(where, 20)) | color(theme::alert()), text(e.message)}));
        if (e.line > 0 && static_cast<std::size_t>(e.line) <= source_lines.size()) {
            const std::string number = pad_left(std::to_string(e.line), 5);
            group.push_back(hbox({theme::faint(number + " │ "),
                                  text(truncate(source_lines[static_cast<std::size_t>(e.line - 1)],
                                                static_cast<std::size_t>(card_width - 12)))}));
            if (e.column > 0) {
                group.push_back(hbox({theme::faint(std::string(5, ' ') + " │ "),
                                      text(std::string(static_cast<std::size_t>(e.column - 1), ' ') + "^") |
                                          color(theme::alert()) | bold}));
            }
        }
        group.push_back(text(""));
        groups.push_back(std::move(group));
    }
    // Show the errors that fit. If some do not fit, the last row tells how
    // many are missing. The terminal gets all of them at exit.
    const std::size_t budget = static_cast<std::size_t>(std::max(5, height - 8));
    std::size_t total = rows.size();
    for (const auto& group : groups) total += group.size();
    const std::size_t room = total <= budget ? budget : budget - 1;
    std::size_t shown = 0;
    for (auto& group : groups) {
        if (rows.size() + group.size() > room) break;
        for (auto& row : group) rows.push_back(std::move(row));
        ++shown;
    }
    if (shown < groups.size()) {
        const std::size_t missing = groups.size() - shown;
        rows.push_back(theme::faint("... and " + std::to_string(missing) +
                                    (missing == 1 ? " more error" : " more errors") +
                                    ". ALI prints all of them when you quit."));
    }
    Element body = vbox(std::move(rows)) | size(HEIGHT, LESS_THAN, static_cast<int>(budget) + 1);
    Element card = vbox({
        hbox({text("ALI ") | bold, theme::faint("· " + file_name)}),
        text(""),
        body,
        hbox({theme::faint("Correct the file and start ALI again.   "), text("q") | bold, theme::faint(" quit")}),
    });
    return card | size(WIDTH, EQUAL, card_width) | center | size(WIDTH, EQUAL, width) |
           size(HEIGHT, EQUAL, height);
}

Element small_screen(int width, int height) {
    Element card = vbox({
        text("Please enlarge the terminal.") | bold | hcenter,
        text(""),
        theme::faint("ALI needs " + std::to_string(kMinWidth) + " × " + std::to_string(kMinHeight) +
                     ". This terminal is " + std::to_string(width) + " × " + std::to_string(height) + ".") |
            hcenter,
        theme::faint("q quit") | hcenter,
    });
    return card | center | size(WIDTH, EQUAL, width) | size(HEIGHT, EQUAL, height);
}

}  // namespace sal::tui
