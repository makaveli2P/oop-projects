// Tests for the trace that the commands record for the visualizer.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "strset/calculator.h"
#include "strset/trace.h"

using namespace strset;

namespace {

template <class T>
std::vector<const T*> events_of(const Trace& trace) {
    std::vector<const T*> result;
    for (const Step& step : trace.steps) {
        if (const auto* e = std::get_if<T>(&step.event)) result.push_back(e);
    }
    return result;
}

std::vector<std::string> narration(const Trace& trace) {
    std::vector<std::string> lines;
    for (const Step& step : trace.steps) lines.push_back(describe(step.event));
    return lines;
}

}  // namespace

TEST_CASE("An add records each comparison and the new link", "[trace][add]") {
    SetCalculator calc;
    calc.add("pear");
    calc.add("apple");
    Trace trace;
    calc.add("banana", &trace);
    CHECK(narration(trace) == std::vector<std::string>{
                                  "Add \"banana\" to S1. The search starts at the root.",
                                  "\"banana\" < \"pear\": go left. 'b' < 'p' at index 0.",
                                  "\"banana\" > \"apple\": go right. 'b' > 'a' at index 0.",
                                  "New node #" + std::to_string(calc.s1().root()->left()->right()->id()) +
                                      " \"banana\" is the right child of node #" +
                                      std::to_string(calc.s1().root()->left()->id()) + ".",
                                  "Added \"banana\" to S1.",
                              });
    // The first snapshot is before the insert, the last one after it.
    CHECK(trace.steps.front().trees->s1.nodes.size() == 2);
    CHECK(trace.steps.back().trees->s1.nodes.size() == 3);
    // Steps that do not change a tree share one snapshot.
    CHECK(trace.steps[1].trees == trace.steps[2].trees);
}

TEST_CASE("A comparison names the first different character", "[trace][compare]") {
    SetCalculator calc;
    calc.add("pear");
    Trace trace;
    calc.add("peach", &trace);
    const auto compares = events_of<event::Compare>(trace);
    REQUIRE(compares.size() == 1);
    CHECK(compares[0]->diff == 3);
    CHECK(describe(Event{*compares[0]}) ==
          "\"peach\" < \"pear\": go left. 'c' < 'r' at index 3.");

    Trace prefix;
    calc.add("pea", &prefix);
    CHECK(describe(Event{*events_of<event::Compare>(prefix)[0]}) ==
          "\"pea\" < \"pear\": go left. \"pea\" is a prefix of \"pear\".");

    Trace duplicate;
    calc.add("pear", &duplicate);
    CHECK(narration(duplicate)[1] == "\"pear\" = \"pear\": a match.");
    CHECK(narration(duplicate)[2] == "\"pear\" is already in S1. The set does not change.");
}

TEST_CASE("quoted makes an excerpt around the focus", "[trace][format]") {
    CHECK(quoted("pear") == "\"pear\"");
    const std::string longer = "abcdefghijklmnopqrstuvwxyz0123456789";
    CHECK(quoted(longer, 10, 0) == "\"abcdefghij…\"");
    const std::string excerpt = quoted(longer, 10, 30);
    CHECK(excerpt.find('4') != std::string::npos);  // The focus character (index 30) is visible.
    CHECK(excerpt.rfind("\"…", 0) == 0);
    // A cut never splits a UTF-8 character.
    const std::string accents = "ééééééééééééééé";  // 15 characters, 30 bytes.
    const std::string cut = quoted(accents, 9, 0);
    CHECK(cut == "\"ééééé…\"");
}

TEST_CASE("Erase frees the nodes leaves first; the snapshot still shows the node", "[trace][erase]") {
    SetCalculator calc;
    for (const char* w : {"m", "d", "t"}) calc.add(w);
    Trace trace;
    calc.erase(&trace);
    const auto frees = events_of<event::Free>(trace);
    REQUIRE(frees.size() == 3);
    CHECK(frees[0]->value == "d");
    CHECK(frees[1]->value == "t");
    CHECK(frees[2]->value == "m");  // The root goes last.
    CHECK(trace.steps[1].trees->s1.nodes.size() == 3);  // Before the first free.
    CHECK(trace.steps[2].trees->s1.nodes.size() == 2);
    CHECK(trace.steps.back().trees->s1.empty());
}

TEST_CASE("Copy records a new node for each node of S1", "[trace][copy]") {
    SetCalculator calc;
    calc.add("x");
    calc.switch_sets();
    for (const char* w : {"m", "d", "t"}) calc.add(w);  // S1 = {d, m, t}, S2 = {x}.
    Trace trace;
    calc.copy(&trace);
    CHECK(events_of<event::Free>(trace).size() == 1);
    const auto copies = events_of<event::Copy>(trace);
    REQUIRE(copies.size() == 3);
    CHECK(copies[0]->value == "m");  // Preorder.
    for (const auto* copy : copies) {
        CHECK(copy->set == SetName::S2);
        CHECK(copy->copy != copy->original);
    }
    CHECK(trace.steps.back().trees->s2.nodes.size() == 3);
}

TEST_CASE("Switch records the swap of the roots", "[trace][switch]") {
    SetCalculator calc;
    calc.add("pear");
    const NodeId root = calc.s1().root()->id();
    Trace trace;
    calc.switch_sets(&trace);
    const auto swaps = events_of<event::Swap>(trace);
    REQUIRE(swaps.size() == 1);
    CHECK_FALSE(swaps[0]->s1_root.has_value());
    CHECK(swaps[0]->s2_root == root);
    CHECK(describe(Event{*swaps[0]}) == "S1 and S2 swap their root pointers. No node moves.");
}

TEST_CASE("Subset records each lookup and the verdict", "[trace][subset]") {
    SetCalculator calc;
    for (const char* w : {"b", "a"}) calc.add(w);
    calc.copy();
    calc.add("c");  // S1 = {a, b, c}, S2 = {a, b}.
    Trace trace;
    const Outcome yes = calc.subset(&trace);
    CHECK(yes.verdict == true);
    CHECK(events_of<event::Visit>(trace).size() == 2);
    CHECK(events_of<event::Found>(trace).size() == 2);
    CHECK(std::get<event::Done>(trace.steps.back().event).verdict == true);
}

TEST_CASE("Intersection records each removal and its successor", "[trace][intersection]") {
    SetCalculator calc;
    for (const char* w : {"m", "d", "t", "a", "f"}) calc.add(w);
    calc.copy();
    calc.switch_sets();
    calc.erase();
    for (const char* w : {"a", "f", "t"}) calc.add(w);
    calc.switch_sets();  // S1 = {a, d, f, m, t}, S2 = {a, f, t}.
    Trace trace;
    calc.intersect(&trace);
    const auto removes = events_of<event::Remove>(trace);
    REQUIRE(removes.size() == 2);
    CHECK(removes[0]->value == "d");
    CHECK(removes[0]->successor_value == "f");
    CHECK(removes[1]->value == "m");
    CHECK(removes[1]->successor_value == "t");
    CHECK(events_of<event::Missing>(trace).size() == 2);
}

TEST_CASE("The limit keeps the first steps and the Done step", "[trace][limit]") {
    SetCalculator calc;
    for (const char* w : {"a", "b", "c", "d", "e", "f"}) calc.add(w);
    Trace trace;
    trace.limit = 4;
    calc.add("g", &trace);
    CHECK(trace.truncated);
    REQUIRE(trace.steps.size() == 5);
    CHECK(std::holds_alternative<event::Done>(trace.steps.back().event));
    CHECK(trace.steps.back().trees->s1.nodes.size() == 7);  // The Done snapshot is the final state.
}

TEST_CASE("shape_of lists the nodes in preorder with child indexes", "[trace][shape]") {
    const BST tree{"m", "d", "t", "a"};
    const Shape shape = shape_of(tree);
    REQUIRE(shape.nodes.size() == 4);
    CHECK(shape.nodes[0].value == "m");
    CHECK(shape.nodes[static_cast<std::size_t>(shape.nodes[0].left)].value == "d");
    CHECK(shape.nodes[static_cast<std::size_t>(shape.nodes[1].left)].value == "a");
    CHECK(shape.nodes[static_cast<std::size_t>(shape.nodes[0].right)].value == "t");
    CHECK(shape.root() == tree.root()->id());
    CHECK(shape.index_of(tree.root()->id()) == 0);
    CHECK(shape.index_of(0) == -1);
}
