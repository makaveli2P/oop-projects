// Tests for the tree drawings: the top-down styles, the Sideways style, long
// chains and long strings.
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

#include "strset/BST.h"
#include "strset/trace.h"
#include "strset_tui/format.h"
#include "strset_tui/tree_layout.h"

using namespace strset;
using namespace strset::tui;

namespace {

Shape shape_from(std::initializer_list<const char*> values) {
    BST tree;
    for (const char* value : values) tree.insert(String{value});
    return shape_of(tree);
}

std::string all_rows(const Canvas& canvas) {
    std::string text;
    for (int y = 0; y < canvas.height(); ++y) text += canvas.row_text(y) + "\n";
    return text;
}

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("A small tree draws top-down with connectors", "[tui][layout]") {
    const Shape shape = shape_from({"mango", "apple", "pear"});
    const TreeDrawing drawing = draw_tree(shape, TreeMarks{}, 40, 10);
    REQUIRE(drawing.style == TreeStyle::Columns);
    CHECK_FALSE(drawing.clipped);
    const std::string text = all_rows(drawing.canvas);
    INFO(text);
    CHECK(contains(text, "mango"));
    CHECK(contains(text, "┌"));
    CHECK(contains(text, "┴"));
    CHECK(contains(text, "┐"));
    // The root is on a row above its children.
    int root_row = -1;
    int child_row = -1;
    for (int y = 0; y < drawing.canvas.height(); ++y) {
        const std::string row = drawing.canvas.row_text(y);
        if (contains(row, "mango")) root_row = y;
        if (contains(row, "apple")) child_row = y;
    }
    CHECK(root_row >= 0);
    CHECK(child_row == root_row + 2);
}

TEST_CASE("The labels are in alphabetical order from left to right", "[tui][layout]") {
    const Shape shape = shape_from({"m", "f", "t", "b", "h", "p", "w"});
    const TreeDrawing drawing = draw_tree(shape, TreeMarks{}, 60, 12);
    REQUIRE(drawing.style == TreeStyle::Columns);
    std::string order;
    for (int x = 0; x < drawing.canvas.width(); ++x) {
        for (int y = 0; y < drawing.canvas.height(); ++y) {
            const std::string& glyph = drawing.canvas.at(x, y).glyph;
            if (glyph.size() == 1 && glyph[0] >= 'a' && glyph[0] <= 'z') order += glyph;
        }
    }
    CHECK(order == "bfhmptw");
}

TEST_CASE("A long chain draws sideways and keeps the focus visible", "[tui][layout]") {
    BST tree;
    for (int i = 0; i < 40; ++i) tree.insert(String{std::string_view{"k" + std::to_string(100 + i)}});
    const Shape shape = shape_of(tree);
    TreeMarks marks;
    marks.focus = shape.nodes.back().id;  // The deepest node: "k139".
    const TreeDrawing drawing = draw_tree(shape, marks, 50, 12);
    REQUIRE(drawing.style == TreeStyle::Sideways);
    CHECK(drawing.clipped);
    const std::string text = all_rows(drawing.canvas);
    INFO(text);
    CHECK(contains(text, "k139"));
    CHECK(contains(text, "above"));
    for (int y = 0; y < drawing.canvas.height(); ++y) {
        CHECK(display_width(drawing.canvas.row_text(y)) <= 50);
    }
}

TEST_CASE("A long string is cut with an ellipsis", "[tui][layout]") {
    const std::string long_value(80, 'z');
    const Shape shape = shape_from({"m", long_value.c_str()});
    const TreeDrawing drawing = draw_tree(shape, TreeMarks{}, 40, 10);
    const std::string text = all_rows(drawing.canvas);
    INFO(text);
    CHECK(contains(text, "…"));
    CHECK_FALSE(contains(text, long_value));
    for (int y = 0; y < drawing.canvas.height(); ++y) {
        CHECK(display_width(drawing.canvas.row_text(y)) <= 40);
    }
}

TEST_CASE("Node labels can show the node id", "[tui][layout]") {
    const ShapeNode node{42, "pear", -1, -1};
    CHECK(node_label(node, 12, false) == "pear");
    CHECK(node_label(node, 12, true) == "pear #42");
    CHECK(display_width(node_label(ShapeNode{7, std::string(30, 'a'), -1, -1}, 12, true)) <= 12);
}

TEST_CASE("A reference shape keeps the columns stable", "[tui][layout]") {
    const Shape small = shape_from({"m", "c"});
    const Shape large = shape_from({"m", "c", "x"});
    const TreeDrawing before = draw_tree(small, TreeMarks{}, 40, 10, true, &large);
    const TreeDrawing after = draw_tree(large, TreeMarks{}, 40, 10, true, &large);
    const auto column_of = [](const Canvas& canvas, const std::string& glyph) {
        for (int y = 0; y < canvas.height(); ++y) {
            for (int x = 0; x < canvas.width(); ++x) {
                if (canvas.at(x, y).glyph == glyph) return x;
            }
        }
        return -1;
    };
    CHECK(column_of(before.canvas, "m") == column_of(after.canvas, "m"));
    CHECK(column_of(before.canvas, "c") == column_of(after.canvas, "c"));
}

TEST_CASE("The text helpers count code points", "[tui][format]") {
    CHECK(display_width("pear") == 4);
    CHECK(display_width("é…") == 2);
    CHECK(truncate("abcdef", 4) == "abc…");
    CHECK(truncate("abc", 4) == "abc");
    CHECK(pad_right("ab", 4) == "ab  ");
    CHECK(pad_left("ab", 4) == "  ab");
    CHECK(glyphs("a\tb") == std::vector<std::string>{"a", "·", "b"});
    CHECK(drop_last_glyph("pé") == "p");
    CHECK(speed_text(std::chrono::milliseconds{500}) == "2");
    CHECK(speed_text(std::chrono::milliseconds{700}) == "1.4");
}

TEST_CASE("A wide tree that fits only when packed uses the Compact style", "[tui][layout]") {
    const Shape shape = shape_from({"mango", "apple", "pear", "cherry", "kiwi", "banana", "zebra", "plum"});
    REQUIRE_FALSE(draw_tree(shape, TreeMarks{}, 30, 12).style == TreeStyle::Columns);
    TreeMarks ids;
    ids.show_ids = true;
    const TreeDrawing drawing = draw_tree(shape, ids, 44, 12);
    CHECK(drawing.style == TreeStyle::Compact);
    const std::string text = all_rows(drawing.canvas);
    INFO(text);
    for (const ShapeNode& node : shape.nodes) {
        CHECK(contains(text, node_label(node, kTopDownLabelWidth, true)));
    }
}

TEST_CASE("The Compact style never lets two labels touch", "[tui][layout]") {
    // A small linear congruential generator, so the test is the same on each run.
    std::uint32_t seed = 12345;
    const auto next = [&seed] {
        seed = seed * 1664525U + 1013904223U;
        return seed >> 8;
    };
    int compact = 0;
    for (int round = 0; round < 200; ++round) {
        BST tree;
        const std::uint32_t count = 1 + next() % 14;
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t length = 1 + next() % 9;
            std::string value;
            for (std::uint32_t j = 0; j < length; ++j) value += static_cast<char>('a' + next() % 26);
            tree.insert(String{std::string_view{value}});
        }
        const Shape shape = shape_of(tree);
        // One column less than the Columns style needs, so that it must pack.
        int columns_width = -2;
        for (const ShapeNode& node : shape.nodes) {
            columns_width += static_cast<int>(display_width(node_label(node, kTopDownLabelWidth, false))) + 2;
        }
        const TreeDrawing drawing = draw_tree(shape, TreeMarks{}, columns_width - 1, 40);
        if (shape.nodes.size() == 1) continue;
        REQUIRE(drawing.style == TreeStyle::Compact);
        ++compact;
        const std::string text = all_rows(drawing.canvas);
        INFO(text);
        for (const ShapeNode& node : shape.nodes) {
            // Each label is complete and has a space or the edge on each side.
            const std::string label = node_label(node, kTopDownLabelWidth, false);
            bool intact = false;
            for (int y = 0; y < drawing.canvas.height() && !intact; ++y) {
                const std::string row = " " + drawing.canvas.row_text(y) + " ";
                intact = contains(row, " " + label + " ");
            }
            CHECK(intact);
        }
    }
    CHECK(compact > 150);
}
