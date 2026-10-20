// Tests for String: the rule of five, the comparison and the edge cases.
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "strset/aString.h"

using strset::String;

static_assert(std::is_nothrow_default_constructible_v<String>);
static_assert(std::is_nothrow_move_constructible_v<String>);
static_assert(std::is_nothrow_move_assignable_v<String>);
static_assert(std::is_nothrow_destructible_v<String>);
static_assert(std::is_nothrow_swappable_v<String>);
static_assert(std::is_copy_constructible_v<String> && std::is_copy_assignable_v<String>);
// The course asks for two data members: an int and a char*. String has no other data.
struct TwoMembers {
    int size;
    char* chars;
};
static_assert(sizeof(String) == sizeof(TwoMembers));

TEST_CASE("A String holds a copy of its text", "[string]") {
    char text[] = "pear";
    const String s{text};
    text[0] = 'b';  // The String owns its own buffer.
    CHECK(s.view() == "pear");
    CHECK(s.size() == 4);
    CHECK(std::string{s.c_str()} == "pear");
    CHECK(s.buffer() != text);
}

TEST_CASE("The empty String allocates nothing", "[string]") {
    const String a;
    const String b{static_cast<const char*>(nullptr)};
    const String c{""};
    for (const String* s : {&a, &b, &c}) {
        CHECK(s->empty());
        CHECK(s->size() == 0);
        CHECK(s->buffer() == nullptr);
        CHECK(std::string{s->c_str()}.empty());
    }
}

TEST_CASE("A String can hold '\\0' characters", "[string]") {
    const std::string raw("a\0b", 3);
    const String s{std::string_view{raw}};
    CHECK(s.size() == 3);
    CHECK(s.view() == std::string_view{raw});
    CHECK(s.c_str()[3] == '\0');
    CHECK(s > String{"a"});
}

TEST_CASE("The copy constructor makes a deep copy", "[string][copy]") {
    const String original{"banana"};
    const String copy{original};
    CHECK(copy == original);
    CHECK(copy.buffer() != original.buffer());
}

TEST_CASE("Copy assignment makes a deep copy and is safe for self-assignment", "[string][copy]") {
    String a{"apple"};
    String b{"a much longer string"};
    a = b;
    CHECK(a == b);
    CHECK(a.buffer() != b.buffer());
    a = String{};
    CHECK(a.empty());

    String c{"self"};
    const String& alias = c;
    c = alias;
    CHECK(c.view() == "self");
}

TEST_CASE("Moves transfer the buffer and leave an empty String", "[string][move]") {
    String a{"kiwi"};
    const char* buffer = a.buffer();
    String b{std::move(a)};
    CHECK(b.buffer() == buffer);
    CHECK(b.view() == "kiwi");
    CHECK(a.empty());  // NOLINT(bugprone-use-after-move): the moved-from state is specified.
    CHECK(a.buffer() == nullptr);

    String c{"plum"};
    c = std::move(b);
    CHECK(c.buffer() == buffer);
    CHECK(b.empty());  // NOLINT(bugprone-use-after-move)

    String& self = c;
    c = std::move(self);
    CHECK(c.view() == "kiwi");
}

TEST_CASE("swap exchanges the buffers", "[string]") {
    String a{"fig"};
    String b{"lime"};
    const char* fa = a.buffer();
    swap(a, b);
    CHECK(a.view() == "lime");
    CHECK(b.view() == "fig");
    CHECK(b.buffer() == fa);
}

TEST_CASE("compare orders the strings as strcmp does", "[string][compare]") {
    CHECK(String{"apple"} < String{"pear"});
    CHECK(String{"pear"} > String{"apple"});
    CHECK(String{"pea"} < String{"pear"});    // A prefix is less.
    CHECK(String{""} < String{"a"});
    CHECK(String{"Zebra"} < String{"apple"});  // Upper case comes first in ASCII.
    CHECK(String{"z"} < String{"\xC3\xA9"});   // Bytes compare as unsigned: 'é' (0xC3 0xA9) > 'z'.
    CHECK(String{"same"} == String{"same"});
    CHECK(String{"same"} <= String{"same"});
    CHECK(String{"same"} >= String{"same"});
    CHECK(String{"a"} != String{"b"});
    CHECK(String{"abc"}.compare(String{"abd"}) < 0);
    CHECK(String{"abd"}.compare(String{"abc"}) > 0);
    CHECK(String{"abc"}.compare(String{"abc"}) == 0);
}

TEST_CASE("first_difference finds the first different character", "[string][compare]") {
    CHECK(strset::first_difference(String{"pear"}, String{"peach"}) == 3);
    CHECK(strset::first_difference(String{"kiwi"}, String{"pear"}) == 0);
    CHECK(strset::first_difference(String{"pea"}, String{"pear"}) == 3);
    CHECK(strset::first_difference(String{"same"}, String{"same"}) == 4);
    CHECK(strset::first_difference(String{}, String{"x"}) == 0);
}

TEST_CASE("at checks the index", "[string]") {
    const String s{"abc"};
    CHECK(s.at(0) == 'a');
    CHECK(s.at(2) == 'c');
    CHECK_THROWS_AS(s.at(3), std::out_of_range);
    CHECK_THROWS_AS(s.at(-1), std::out_of_range);
    CHECK_THROWS_AS(String{}.at(0), std::out_of_range);
}

TEST_CASE("operator<< writes the characters", "[string]") {
    std::ostringstream out;
    out << String{"hello"} << '|' << String{};
    CHECK(out.str() == "hello|");
}
