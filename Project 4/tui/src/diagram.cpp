#include "diagram.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "sal_tui/format.hpp"
#include "sal_tui/route.hpp"
#include "theme.hpp"

namespace sal::tui {

namespace {

using namespace ftxui;

// ---- A small character grid --------------------------------------------------

enum class Ink : std::uint8_t { Plain, Muted, Source, Target, Path, Label };

struct GridCell {
    std::string glyph = " ";
    Ink ink = Ink::Plain;
    bool bold = false;
};

class Grid {
public:
    Grid(int width, int height)
        : width_(width), cells_(static_cast<std::size_t>(width * height)) {}

    void put(int x, int y, std::string glyph, Ink ink, bool bold = false) {
        if (x < 0 || x >= width_ || y < 0 || y >= height()) {
            return;
        }
        cells_[index(x, y)] = GridCell{std::move(glyph), ink, bold};
    }

    /// Writes UTF-8 text, one code point for each cell.
    void write(int x, int y, const std::string& value, Ink ink, bool bold = false) {
        std::size_t i = 0;
        while (i < value.size()) {
            std::size_t length = 1;
            while (i + length < value.size() &&
                   (static_cast<unsigned char>(value[i + length]) & 0xC0U) == 0x80U) {
                ++length;
            }
            put(x++, y, value.substr(i, length), ink, bold);
            i += length;
        }
    }

    [[nodiscard]] const GridCell& at(int x, int y) const { return cells_[index(x, y)]; }
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return static_cast<int>(cells_.size()) / width_; }

    /// One FTXUI element: a vbox of rows; each row joins the cells with the same style.
    [[nodiscard]] Element render() const {
        Elements rows;
        for (int y = 0; y < height(); ++y) {
            Elements spans;
            int x = 0;
            while (x < width_) {
                const GridCell& first = at(x, y);
                std::string run;
                int end = x;
                while (end < width_ && at(end, y).ink == first.ink && at(end, y).bold == first.bold) {
                    run += at(end, y).glyph;
                    ++end;
                }
                spans.push_back(style(text(run), first.ink, first.bold));
                x = end;
            }
            rows.push_back(hbox(std::move(spans)));
        }
        return vbox(std::move(rows));
    }

private:
    [[nodiscard]] std::size_t index(int x, int y) const {
        return static_cast<std::size_t>(y * width_ + x);
    }

    static Element style(Element element, Ink ink, bool bold) {
        switch (ink) {
            case Ink::Plain: break;
            case Ink::Muted: element = element | color(theme::muted()); break;
            case Ink::Source: element = element | color(theme::source()); break;
            case Ink::Target:
            case Ink::Path: element = element | color(theme::target()); break;
            case Ink::Label: element = element | color(Color::Default); break;
        }
        return bold ? element | ftxui::bold : element;
    }

    int width_;
    std::vector<GridCell> cells_;
};

// ---- Layout -------------------------------------------------------------------

constexpr int kTopRow = 0;     // Rows 0-2: PC, Memory, IR, Decoder.
constexpr int kBusRow = 5;     // Rows 3-4: stubs; row 5: the bus; rows 6-7: stubs.
constexpr int kBottomRow = 8;  // Rows 8-10: A, B, ALU, Z, V.

struct BoxLayout {
    Unit unit;
    int x = 0;
    int width = 0;
    bool top = true;
    int stub_x = -1;  ///< The column of the stub to the bus. -1 means the center.
    [[nodiscard]] int stub() const noexcept { return stub_x >= 0 ? stub_x : x + width / 2; }
};

/// Moves the stubs of the lower boxes away from the stubs of the upper
/// boxes, so that two stubs never meet the bus at near columns.
void separate_stubs(std::vector<BoxLayout>& boxes) {
    std::vector<int> upper;
    for (const auto& box : boxes) {
        if (box.top) upper.push_back(box.stub());
    }
    for (auto& box : boxes) {
        if (box.top) continue;
        const int center = box.x + box.width / 2;
        int best = center;
        int best_cost = 1 << 20;
        for (int column = box.x + 2; column <= box.x + box.width - 3; ++column) {
            const bool near = std::any_of(upper.begin(), upper.end(), [column](int u) { return std::abs(u - column) <= 2; });
            const int cost = (near ? 1000 : 0) + std::abs(column - center);
            if (cost < best_cost) {
                best_cost = cost;
                best = column;
            }
        }
        box.stub_x = best;
    }
}

/// Puts boxes on one row: fixed widths, equal gaps, centered.
void justify(std::vector<BoxLayout>& boxes, int width) {
    int used = 0;
    for (const auto& box : boxes) used += box.width;
    const int gaps = static_cast<int>(boxes.size()) - 1;
    const int gap = std::max(1, (width - used) / std::max(1, gaps));
    int x = std::max(0, (width - used - gap * gaps) / 2);
    for (auto& box : boxes) {
        box.x = x;
        x += box.width + gap;
    }
}

std::vector<BoxLayout> layout(int width) {
    const int pc = 8;
    const int wide = std::clamp((width - pc - 9) / 3, 9, 22);
    std::vector<BoxLayout> top{{Unit::PC, 0, pc, true},
                               {Unit::Memory, 0, wide, true},
                               {Unit::IR, 0, wide, true},
                               {Unit::Decoder, 0, wide, true}};
    justify(top, width);

    const int reg = std::clamp((width - 9 - 10 - 16) / 2, 13, 16);
    const int alu = width >= 56 ? 9 : 7;
    std::vector<BoxLayout> bottom{{Unit::A, 0, reg, false},
                                  {Unit::B, 0, reg, false},
                                  {Unit::ALU, 0, alu, false},
                                  {Unit::Z, 0, 5, false},
                                  {Unit::V, 0, 5, false}};
    justify(bottom, width);

    top.insert(top.end(), bottom.begin(), bottom.end());
    separate_stubs(top);
    return top;
}

// ---- Contents -----------------------------------------------------------------

struct BoxText {
    std::string value;
    bool right_align = false;
    bool changed = false;
};

std::string decoded_text(const sal::Decode& decode) {
    return decode.operand.empty() ? decode.opcode : decode.opcode + " " + decode.operand;
}

/// The last memory access, decode and fetch among the visible micro-operations.
struct Visible {
    std::optional<std::string> memory;
    std::optional<std::string> ir;
    std::optional<std::string> decoder;
};

Visible visible_contents(const Controller& c) {
    Visible result;
    const auto& ops = c.micro_ops();
    for (std::size_t i = 0; i < c.cursor(); ++i) {
        const sal::MicroOp& op = ops[i];
        if (const auto* fetch = std::get_if<sal::Fetch>(&op)) {
            result.memory = "[" + std::to_string(fetch->address) + "]";
            result.ir = fetch->text;
        } else if (const auto* decode = std::get_if<sal::Decode>(&op)) {
            result.decoder = decoded_text(*decode);
        } else if (const auto* read = std::get_if<sal::MemRead>(&op)) {
            result.memory = "[" + std::to_string(read->address) + "] " + read->symbol.value_or("");
        } else if (const auto* write = std::get_if<sal::MemWrite>(&op)) {
            result.memory = "[" + std::to_string(write->address) + "] " + write->symbol.value_or("");
        } else if (const auto* symbol = std::get_if<sal::SymbolInfo>(&op)) {
            result.memory = "[" + std::to_string(symbol->address) + "] " + symbol->name;
        }
    }
    return result;
}

BoxText content(Unit unit, const Controller& c, const Visible& visible) {
    const sal::MachineState& now = c.state();
    const sal::MachineState& before = c.previous_state();
    const auto bit = [](bool value) { return std::string{value ? "1" : "0"}; };
    switch (unit) {
        case Unit::PC: return {format_address(now.pc, c.base()), true, now.pc != before.pc};
        case Unit::Memory: return {visible.memory.value_or(""), false, false};
        case Unit::IR: return {visible.ir.value_or(""), false, false};
        case Unit::Decoder: return {visible.decoder.value_or(""), false, false};
        case Unit::A: return {format_word(now.a, c.base()), true, now.a != before.a};
        case Unit::B: return {format_word(now.b, c.base()), true, now.b != before.b};
        case Unit::ALU: return {"+", false, false};
        case Unit::Z: return {bit(now.z), false, now.z != before.z};
        case Unit::V: return {bit(now.v), false, now.v != before.v};
    }
    return {};
}

// ---- Drawing --------------------------------------------------------------------

void draw_box(Grid& grid, const BoxLayout& box, const BoxText& body, const Route& route) {
    const bool is_target = route.is_target(box.unit);
    const bool is_source = route.is_source(box.unit);
    const Ink frame = is_target ? Ink::Target : is_source ? Ink::Source : Ink::Muted;
    const bool bold = is_target || is_source;
    const int y = box.top ? kTopRow : kBottomRow;
    const int right = box.x + box.width - 1;

    // The frame.
    for (int x = box.x + 1; x < right; ++x) {
        grid.put(x, y, "─", frame, bold);
        grid.put(x, y + 2, "─", frame, bold);
    }
    grid.put(box.x, y, "┌", frame, bold);
    grid.put(right, y, "┐", frame, bold);
    grid.put(box.x, y + 2, "└", frame, bold);
    grid.put(right, y + 2, "┘", frame, bold);
    grid.put(box.x, y + 1, "│", frame, bold);
    grid.put(right, y + 1, "│", frame, bold);
    // The junction with the stub to the bus.
    grid.put(box.stub(), box.top ? y + 2 : y, box.top ? "┬" : "┴", frame, bold);

    // The title: in the top edge for the upper row, in the bottom edge for the lower row.
    const std::string title = to_string(box.unit);
    const int title_y = box.top ? y : y + 2;
    const int inner = box.width - 2;
    if (static_cast<int>(title.size()) + 2 <= inner) {
        grid.write(box.x + 1, title_y, " " + title + " ", frame, bold);
    } else {
        const int x = box.x + 1 + (inner - static_cast<int>(title.size())) / 2;
        grid.write(x, title_y, title, frame, bold);
    }

    // The value, with one space of padding when it fits.
    // Use one space of padding on each side, unless the value needs the room.
    const int room = inner >= 4 && static_cast<int>(display_width(body.value)) <= inner - 2 ? inner - 2 : inner;
    const int left = box.x + 1 + (inner - room) / 2;
    std::string value = truncate(body.value, static_cast<std::size_t>(room));
    if (body.right_align) {
        value = pad_left(value, static_cast<std::size_t>(room));
    } else if (box.width <= 9) {
        // Center short values (ALU, Z, V).
        const int pad = (room - static_cast<int>(display_width(value))) / 2;
        value = std::string(static_cast<std::size_t>(std::max(0, pad)), ' ') + value;
    }
    grid.write(left, y + 1, pad_right(value, static_cast<std::size_t>(room)),
               body.changed ? Ink::Target : Ink::Plain, body.changed || is_target);
}

/// The box-drawing character for a point with lines in the given directions.
const char* junction(bool up, bool down, bool left, bool right) {
    const int key = (up ? 8 : 0) | (down ? 4 : 0) | (left ? 2 : 0) | (right ? 1 : 0);
    static constexpr std::array<const char*, 16> glyphs{
        " ", "─", "─", "─", "│", "┌", "┐", "┬", "│", "└", "┘", "┴", "│", "├", "┤", "┼"};
    return glyphs[static_cast<std::size_t>(key)];
}

void draw_bus(Grid& grid, const std::vector<BoxLayout>& boxes, const Route& route) {
    int bus_left = grid.width();
    int bus_right = -1;
    int path_left = grid.width();
    int path_right = -1;
    for (const auto& box : boxes) {
        bus_left = std::min(bus_left, box.stub());
        bus_right = std::max(bus_right, box.stub());
        if (route.involves(box.unit) && !route.sources.empty()) {
            path_left = std::min(path_left, box.stub());
            path_right = std::max(path_right, box.stub());
        }
    }
    const auto on_path = [&](int x) { return x >= path_left && x <= path_right; };

    // The bus line and its junctions.
    for (int x = bus_left; x <= bus_right; ++x) {
        bool up = false;
        bool down = false;
        for (const auto& box : boxes) {
            if (box.stub() == x) (box.top ? up : down) = true;
        }
        const bool path = on_path(x);
        grid.put(x, kBusRow, junction(up, down, x > bus_left, x < bus_right), path ? Ink::Path : Ink::Muted,
                 path);
    }

    // The stubs between the boxes and the bus.
    for (const auto& box : boxes) {
        const bool active = route.involves(box.unit) && !route.sources.empty();
        const Ink ink = active ? Ink::Path : Ink::Muted;
        const int near_box = box.top ? kTopRow + 3 : kBottomRow - 1;
        const int near_bus = box.top ? kTopRow + 4 : kBottomRow - 2;
        grid.put(box.stub(), near_bus, "│", ink, active);
        const bool arrow = active && route.is_target(box.unit);
        grid.put(box.stub(), near_box, arrow ? (box.top ? "▲" : "▼") : "│", ink, active);
    }

    // The label: the moving value and its direction, on the bus.
    if (route.label.empty() || route.sources.empty() || path_right < 0) {
        return;
    }
    double source_x = 0;
    for (const Unit unit : route.sources) {
        const auto it = std::find_if(boxes.begin(), boxes.end(), [unit](const BoxLayout& b) { return b.unit == unit; });
        source_x += it->stub();
    }
    source_x /= static_cast<double>(route.sources.size());
    const auto target = std::find_if(boxes.begin(), boxes.end(),
                                     [&](const BoxLayout& b) { return b.unit == route.targets.front(); });
    const bool rightward = target->stub() >= source_x;
    const std::string value = truncate(route.label, 16);
    const std::string arrow = rightward ? "▶" : "◀";

    // Try a spaced label, a compact label and shorter compact labels, on the
    // bus row and on the stub rows next to it. A label must not cover a stub:
    // a covered junction makes a unit look connected to the value, or cut
    // off from the bus. The cost of a cover is so high that a shorter label
    // always wins. The cost also grows with the distance from the bus, from
    // the middle of the path, and with each character that the label loses.
    // The status line always shows the full value.
    struct Choice {
        std::string label;
        int row = kBusRow;
        int start = 0;
        int cost = -1;
    };
    struct Candidate {
        std::string label;
        int penalty = 0;
    };
    constexpr int kCoverCost = 10000;
    Choice best;
    const int middle = (path_left + path_right) / 2;
    int upper_units = 0;
    int lower_units = 0;
    for (const auto& box : boxes) {
        if (route.involves(box.unit)) (box.top ? upper_units : lower_units) += 1;
    }
    const bool route_on_top = upper_units >= lower_units;
    const auto with_arrow = [&](const std::string& text) { return rightward ? text + "▶" : "◀" + text; };
    const std::string spaced = rightward ? " " + value + " ▶ " : " ◀ " + value + " ";
    std::vector<Candidate> candidates{{spaced, 0}, {with_arrow(value), 4}};
    const int value_width = static_cast<int>(display_width(value));
    for (int width = value_width - 1; width >= 2; --width) {
        // Each lost character costs more than any position of a full label.
        candidates.push_back({with_arrow(truncate(value, static_cast<std::size_t>(width))),
                              200 + 20 * (value_width - width)});
    }
    for (const auto& [label, penalty] : candidates) {
        const int length = static_cast<int>(display_width(label));
        for (const int row : {kBusRow, kBusRow - 1, kBusRow + 1}) {
            for (int start = 0; start + length <= grid.width(); ++start) {
                int cost = std::abs(start + length / 2 - middle) + penalty;
                if (row != kBusRow) {
                    // Prefer the side of the bus where most units of the route are.
                    const bool upper_side = row < kBusRow;
                    cost += 12 + (upper_side == route_on_top ? 0 : 8);
                }
                for (const auto& box : boxes) {
                    const bool on_row = row == kBusRow || (row < kBusRow) == box.top;
                    if (!on_row) continue;
                    const int s = box.stub();
                    if (s >= start && s < start + length) {
                        cost += kCoverCost;
                    } else if (row != kBusRow && (s == start - 1 || s == start + length)) {
                        // On a stub row, a free column next to each stub reads better.
                        cost += 30;
                    }
                }
                if (row == kBusRow) {
                    cost += 40 * (std::max(0, path_left + 1 - start) + std::max(0, start + length - path_right));
                } else {
                    cost += 40 * (std::max(0, path_left - start) + std::max(0, start + length - 1 - path_right));
                }
                if (best.cost < 0 || cost < best.cost) {
                    best = Choice{label, row, start, cost};
                }
            }
        }
    }
    grid.write(best.start, best.row, best.label, Ink::Label, true);
    // Color the arrow like the path.
    const int length = static_cast<int>(display_width(best.label));
    const bool spaced_label = best.label == spaced;
    const int arrow_x = rightward ? best.start + length - (spaced_label ? 2 : 1) : best.start + (spaced_label ? 1 : 0);
    grid.put(arrow_x, best.row, arrow, Ink::Path, true);
}

}  // namespace

Element datapath_diagram(const Controller& controller, int width) {
    width = std::max(width, kDiagramMinWidth);
    Grid grid{width, kDiagramHeight};
    const std::vector<BoxLayout> boxes = layout(width);

    Route route;
    if (controller.cursor() > 0) {
        route = route_of(controller.micro_ops(), controller.cursor() - 1, controller.base());
    }
    draw_bus(grid, boxes, route);
    const Visible visible = visible_contents(controller);
    for (const auto& box : boxes) {
        draw_box(grid, box, content(box.unit, controller, visible), route);
    }
    return grid.render();
}

}  // namespace sal::tui
