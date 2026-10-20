// Tests for the classic command loop with a script on the input.
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include "strset/classic.h"

namespace {

std::string run(const std::string& input) {
    std::istringstream in{input};
    std::ostringstream out;
    REQUIRE(strset::run_classic(in, out) == 0);
    return out.str();
}

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("Each command prints S1 and S2 after it", "[classic]") {
    const std::string out = run("a pear\na apple\nc\na kiwi\nb\nq\n");
    CHECK(contains(out, "Added \"pear\" to S1.\nS1 = {\"pear\"}\nS2 = {}\n"));
    CHECK(contains(out, "Copied S1 into S2: 2 new nodes.\nS1 = {\"apple\", \"pear\"}\nS2 = {\"apple\", \"pear\"}\n"));
    CHECK(contains(out, "Yes: S2 is a proper subset of S1.\nS1 = {\"apple\", \"kiwi\", \"pear\"}\n"));
    CHECK(contains(out, "Quitting the set calculator.\n"));
}

TEST_CASE("Every command works in the loop", "[classic]") {
    const std::string out = run("a b\na a\ns\na c\nu\ni\nl\ne\nq\n");
    CHECK(contains(out, "Switched S1 and S2.\nS1 = {}\nS2 = {\"a\", \"b\"}\n"));
    CHECK(contains(out, "S1 = S1 ∪ S2: added 2 strings.\nS1 = {\"a\", \"b\", \"c\"}\n"));
    CHECK(contains(out, "S1 = S1 ∩ S2: removed 1 string.\nS1 = {\"a\", \"b\"}\n"));
    CHECK(contains(out, "S1 and S2 in alphabetical order:\nS1 = {\"a\", \"b\"}\nS2 = {\"a\", \"b\"}\n"));
    CHECK(contains(out, "Erased S1: freed 2 nodes.\nS1 = {}\n"));
}

TEST_CASE("a without a string asks for it", "[classic]") {
    const std::string out = run("a\n  fig  \na\n\nq\n");
    CHECK(contains(out, "String to add: Added \"fig\" to S1."));
    CHECK(contains(out, "String to add: Nothing to add: the string is empty."));
}

TEST_CASE("Bad lines give a message and change nothing", "[classic]") {
    const std::string out = run("x\ne now\n\n?\nq\n");
    CHECK(contains(out, "Unknown command 'x'. Type ? to see the commands.\n"));
    CHECK(contains(out, "The command 'e' takes no argument.\n"));
    CHECK(contains(out, "  a <string>  Add a string to S1\n"));
}

TEST_CASE("The end of the input quits like q", "[classic]") {
    const std::string out = run("a pear\n");  // No q: the original version looped forever here.
    CHECK(contains(out, "End of input.\nS1 = {\"pear\"}\nS2 = {}\nQuitting the set calculator.\n"));
    CHECK(contains(run("a"), "String to add: \nEnd of input."));
}

TEST_CASE("CRLF line ends work", "[classic]") {
    const std::string out = run("a pear\r\nl\r\nq\r\n");
    CHECK(contains(out, "Added \"pear\" to S1."));
    CHECK_FALSE(contains(out, "Unknown command"));
}
