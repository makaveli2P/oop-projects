// Tests for the visualizer controller and the keys. No terminal is needed.
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>

#include "strset_tui/controller.h"
#include "strset_tui/keys.h"

using namespace strset;
using namespace strset::tui;

namespace {

void press(Controller& c, const std::string& script) {
    for (const std::string& key : parse_key_script(script)) handle_key(c, key);
}

template <class T>
bool step_is(const Controller& c) {
    return c.step() != nullptr && std::holds_alternative<T>(c.step()->event);
}

/// The cell column where the ASCII `label` starts, if it is on the canvas.
std::optional<int> cell_column(const Canvas& canvas, const std::string& label) {
    const int n = static_cast<int>(label.size());
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x + n <= canvas.width(); ++x) {
            bool match = x == 0 || canvas.at(x - 1, y).glyph == " ";
            for (int i = 0; match && i < n; ++i) {
                match = canvas.at(x + i, y).glyph == std::string(1, label[static_cast<std::size_t>(i)]);
            }
            if (match && (x + n == canvas.width() || canvas.at(x + n, y).glyph == " ")) return x;
        }
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("The controller starts with no trace", "[tui][controller]") {
    const Controller c;
    CHECK_FALSE(c.has_trace());
    CHECK(c.step() == nullptr);
    CHECK(c.at_end());
    CHECK(c.trees().s1.empty());
    CHECK(c.trees().s2.empty());
    CHECK(c.progress().empty());
}

TEST_CASE("The add prompt runs an animated add", "[tui][controller][keys]") {
    Controller c;
    press(c, "a");
    REQUIRE(c.prompt_open());
    press(c, " pear{enter}");  // The space at the start does not count.
    CHECK_FALSE(c.prompt_open());
    REQUIRE(c.has_trace());
    CHECK(c.command_text() == "a pear");
    CHECK(c.running());
    CHECK(step_is<event::Begin>(c));
    // The calculator changed at once; the screen shows the step before the change.
    CHECK(c.calculator().s1().size() == 1);
    CHECK(c.trees().s1.empty());

    c.to_end();
    CHECK_FALSE(c.running());
    CHECK(step_is<event::Done>(c));
    CHECK(c.trees().s1.nodes.size() == 1);
}

TEST_CASE("An add marks the search path and the comparison", "[tui][controller]") {
    Controller c;
    c.run_quietly(Command{CommandKind::Add, "mango"});
    c.run_quietly(Command{CommandKind::Add, "apple"});
    c.run_quietly(Command{CommandKind::Add, "cherry"});
    c.start(Command{CommandKind::Add, "banana"});
    c.next();  // The first comparison: banana < mango.
    REQUIRE(step_is<event::Compare>(c));
    const NodeId mango = c.trees().s1.nodes[0].id;
    TreeMarks marks = c.marks(SetName::S1);
    CHECK(marks.nodes.at(mango).tone == Tone::Source);
    CHECK(marks.focus == mango);

    c.next();  // banana > apple.
    c.next();  // banana < cherry.
    REQUIRE(step_is<event::Compare>(c));
    marks = c.marks(SetName::S1);
    CHECK(marks.nodes.at(mango).tone == Tone::Target);  // Part of the path now.
    CHECK(marks.lit_links.count(mango) == 1);

    c.next();  // The new node.
    REQUIRE(step_is<event::Link>(c));
    const auto& link = std::get<event::Link>(c.step()->event);
    CHECK(link.value == "banana");
    CHECK(link.side == Side::Left);
    CHECK(c.marks(SetName::S1).nodes.at(link.node).tone == Tone::Target);
}

TEST_CASE("Step, back, run and tick move the cursor", "[tui][controller][keys]") {
    Controller c;
    c.run_quietly(Command{CommandKind::Add, "b"});
    c.start(Command{CommandKind::Add, "a"});
    REQUIRE(c.trace().steps.size() == 4);  // Begin, Compare, Link, Done.
    CHECK(c.running());

    press(c, "{space}");
    CHECK_FALSE(c.running());
    CHECK(c.cursor() == 1);
    press(c, "{left}{left}");
    CHECK(c.cursor() == 0);
    press(c, "g");
    CHECK(c.at_end());

    press(c, "r");  // At the end, a run starts again from the first step.
    CHECK(c.running());
    CHECK(c.cursor() == 0);
    CHECK(c.tick());
    CHECK(c.tick());
    CHECK(c.tick());
    CHECK(c.at_end());
    CHECK_FALSE(c.running());
    CHECK_FALSE(c.tick());
}

TEST_CASE("Speed keys change the tick interval within limits", "[tui][controller][keys]") {
    Controller c;
    const auto start = c.tick_interval();
    press(c, "+");
    CHECK(c.tick_interval() < start);
    for (int i = 0; i < 20; ++i) press(c, "+");
    CHECK(c.speed_level() == Controller::speed_level_count() - 1);
    for (int i = 0; i < 20; ++i) press(c, "-");
    CHECK(c.speed_level() == 0);
}

TEST_CASE("Each command key runs its command", "[tui][controller][keys]") {
    Controller c;
    c.run_quietly(Command{CommandKind::Add, "pear"});
    c.run_quietly(Command{CommandKind::Add, "apple"});
    for (const char key : std::string{"cbluise"}) {
        press(c, std::string(1, key));
        REQUIRE(c.has_trace());
        CHECK(c.command_text() == std::string(1, key));
        c.to_end();
        CHECK(step_is<event::Done>(c));
    }
    // After c, b, l, u, i, s, e: S1 had S2's content (switch), then e erased it.
    CHECK(c.calculator().s1().empty());
    CHECK(c.calculator().s2().size() == 2);
}

TEST_CASE("The subset command shows each lookup and the verdict", "[tui][controller]") {
    Controller c;
    c.run_quietly(Command{CommandKind::Add, "pear"});
    c.run_quietly(Command{CommandKind::Add, "apple"});
    c.run_quietly(Command{CommandKind::Copy, {}});
    c.run_quietly(Command{CommandKind::Add, "fig"});  // S1 = {apple, fig, pear}, S2 = {apple, pear}.
    c.start(Command{CommandKind::Subset, {}});
    c.to_end();
    const auto& done = std::get<event::Done>(c.step()->event);
    REQUIRE(done.verdict.has_value());
    CHECK(*done.verdict);
    std::string progress;
    for (const Token& token : c.progress()) progress += token.text;
    CHECK(progress == "In S1: apple ✓, pear ✓");
}

TEST_CASE("Copy and switch show the node ids", "[tui][controller]") {
    Controller c;
    c.run_quietly(Command{CommandKind::Add, "pear"});
    c.start(Command{CommandKind::Copy, {}});
    CHECK(c.marks(SetName::S1).show_ids);
    c.to_end();
    const NodeId original = c.trees().s1.nodes[0].id;
    const NodeId copy = c.trees().s2.nodes[0].id;
    CHECK(original != copy);  // The copy is a new allocation.

    c.start(Command{CommandKind::Switch, {}});
    c.to_end();
    CHECK(c.trees().s1.nodes[0].id == copy);
    CHECK(c.trees().s2.nodes[0].id == original);
}

TEST_CASE("Erase shows each node before it is freed", "[tui][controller]") {
    Controller c;
    for (const char* value : {"m", "c", "x"}) c.run_quietly(Command{CommandKind::Add, value});
    c.start(Command{CommandKind::Erase, {}});
    std::size_t frees = 0;
    for (const Step& step : c.trace().steps) {
        if (const auto* free = std::get_if<event::Free>(&step.event)) {
            ++frees;
            CHECK(step.trees->s1.index_of(free->node) >= 0);  // Still on the screen.
        }
    }
    CHECK(frees == 3);
    c.to_end();
    CHECK(c.trees().s1.empty());
}

TEST_CASE("The Done step of an add keeps the new node in focus", "[tui][controller]") {
    // A long chain uses the Sideways style, which scrolls to the focus node.
    Controller c;
    for (int i = 10; i < 50; ++i) c.run_quietly(Command{CommandKind::Add, "s" + std::to_string(i)});
    c.start(Command{CommandKind::Add, "s50"});
    c.to_end();
    REQUIRE(step_is<event::Done>(c));
    const Shape& s1 = c.trees().s1;
    const TreeMarks marks = c.marks(SetName::S1);
    REQUIRE(marks.focus.has_value());
    const int index = s1.index_of(*marks.focus);
    REQUIRE(index >= 0);
    CHECK(s1.nodes[static_cast<std::size_t>(index)].value == "s50");

    // The new node is on the screen of the last frame.
    const TreeDrawing drawing = draw_tree(s1, marks, 56, 24, c.allow_top_down(SetName::S1, 56, 24),
                                          c.layout_reference(SetName::S1));
    REQUIRE(drawing.style == TreeStyle::Sideways);
    bool visible = false;
    for (int y = 0; y < drawing.canvas.height(); ++y) {
        if (drawing.canvas.row_text(y).find("s50") != std::string::npos) visible = true;
    }
    CHECK(visible);
}

TEST_CASE("A copy over a larger S2 keeps the columns of the new nodes", "[tui][controller]") {
    Controller c;
    for (const char* value : {"d", "b", "f", "a", "c", "e", "g"}) c.run_quietly(Command{CommandKind::Add, value});
    c.run_quietly(Command{CommandKind::Switch, {}});
    for (const char* value : {"m", "z", "zz", "zzz"}) c.run_quietly(Command{CommandKind::Add, value});
    c.start(Command{CommandKind::Copy, {}});

    // The column of each label of S2, in each frame of the trace.
    std::map<std::string, std::set<int>> columns;
    std::size_t frames = 0;
    for (;;) {
        const Shape& s2 = c.trees().s2;
        if (!s2.empty()) {
            const TreeDrawing drawing = draw_tree(s2, c.marks(SetName::S2), 36, 17,
                                                  c.allow_top_down(SetName::S2, 36, 17),
                                                  c.layout_reference(SetName::S2));
            CHECK(drawing.style != TreeStyle::Sideways);
            for (const ShapeNode& node : s2.nodes) {
                const std::string label = node_label(node, kTopDownLabelWidth, true);
                if (const std::optional<int> x = cell_column(drawing.canvas, label)) {
                    columns[node.value + "|" + std::to_string(node.id)].insert(*x);
                }
            }
            ++frames;
        }
        if (c.at_end()) break;
        c.next();
    }
    CHECK(frames > 8);
    for (const auto& [label, seen] : columns) {
        INFO(label);
        CHECK(seen.size() == 1);  // The node does not move between frames.
    }
    CHECK(columns.size() == 11);  // 7 old nodes and 4 new nodes.
}

TEST_CASE("A command with too many steps shows the result", "[tui][controller]") {
    Controller c{10};
    for (int i = 0; i < 20; ++i) c.run_quietly(Command{CommandKind::Add, "v" + std::to_string(100 + i)});
    c.start(Command{CommandKind::List, {}});
    CHECK(c.trace().truncated);
    CHECK_FALSE(c.running());
    CHECK(c.at_end());
    CHECK_FALSE(c.notice().empty());
}

TEST_CASE("The prompt handles editing keys", "[tui][keys]") {
    Controller c;
    press(c, "apé{bs}e");
    CHECK(c.prompt_text() == "pe");
    press(c, "{esc}");
    CHECK_FALSE(c.prompt_open());
    CHECK_FALSE(c.has_trace());

    press(c, "a{enter}");  // An empty string is not added.
    CHECK_FALSE(c.has_trace());
    CHECK_FALSE(c.notice().empty());
}

TEST_CASE("Help opens and closes, and q quits", "[tui][keys]") {
    Controller c;
    CHECK(handle_key(c, "?") == KeyResult::Handled);
    CHECK(c.help_open());
    CHECK(handle_key(c, "x") == KeyResult::Handled);
    CHECK_FALSE(c.help_open());
    CHECK(handle_key(c, "z") == KeyResult::Ignored);
    CHECK(handle_key(c, "q") == KeyResult::Quit);
    CHECK(handle_key(c, "Escape") == KeyResult::Quit);
}

TEST_CASE("The key script parser knows the named keys", "[tui][keys]") {
    CHECK(parse_key_script("a{space}b{enter}") == std::vector<std::string>{"a", " ", "b", "Enter"});
    CHECK(parse_key_script("é{left}") == std::vector<std::string>{"é", "Left"});
    CHECK_THROWS_AS(parse_key_script("{nope}"), std::invalid_argument);
    CHECK_THROWS_AS(parse_key_script("{enter"), std::invalid_argument);
}
