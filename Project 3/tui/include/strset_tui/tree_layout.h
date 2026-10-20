// tree_layout.h - draws a tree Shape on a grid of character cells. No FTXUI
// code, so the tests can examine the drawings.
//
// There are three styles. The visualizer uses the first style that fits:
// - Columns: the root at the top, the children below it, and each node in
//   its own columns, in alphabetical order from left to right. It is the
//   usual picture of a BST.
// - Compact: also top-down, but each subtree is packed against its sibling
//   (a "tidy tree"), so a wider tree fits.
// - Sideways: one row for each node, in alphabetical order from the top. A
//   left child is above its parent, a right child is below it, and the
//   indentation is the depth. It fits long chains and long strings.
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "strset/trace.h"

namespace strset::tui {

/// The meaning of a color. The views map each tone to one terminal color.
enum class Tone : std::uint8_t {
    Plain,   ///< The default color of the terminal.
    Muted,   ///< Frames, connectors and labels.
    Source,  ///< The node that a step examines or reads.
    Target,  ///< A new or changed node, and the search path.
    Ok,      ///< A match that a lookup found, a "yes" verdict.
    Alert,   ///< A node that is freed or removed, a "no" verdict.
};

struct Cell {
    std::string glyph = " ";
    Tone tone = Tone::Plain;
    bool bold = false;
};

class Canvas {
public:
    Canvas(int width, int height);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] const Cell& at(int x, int y) const;
    /// Writes one glyph. A position outside the canvas does nothing.
    void put(int x, int y, const std::string& glyph, Tone tone, bool bold = false);
    /// Writes UTF-8 text from (x, y), one glyph for each cell. Returns the
    /// number of cells.
    int write(int x, int y, const std::string& text, Tone tone, bool bold = false);
    /// The glyphs of one row, without the spaces at the end.
    [[nodiscard]] std::string row_text(int y) const;

private:
    int width_;
    int height_;
    std::vector<Cell> cells_;
};

/// How to show one node.
struct NodeMark {
    Tone tone = Tone::Plain;
    bool bold = false;
};

struct TreeMarks {
    std::map<NodeId, NodeMark> nodes;
    /// The nodes whose link from the parent is part of the path (amber).
    std::set<NodeId> lit_links;
    /// The node that must stay visible when the tree is larger than the area.
    std::optional<NodeId> focus;
    /// Show "#id" after each value.
    bool show_ids = false;
};

enum class TreeStyle : std::uint8_t { Columns, Compact, Sideways };

struct TreeDrawing {
    Canvas canvas;
    TreeStyle style;
    /// True if a part of the tree is not visible (rows or columns cut).
    bool clipped = false;
};

/// The longest label (in columns) of the top-down styles. A longer value is cut with "…".
inline constexpr std::size_t kTopDownLabelWidth = 12;
/// The longest label of the Sideways style.
inline constexpr std::size_t kSidewaysLabelWidth = 28;

/// The label of a node: the value (cut to `max_width`) and "#id" if `show_ids`.
[[nodiscard]] std::string node_label(const ShapeNode& node, std::size_t max_width, bool show_ids);

/// The first top-down style (Columns, then Compact) that shows the complete
/// tree in width x height, if there is one.
[[nodiscard]] std::optional<TreeStyle> top_down_style(const Shape& shape, int width, int height, bool show_ids);
/// True if a top-down style shows the complete tree in width x height.
[[nodiscard]] bool fits_top_down(const Shape& shape, int width, int height, bool show_ids);

/// Draws the tree. It uses a top-down style if `allow_top_down` is true and
/// the tree fits. Otherwise it uses Sideways.
///
/// `reference` is optional: a larger shape of the same set, for example the
/// tree at the end of an animation. The drawing then uses the style of the
/// reference, and if each value of `shape` is also in the reference, each
/// value keeps the column that it has in the reference. Thus the nodes do
/// not move while a tree grows or shrinks.
[[nodiscard]] TreeDrawing draw_tree(const Shape& shape, const TreeMarks& marks, int width, int height,
                                    bool allow_top_down = true, const Shape* reference = nullptr);

}  // namespace strset::tui
