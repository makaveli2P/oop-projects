// Tests for the set operations of BST: union, intersection and the proper
// subset check.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "strset/BST.h"

using strset::BST;
using strset::String;

namespace {

std::vector<std::string> values_of(const BST& tree) {
    std::vector<std::string> result;
    for (const String& value : tree.values()) result.emplace_back(value.view());
    return result;
}

}  // namespace

TEST_CASE("unite adds the values of the other set", "[ops][union]") {
    BST s1{"pear", "apple"};
    const BST s2{"kiwi", "apple", "zucchini"};
    CHECK(s1.unite(s2) == 2);
    CHECK(values_of(s1) == std::vector<std::string>{"apple", "kiwi", "pear", "zucchini"});
    CHECK(values_of(s2) == std::vector<std::string>{"apple", "kiwi", "zucchini"});  // S2 does not change.
    CHECK(s1.unite(s2) == 0);
    CHECK(s1.unite(s1) == 0);
}

TEST_CASE("unite into an empty set copies the shape and makes new nodes", "[ops][union]") {
    BST s1;
    const BST s2{"m", "d", "t"};
    s1.unite(s2);
    CHECK(s1.root()->value() == String{"m"});  // Preorder inserts keep the shape.
    CHECK(s1.root() != s2.root());              // The original version shared the nodes of S2 here.
    CHECK(s1.root()->value().buffer() != s2.root()->value().buffer());
}

TEST_CASE("intersect keeps only the common values", "[ops][intersection]") {
    BST s1{"m", "d", "t", "a", "f", "p", "z"};
    const BST s2{"a", "f", "t", "x"};
    CHECK(s1.intersect(s2) == 4);
    CHECK(values_of(s1) == std::vector<std::string>{"a", "f", "t"});
    CHECK(s1.intersect(s1) == 0);
    BST empty;
    CHECK(s1.intersect(empty) == 3);
    CHECK(s1.empty());
}

TEST_CASE("is_proper_subset_of follows the definition", "[ops][subset]") {
    const BST empty;
    const BST ab{"a", "b"};
    const BST abc{"c", "a", "b"};
    const BST abx{"a", "b", "x"};
    CHECK(empty.is_proper_subset_of(ab));       // ∅ ⊂ {a, b}
    CHECK_FALSE(empty.is_proper_subset_of(empty));
    CHECK(ab.is_proper_subset_of(abc));
    CHECK_FALSE(abc.is_proper_subset_of(ab));
    CHECK_FALSE(abc.is_proper_subset_of(abc));  // A set is not a proper subset of itself.
    CHECK_FALSE(abc.is_proper_subset_of(BST{"a", "b", "c"}));
    CHECK_FALSE(abx.is_proper_subset_of(abc));  // Same size, x is missing.
    CHECK_FALSE(BST{"x"}.is_proper_subset_of(abc));
}
