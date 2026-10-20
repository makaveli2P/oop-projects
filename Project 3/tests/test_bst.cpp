// Tests for BST: insert, lookup, remove, the rule of five, deep copies and
// long chains.
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <pthread.h>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "strset/BST.h"

using strset::BST;
using strset::Node;
using strset::String;

static_assert(std::is_nothrow_move_constructible_v<BST>);
static_assert(std::is_nothrow_move_assignable_v<BST>);
static_assert(std::is_nothrow_destructible_v<BST>);
static_assert(!std::is_copy_constructible_v<Node>);

namespace {

std::vector<std::string> values_of(const BST& tree) {
    std::vector<std::string> result;
    for (const String& value : tree.values()) result.emplace_back(value.view());
    return result;
}

/// The node ids in preorder: the identity of the nodes and the shape.
std::vector<strset::NodeId> ids_of(const BST& tree) {
    std::vector<strset::NodeId> ids;
    for (const Node* node : tree.nodes_in_preorder()) ids.push_back(node->id());
    return ids;
}

/// Checks the BST property: each node is greater than its left subtree and
/// less than its right subtree. In order, the values increase strictly.
bool is_search_tree(const BST& tree) {
    const auto values = tree.values();
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (!(values[i - 1] < values[i])) return false;
    }
    return values.size() == tree.size();
}

}  // namespace

TEST_CASE("insert adds new values and keeps the order", "[bst]") {
    BST tree;
    CHECK(tree.empty());
    CHECK(tree.insert("pear"));
    CHECK(tree.insert("apple"));
    CHECK(tree.insert("plum"));
    CHECK(tree.insert("banana"));
    CHECK_FALSE(tree.insert("pear"));  // A set has no duplicates.
    CHECK(tree.size() == 4);
    CHECK(values_of(tree) == std::vector<std::string>{"apple", "banana", "pear", "plum"});
    CHECK(tree.root()->value() == String{"pear"});
    CHECK(tree.root()->left()->value() == String{"apple"});
    CHECK(tree.root()->left()->right()->value() == String{"banana"});
    CHECK(tree.height() == 3);
    CHECK(tree.contains("banana"));
    CHECK_FALSE(tree.contains("cherry"));
}

TEST_CASE("remove handles a leaf, one child and two children", "[bst][remove]") {
    //        m
    //      /   \
    //     d     t
    //    / \   / \
    //   a   f p   z
    //        \
    //         g
    BST tree{"m", "d", "t", "a", "f", "p", "z", "g"};
    const Node* f = tree.root()->left()->right();

    SECTION("a leaf") {
        CHECK(tree.remove("a"));
        CHECK(tree.root()->left()->left() == nullptr);
    }
    SECTION("one child: the child takes the place") {
        CHECK(tree.remove("f"));
        CHECK(tree.root()->left()->right()->value() == String{"g"});
    }
    SECTION("two children: the successor node is relinked, not copied") {
        const Node* p = tree.root()->right()->left();
        CHECK(tree.remove("m"));
        CHECK(tree.root() == p);  // The same node object, now the root.
        CHECK(tree.root()->left()->right() == f);
        CHECK(tree.root()->right()->left() == nullptr);
    }
    SECTION("two children, the successor is the right child") {
        tree.remove("p");  // Now t has only z on the right.
        const Node* t = tree.root()->right();
        const Node* z = t->right();
        CHECK(tree.remove("t"));
        CHECK(tree.root()->right() == z);
    }
    SECTION("a value that is not in the set") {
        CHECK_FALSE(tree.remove("q"));
        CHECK(tree.size() == 8);
    }
    CHECK(is_search_tree(tree));
}

TEST_CASE("Removing all values in any order empties the tree", "[bst][remove]") {
    const std::vector<std::string> words{"m", "d", "t", "a", "f", "p", "z", "g", "b", "y"};
    BST tree;
    for (const auto& w : words) tree.insert(String{std::string_view{w}});
    for (const auto& w : {"m", "a", "z", "d", "y", "t", "b", "g", "p", "f"}) {
        CHECK(tree.remove(w));
        CHECK(is_search_tree(tree));
    }
    CHECK(tree.empty());
    CHECK(tree.root() == nullptr);
}

TEST_CASE("The copy constructor makes a deep copy with the same shape", "[bst][copy]") {
    const BST original{"pear", "apple", "plum", "banana"};
    const BST copy{original};
    CHECK(copy == original);
    CHECK(values_of(copy) == values_of(original));
    CHECK(copy.height() == original.height());

    // Each node and each String buffer is a new allocation.
    const auto a = original.nodes_in_preorder();
    const auto b = copy.nodes_in_preorder();
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i] != b[i]);
        CHECK(a[i]->id() != b[i]->id());
        CHECK(a[i]->value() == b[i]->value());
        CHECK(a[i]->value().buffer() != b[i]->value().buffer());
    }
}

TEST_CASE("A copy is independent of the original", "[bst][copy]") {
    BST original{"pear", "apple"};
    BST copy{original};
    copy.insert("kiwi");
    copy.remove("pear");
    original.insert("fig");
    CHECK(values_of(original) == std::vector<std::string>{"apple", "fig", "pear"});
    CHECK(values_of(copy) == std::vector<std::string>{"apple", "kiwi"});
}

TEST_CASE("Copy assignment replaces the content and survives self-assignment", "[bst][copy]") {
    BST a{"x", "y"};
    const BST b{"m", "n", "o"};
    a = b;
    CHECK(a == b);
    CHECK(ids_of(a) != ids_of(b));

    const BST& alias = a;
    const auto before = ids_of(a);
    a = alias;
    CHECK(values_of(a) == std::vector<std::string>{"m", "n", "o"});
    CHECK(ids_of(a) != before);  // Copy and swap: a self-assignment makes a fresh copy.

    a.assign_copy(a);  // assign_copy() ignores itself.
    CHECK(a.size() == 3);
}

TEST_CASE("Moves transfer the nodes without a copy", "[bst][move]") {
    BST a{"pear", "apple"};
    const auto ids = ids_of(a);
    BST b{std::move(a)};
    CHECK(ids_of(b) == ids);
    CHECK(a.empty());  // NOLINT(bugprone-use-after-move): the moved-from state is specified.
    CHECK(a.root() == nullptr);

    BST c{"old"};
    c = std::move(b);
    CHECK(ids_of(c) == ids);
    CHECK(b.empty());  // NOLINT(bugprone-use-after-move)
}

TEST_CASE("swap exchanges only the roots", "[bst]") {
    BST a{"pear", "apple"};
    BST b{"kiwi"};
    const Node* root_a = a.root();
    const Node* root_b = b.root();
    a.swap(b);
    CHECK(a.root() == root_b);
    CHECK(b.root() == root_a);
    CHECK(a.size() == 1);
    CHECK(b.size() == 2);
}

TEST_CASE("No node leaks", "[bst][memory]") {
    const auto before = Node::live_count();
    {
        BST a{"m", "d", "t", "a", "f"};
        BST b{a};
        BST c;
        c = b;
        c.insert("q");
        BST d{std::move(c)};
        d.remove("m");
        a.clear();
        CHECK(Node::live_count() == before + 0 + 5 + 5);  // b, and d (5 - 1 + 1).
        a.unite(d);
        a.intersect(b);
    }
    CHECK(Node::live_count() == before);
}

namespace {

struct ChainResult {
    std::size_t size = 0;
    std::size_t height = 0;
    std::size_t copy_height = 0;
    bool equal = false;
    bool subset = false;
    std::size_t kept = 0;
    std::int64_t live = 0;
};

/// Builds a chain and uses each operation on it. It runs on a thread with
/// a small stack, so a recursive algorithm would crash.
void* chain_work(void* arg) {
    auto& result = *static_cast<ChainResult*>(arg);
    constexpr int kCount = 12000;
    {
        BST chain;
        for (int i = 0; i < kCount; ++i) {
            std::string word = std::to_string(i);
            word.insert(0, 6 - word.size(), '0');
            chain.insert(String{std::string_view{word}});  // Sorted: each node has only a right child.
        }
        result.size = chain.size();
        result.height = chain.height();
        const BST copy{chain};
        result.copy_height = copy.height();
        result.equal = copy == chain;
        BST other;
        other.insert("003000");
        result.subset = other.is_proper_subset_of(chain);
        BST shared{copy};
        shared.intersect(other);
        result.kept = shared.size();
        BST moved = std::move(shared);
        chain.unite(moved);
        result.live = Node::live_count();
    }  // The destructors free 2 * kCount + 2 nodes.
    return nullptr;
}

}  // namespace

TEST_CASE("A long chain does not overflow a small stack", "[bst][chain]") {
    const auto before = Node::live_count();
    ChainResult result;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // 256 KiB: a recursive destructor or copy of a 12000-node chain needs more.
    pthread_attr_setstacksize(&attr, 256 * 1024);
    pthread_t thread;
    REQUIRE(pthread_create(&thread, &attr, chain_work, &result) == 0);
    pthread_join(thread, nullptr);
    pthread_attr_destroy(&attr);

    CHECK(result.size == 12000);
    CHECK(result.height == 12000);
    CHECK(result.copy_height == 12000);
    CHECK(result.equal);
    CHECK(result.subset);
    CHECK(result.kept == 1);
    CHECK(result.live == before + 2 * 12000 + 2);
    CHECK(Node::live_count() == before);
}

TEST_CASE("Two sets are equal if they have the same values", "[bst]") {
    const BST a{"b", "a", "c"};
    const BST b{"a", "b", "c"};  // A different shape.
    CHECK(a == b);
    CHECK(a != BST{"a", "b"});
    CHECK(BST{} == BST{});
}
