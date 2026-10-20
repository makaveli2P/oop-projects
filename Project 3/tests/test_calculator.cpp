// Tests for each command of the set calculator.
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "strset/calculator.h"

using namespace strset;

namespace {

SetCalculator with(std::initializer_list<const char*> s1, std::initializer_list<const char*> s2) {
    SetCalculator calc;
    for (const char* w : s2) calc.add(w);
    calc.switch_sets();
    for (const char* w : s1) calc.add(w);
    return calc;
}

}  // namespace

TEST_CASE("a adds to S1", "[calc][add]") {
    SetCalculator calc;
    CHECK(calc.add("pear").message == "Added \"pear\" to S1.");
    CHECK(calc.add("pear").message == "\"pear\" is already in S1.");
    CHECK(calc.add("").message == "Nothing to add: the string is empty.");
    CHECK(calc.add("two words").changed);
    CHECK(format_set(calc.s1()) == "{\"pear\", \"two words\"}");
    CHECK(format_set(calc.s2()) == "{}");
}

TEST_CASE("e erases S1 only", "[calc][erase]") {
    SetCalculator calc = with({"a", "b"}, {"c"});
    CHECK(calc.erase().message == "Erased S1: freed 2 nodes.");
    CHECK(calc.s1().empty());
    CHECK(format_set(calc.s2()) == "{\"c\"}");
    CHECK(calc.erase().message == "S1 is already empty.");
}

TEST_CASE("s switches the sets", "[calc][switch]") {
    SetCalculator calc = with({"a"}, {"b", "c"});
    const Node* r1 = calc.s1().root();
    const Node* r2 = calc.s2().root();
    CHECK(calc.switch_sets().message == "Switched S1 and S2.");
    CHECK(calc.s1().root() == r2);
    CHECK(calc.s2().root() == r1);
    CHECK(format_set(calc.s1()) == "{\"b\", \"c\"}");
}

TEST_CASE("b tells if S2 is a proper subset of S1", "[calc][subset]") {
    CHECK(with({"a", "b"}, {"a"}).subset().message == "Yes: S2 is a proper subset of S1.");
    CHECK(with({"a"}, {}).subset().verdict == true);
    CHECK(with({"a", "b"}, {"a", "b"}).subset().message ==
          "No: S2 is not a proper subset of S1. S1 and S2 have the same strings.");
    CHECK(with({}, {}).subset().verdict == false);
    CHECK(with({"a", "b"}, {"a", "x"}).subset().message ==
          "No: S2 is not a proper subset of S1. \"x\" is in S2 but not in S1.");
    CHECK(with({"a"}, {"a", "b"}).subset().verdict == false);  // S1 ⊂ S2 is not S2 ⊂ S1.
}

TEST_CASE("c deep copies S1 into S2", "[calc][copy]") {
    SetCalculator calc = with({"m", "d"}, {"old", "older"});
    CHECK(calc.copy().message == "Copied S1 into S2: 2 new nodes.");
    CHECK(calc.s1() == calc.s2());
    CHECK(calc.s1().root() != calc.s2().root());
    calc.add("x");
    CHECK(calc.s2().size() == 2);  // The copy is independent.
}

TEST_CASE("l lists the sets", "[calc][list]") {
    SetCalculator calc = with({"b", "a"}, {"c"});
    CHECK(calc.list().message == "S1 and S2 in alphabetical order:");
    CHECK(format_set(calc.s1()) == "{\"a\", \"b\"}");
}

TEST_CASE("u and i store the result in S1", "[calc][union][intersection]") {
    SetCalculator calc = with({"a", "b"}, {"b", "c"});
    CHECK(calc.unite().message == "S1 = S1 ∪ S2: added 1 string.");
    CHECK(format_set(calc.s1()) == "{\"a\", \"b\", \"c\"}");
    CHECK(format_set(calc.s2()) == "{\"b\", \"c\"}");
    CHECK(calc.intersect().message == "S1 = S1 ∩ S2: removed 1 string.");
    CHECK(format_set(calc.s1()) == "{\"b\", \"c\"}");
    CHECK(calc.intersect().message == "S1 = S1 ∩ S2: removed 0 strings.");
}

TEST_CASE("run dispatches the commands", "[calc]") {
    SetCalculator calc;
    CHECK(calc.run(Command{CommandKind::Add, "kiwi"}).changed);
    CHECK(calc.run(Command{CommandKind::Copy, {}}).changed);
    CHECK(calc.run(Command{CommandKind::Quit, {}}).message.empty());
    CHECK(calc.run(Command{CommandKind::Help, {}}).message.empty());
    CHECK(calc.s2().contains("kiwi"));
}
