#include "strset_tui/tree_layout.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "strset_tui/format.h"

namespace strset::tui {

// ---- Canvas -------------------------------------------------------------------------

Canvas::Canvas(int width, int height)
    : width_(std::max(width, 0)),
      height_(std::max(height, 0)),
      cells_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_)) {}

const Cell& Canvas::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        throw std::out_of_range("Canvas::at: the position is not on the canvas");
    }
    return cells_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

void Canvas::put(int x, int y, const std::string& glyph, Tone tone, bool bold) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    Cell& cell = cells_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
    cell.glyph = glyph;
    cell.tone = tone;
    cell.bold = bold;
}

int Canvas::write(int x, int y, const std::string& text, Tone tone, bool bold) {
    int count = 0;
    for (const std::string& glyph : glyphs(text)) {
        put(x + count, y, glyph, tone, bold);
        ++count;
    }
    return count;
}

std::string Canvas::row_text(int y) const {
    std::string text;
    for (int x = 0; x < width_; ++x) {
        text += at(x, y).glyph;
    }
    while (!text.empty() && text.back() == ' ') {
        text.pop_back();
    }
    return text;
}

// ---- Layout helpers ---------------------------------------------------------------

namespace {

/// One node in order, with its depth and its parent.
struct Placed {
    int node = 0;     ///< The index in Shape::nodes.
    int depth = 0;
    int parent = -1;  ///< The index in Shape::nodes, or -1 for the root.
    bool left = false;  ///< The node is the left child of its parent.
};

/// The nodes in order (alphabetical order), without recursion.
std::vector<Placed> in_order(const Shape& shape) {
    std::vector<Placed> result;
    if (shape.nodes.empty()) {
        return result;
    }
    result.reserve(shape.nodes.size());
    std::vector<Placed> stack;
    Placed current{0, 0, -1, false};
    bool have_current = true;
    while (have_current || !stack.empty()) {
        while (have_current) {
            stack.push_back(current);
            const int left = shape.nodes[static_cast<std::size_t>(current.node)].left;
            have_current = left >= 0;
            if (have_current) current = Placed{left, current.depth + 1, current.node, true};
        }
        const Placed top = stack.back();
        stack.pop_back();
        result.push_back(top);
        const int right = shape.nodes[static_cast<std::size_t>(top.node)].right;
        have_current = right >= 0;
        if (have_current) current = Placed{right, top.depth + 1, top.node, false};
    }
    return result;
}

NodeMark mark_of(const TreeMarks& marks, NodeId id) {
    const auto it = marks.nodes.find(id);
    return it == marks.nodes.end() ? NodeMark{} : it->second;
}

bool lit(const TreeMarks& marks, NodeId id) { return marks.lit_links.count(id) != 0; }

struct TopDownPlan {
    std::vector<Placed> order;
    std::vector<std::string> labels;  ///< By position in `order`.
    std::vector<int> x;               ///< The first column of each label.
    std::vector<int> center;          ///< The column of the connector.
    std::vector<int> position;        ///< The position in `order` of each shape node.
    int width = 0;
    int rows = 0;
};

constexpr int kGap = 2;  ///< The minimum number of spaces between two labels on a row.

/// The parts of a plan that do not depend on the style: the order, the
/// labels and the rows. The columns are still 0.
TopDownPlan plan_base(const Shape& shape, bool show_ids) {
    TopDownPlan plan;
    plan.order = in_order(shape);
    plan.position.assign(shape.nodes.size(), 0);
    int max_depth = 0;
    for (std::size_t i = 0; i < plan.order.size(); ++i) {
        const Placed& p = plan.order[i];
        plan.labels.push_back(node_label(shape.nodes[static_cast<std::size_t>(p.node)], kTopDownLabelWidth, show_ids));
        plan.position[static_cast<std::size_t>(p.node)] = static_cast<int>(i);
        max_depth = std::max(max_depth, p.depth);
    }
    plan.x.assign(plan.order.size(), 0);
    plan.center.assign(plan.order.size(), 0);
    plan.rows = plan.order.empty() ? 0 : 2 * max_depth + 1;
    return plan;
}

int label_width(const TopDownPlan& plan, std::size_t i) { return static_cast<int>(display_width(plan.labels[i])); }

/// Sets the centers and the width from the columns. The columns start at 0.
void finish(TopDownPlan& plan) {
    int end = 0;
    for (std::size_t i = 0; i < plan.order.size(); ++i) {
        const int w = label_width(plan, i);
        plan.center[i] = plan.x[i] + (w - 1) / 2;
        end = std::max(end, plan.x[i] + w);
    }
    plan.width = end;
}

/// The Columns style: each node has its own columns, in alphabetical order.
/// A vertical line through the tree crosses at most one label.
TopDownPlan plan_columns(const Shape& shape, bool show_ids) {
    TopDownPlan plan = plan_base(shape, show_ids);
    int cursor = 0;
    for (std::size_t i = 0; i < plan.order.size(); ++i) {
        plan.x[i] = cursor;
        cursor += label_width(plan, i) + kGap;
    }
    finish(plan);
    return plan;
}

/// The Compact style: a "tidy tree". Each subtree is packed against its
/// sibling as close as its contour lets it, row by row. A parent is centered
/// above its two children, or two columns to the side of its only child.
///
/// The children of a node come after it in Shape::nodes (preorder), so one
/// pass from the end visits each subtree before its parent, without recursion.
/// If the tree has more than `max_rows` rows, it cannot fit, and the function
/// stops early (the width is INT_MAX).
TopDownPlan plan_compact(const Shape& shape, bool show_ids, int max_rows) {
    TopDownPlan plan = plan_base(shape, show_ids);
    if (plan.rows > max_rows) {
        plan.width = std::numeric_limits<int>::max();
        return plan;
    }
    const std::size_t n = shape.nodes.size();
    std::vector<int> width_of(n);
    for (std::size_t k = 0; k < n; ++k) {
        width_of[k] = label_width(plan, static_cast<std::size_t>(plan.position[k]));
    }
    const auto center_of = [&](int node) { return (width_of[static_cast<std::size_t>(node)] - 1) / 2; };

    // contour[k]: for each level of the subtree of node k, the first and the
    // last column of the labels, relative to the first column of node k.
    struct Contour {
        std::vector<int> low;
        std::vector<int> high;
    };
    std::vector<Contour> contour(n);
    std::vector<int> offset(n, 0);  // The column of a node relative to its parent.

    const auto merge = [](Contour& into, const Contour& from, int shift) {
        for (std::size_t level = 0; level < from.low.size(); ++level) {
            const int low = from.low[level] + shift;
            const int high = from.high[level] + shift;
            if (level + 1 < into.low.size()) {
                into.low[level + 1] = std::min(into.low[level + 1], low);
                into.high[level + 1] = std::max(into.high[level + 1], high);
            } else {
                into.low.push_back(low);
                into.high.push_back(high);
            }
        }
    };

    for (std::size_t k = n; k-- > 0;) {
        const ShapeNode& node = shape.nodes[k];
        const int own_center = center_of(static_cast<int>(k));
        Contour own{{0}, {width_of[k] - 1}};
        if (node.left >= 0 && node.right >= 0) {
            const auto left = static_cast<std::size_t>(node.left);
            const auto right = static_cast<std::size_t>(node.right);
            const Contour& a = contour[left];
            const Contour& b = contour[right];
            // `shift`: the column of the right child relative to the left child.
            int shift = std::numeric_limits<int>::min();
            for (std::size_t level = 0; level < std::min(a.low.size(), b.low.size()); ++level) {
                shift = std::max(shift, a.high[level] - b.low[level] + kGap + 1);
            }
            const int middle = (center_of(node.left) + shift + center_of(node.right)) / 2;
            const int parent = middle - own_center;  // In the columns of the left child.
            offset[left] = -parent;
            offset[right] = shift - parent;
            merge(own, a, offset[left]);
            merge(own, b, offset[right]);
        } else if (node.left >= 0 || node.right >= 0) {
            const int child = node.left >= 0 ? node.left : node.right;
            const int side = node.left >= 0 ? 2 : -2;  // The parent is to the right of a left child.
            const int parent = center_of(child) + side - own_center;
            offset[static_cast<std::size_t>(child)] = -parent;
            merge(own, contour[static_cast<std::size_t>(child)], -parent);
        }
        if (node.left >= 0) contour[static_cast<std::size_t>(node.left)] = Contour{};
        if (node.right >= 0) contour[static_cast<std::size_t>(node.right)] = Contour{};
        contour[k] = std::move(own);
    }

    // The absolute columns, parents first, then a shift so that the leftmost
    // label starts at column 0.
    std::vector<int> column(n, 0);
    int leftmost = 0;
    for (std::size_t k = 0; k < n; ++k) {
        const ShapeNode& node = shape.nodes[k];
        for (const int child : {node.left, node.right}) {
            if (child >= 0) {
                column[static_cast<std::size_t>(child)] = column[k] + offset[static_cast<std::size_t>(child)];
            }
        }
        leftmost = std::min(leftmost, column[k]);
    }
    for (std::size_t k = 0; k < n; ++k) {
        plan.x[static_cast<std::size_t>(plan.position[k])] = column[k] - leftmost;
    }
    finish(plan);
    return plan;
}

TopDownPlan make_plan(TreeStyle style, const Shape& shape, bool show_ids, int max_rows) {
    return style == TreeStyle::Columns ? plan_columns(shape, show_ids) : plan_compact(shape, show_ids, max_rows);
}

bool fits(const TopDownPlan& plan, int width, int height) { return plan.width <= width && plan.rows <= height; }

/// True if no two labels on a row touch, no two connectors on a row touch,
/// and each child is on its own side of its parent.
bool is_clean(const TopDownPlan& plan, const Shape& shape) {
    std::map<int, std::vector<std::pair<int, int>>> spans;  // Row -> [first, last] columns.
    for (std::size_t i = 0; i < plan.order.size(); ++i) {
        const Placed& p = plan.order[i];
        if (plan.x[i] < 0) {
            return false;
        }
        spans[2 * p.depth].emplace_back(plan.x[i], plan.x[i] + label_width(plan, i) - 1);
        const ShapeNode& node = shape.nodes[static_cast<std::size_t>(p.node)];
        int first = plan.center[i];
        int last = plan.center[i];
        if (node.left >= 0) {
            first = plan.center[static_cast<std::size_t>(plan.position[static_cast<std::size_t>(node.left)])];
            if (first >= plan.center[i]) return false;
        }
        if (node.right >= 0) {
            last = plan.center[static_cast<std::size_t>(plan.position[static_cast<std::size_t>(node.right)])];
            if (last <= plan.center[i]) return false;
        }
        if (node.left >= 0 || node.right >= 0) {
            spans[2 * p.depth + 1].emplace_back(first, last);
        }
    }
    for (auto& [row, list] : spans) {
        std::sort(list.begin(), list.end());
        for (std::size_t j = 1; j < list.size(); ++j) {
            if (list[j].first < list[j - 1].second + 2) {
                return false;
            }
        }
    }
    return true;
}

/// Moves each label of `plan` to the column of the same value in the plan
/// of `reference`. Returns false (and changes nothing) if a value is not in
/// the reference, or if the result is not clean.
bool align_to(TopDownPlan& plan, const Shape& shape, const TopDownPlan& reference, const Shape& reference_shape) {
    std::map<std::string, int> column;
    for (std::size_t i = 0; i < reference.order.size(); ++i) {
        column[reference_shape.nodes[static_cast<std::size_t>(reference.order[i].node)].value] = reference.x[i];
    }
    TopDownPlan moved = plan;
    for (std::size_t i = 0; i < moved.order.size(); ++i) {
        const auto it = column.find(shape.nodes[static_cast<std::size_t>(moved.order[i].node)].value);
        if (it == column.end()) {
            return false;
        }
        moved.x[i] = it->second;
    }
    finish(moved);
    if (moved.width > reference.width || !is_clean(moved, shape)) {
        return false;
    }
    // The reference sets the size, so the drawing does not move on the canvas.
    moved.width = reference.width;
    moved.rows = std::max(moved.rows, reference.rows);
    plan = std::move(moved);
    return true;
}

void draw_top_down(const Shape& shape, const TreeMarks& marks, const TopDownPlan& plan, Canvas& canvas) {
    const int x0 = std::max((canvas.width() - plan.width) / 2, 0);
    const int y0 = std::clamp((canvas.height() - plan.rows) / 4, 0, 2);
    for (std::size_t i = 0; i < plan.order.size(); ++i) {
        const Placed& p = plan.order[i];
        const ShapeNode& node = shape.nodes[static_cast<std::size_t>(p.node)];
        const NodeMark mark = mark_of(marks, node.id);
        canvas.write(x0 + plan.x[i], y0 + 2 * p.depth, plan.labels[i], mark.tone, mark.bold);

        if (node.left < 0 && node.right < 0) {
            continue;
        }
        const int row = y0 + 2 * p.depth + 1;
        const int middle = x0 + plan.center[i];
        bool junction_lit = false;
        if (node.left >= 0) {
            const auto child = static_cast<std::size_t>(plan.position[static_cast<std::size_t>(node.left)]);
            const bool on = lit(marks, shape.nodes[static_cast<std::size_t>(node.left)].id);
            junction_lit = junction_lit || on;
            const Tone tone = on ? Tone::Target : Tone::Muted;
            const int start = x0 + plan.center[child];
            canvas.put(start, row, "┌", tone);
            for (int x = start + 1; x < middle; ++x) canvas.put(x, row, "─", tone);
        }
        if (node.right >= 0) {
            const auto child = static_cast<std::size_t>(plan.position[static_cast<std::size_t>(node.right)]);
            const bool on = lit(marks, shape.nodes[static_cast<std::size_t>(node.right)].id);
            junction_lit = junction_lit || on;
            const Tone tone = on ? Tone::Target : Tone::Muted;
            const int end = x0 + plan.center[child];
            for (int x = middle + 1; x < end; ++x) canvas.put(x, row, "─", tone);
            canvas.put(end, row, "┐", tone);
        }
        const char* junction = node.left >= 0 && node.right >= 0 ? "┴" : node.left >= 0 ? "┘" : "└";
        canvas.put(middle, row, junction, junction_lit ? Tone::Target : Tone::Muted);
    }
}


bool draw_sideways(const Shape& shape, const TreeMarks& marks, Canvas& canvas) {
    const std::vector<Placed> order = in_order(shape);
    const int n = static_cast<int>(order.size());
    const int width = canvas.width();
    const int height = canvas.height();
    bool clipped = false;

    std::vector<int> row_of(shape.nodes.size(), 0);
    std::vector<int> depth_of(shape.nodes.size(), 0);
    int focus_row = 0;
    for (int r = 0; r < n; ++r) {
        const Placed& p = order[static_cast<std::size_t>(r)];
        row_of[static_cast<std::size_t>(p.node)] = r;
        depth_of[static_cast<std::size_t>(p.node)] = p.depth;
        if (marks.focus && shape.nodes[static_cast<std::size_t>(p.node)].id == *marks.focus) {
            focus_row = r;
        }
    }

    // The indent step: 3 columns ("┌─" and the label) if each label can keep
    // at least 6 columns, else 2 columns ("┌" and the label).
    int step = 3;
    for (const Placed& p : order) {
        if (p.depth * 3 + 6 > width) {
            step = 2;
            break;
        }
    }
    // Shift the drawing to the left if the focus node is too far to the right.
    int x_shift = 0;
    if (marks.focus) {
        const int focus_depth = depth_of[static_cast<std::size_t>(order[static_cast<std::size_t>(focus_row)].node)];
        const int focus_column = focus_depth * step;
        if (focus_column + 8 > width) {
            x_shift = focus_column + 8 - width;
            clipped = true;
        }
    }

    // The visible rows. If the tree has more rows than the area, the first
    // and the last row tell how many rows are not visible.
    int first = 0;
    int content = height;
    int y0 = n + 2 <= height ? 1 : 0;
    if (n > height) {
        content = std::max(height - 2, 1);
        first = std::clamp(focus_row - content / 2, 0, std::max(n - content, 0));
        y0 = 1;
        clipped = true;
    }
    const auto visible = [&](int r) { return r >= first && r < first + content; };
    const auto y_of = [&](int r) { return y0 + r - first; };

    for (int r = 0; r < n; ++r) {
        const Placed& p = order[static_cast<std::size_t>(r)];
        const ShapeNode& node = shape.nodes[static_cast<std::size_t>(p.node)];
        const int label_x = p.depth * step - x_shift;
        if (p.parent >= 0) {
            const int parent_row = row_of[static_cast<std::size_t>(p.parent)];
            const int line_x = depth_of[static_cast<std::size_t>(p.parent)] * step + 1 - x_shift;
            const Tone tone = lit(marks, node.id) ? Tone::Target : Tone::Muted;
            const int low = std::min(r, parent_row) + 1;
            const int high = std::max(r, parent_row);
            for (int between = low; between < high; ++between) {
                if (visible(between)) canvas.put(line_x, y_of(between), "│", tone);
            }
            if (visible(r)) {
                canvas.put(line_x, y_of(r), p.left ? "┌" : "└", tone);
                for (int x = line_x + 1; x < label_x; ++x) canvas.put(x, y_of(r), "─", tone);
            }
        }
        if (!visible(r)) {
            continue;
        }
        const NodeMark mark = mark_of(marks, node.id);
        const int room = width - std::max(label_x, 0);
        if (label_x >= width) {
            canvas.put(width - 1, y_of(r), "…", Tone::Muted);
            clipped = true;
            continue;
        }
        std::size_t max_label = kSidewaysLabelWidth;
        if (room > 0) {
            max_label = std::min(max_label, static_cast<std::size_t>(room));
        }
        const std::string label = node_label(node, std::max<std::size_t>(max_label, 1), marks.show_ids);
        if (display_width(label) > static_cast<std::size_t>(room)) {
            clipped = true;
        }
        canvas.write(label_x, y_of(r), truncate(label, static_cast<std::size_t>(std::max(room, 1))), mark.tone,
                     mark.bold);
    }
    if (n > height) {
        if (first > 0) {
            canvas.write(1, 0, "⋮ " + std::to_string(first) + " above", Tone::Muted);
        }
        const int below = n - (first + content);
        if (below > 0) {
            canvas.write(1, height - 1, "⋮ " + std::to_string(below) + " below", Tone::Muted);
        }
    }
    return clipped;
}

}  // namespace

std::string node_label(const ShapeNode& node, std::size_t max_width, bool show_ids) {
    if (!show_ids) {
        return truncate(node.value, max_width);
    }
    const std::string id = " #" + std::to_string(node.id);
    const std::size_t room = max_width > display_width(id) + 1 ? max_width - display_width(id) : 1;
    return truncate(node.value, room) + id;
}

std::optional<TreeStyle> top_down_style(const Shape& shape, int width, int height, bool show_ids) {
    for (const TreeStyle style : {TreeStyle::Columns, TreeStyle::Compact}) {
        if (fits(make_plan(style, shape, show_ids, height), width, height)) {
            return style;
        }
    }
    return std::nullopt;
}

bool fits_top_down(const Shape& shape, int width, int height, bool show_ids) {
    return top_down_style(shape, width, height, show_ids).has_value();
}

TreeDrawing draw_tree(const Shape& shape, const TreeMarks& marks, int width, int height, bool allow_top_down,
                      const Shape* reference) {
    TreeDrawing drawing{Canvas{width, height}, TreeStyle::Columns, false};
    if (shape.empty() || width <= 0 || height <= 0) {
        return drawing;
    }
    if (allow_top_down) {
        if (reference == &shape || (reference != nullptr && reference->empty())) {
            reference = nullptr;
        }
        // With a reference, the style is the style of the reference, so that
        // the style does not change in the middle of an animation.
        const std::optional<TreeStyle> chosen =
            reference != nullptr ? top_down_style(*reference, width, height, marks.show_ids) : std::nullopt;
        std::vector<TreeStyle> styles{TreeStyle::Columns, TreeStyle::Compact};
        if (chosen == TreeStyle::Compact) {
            std::swap(styles[0], styles[1]);
        }
        for (const TreeStyle style : styles) {
            TopDownPlan plan = make_plan(style, shape, marks.show_ids, height);
            if (chosen == style) {
                align_to(plan, shape, make_plan(style, *reference, marks.show_ids, height), *reference);
            }
            if (fits(plan, width, height)) {
                drawing.style = style;
                draw_top_down(shape, marks, plan, drawing.canvas);
                return drawing;
            }
        }
    }
    drawing.style = TreeStyle::Sideways;
    drawing.clipped = draw_sideways(shape, marks, drawing.canvas);
    return drawing;
}

}  // namespace strset::tui
