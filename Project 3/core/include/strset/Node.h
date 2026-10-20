// Node.h - one node of a BST: a String and the two child links.
//
// A node owns its children with std::unique_ptr. Only BST changes the links.
// Each node gets a serial number when it is allocated. The visualizer shows
// the serial number, so that you can see that a copy makes new nodes.
#pragma once

#include <cstdint>
#include <memory>

#include "strset/aString.h"

namespace strset {

using NodeId = std::uint64_t;

class Node {
public:
    explicit Node(String value);

    // A node is part of one tree. BST makes deep copies node by node, so a
    // node itself cannot be copied or moved.
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;
    // The BST frees its nodes leaf by leaf (BST::clear), so a node always
    // has no children when it is destroyed, and the destructor does not recurse.
    ~Node() = default;

    [[nodiscard]] const String& value() const noexcept { return value_; }
    [[nodiscard]] NodeId id() const noexcept { return id_; }
    [[nodiscard]] const Node* left() const noexcept { return left_.get(); }
    [[nodiscard]] const Node* right() const noexcept { return right_.get(); }
    [[nodiscard]] bool is_leaf() const noexcept { return !left_ && !right_; }

    /// The number of Node objects that exist now. The tests use it to find leaks.
    [[nodiscard]] static std::int64_t live_count() noexcept;

private:
    friend class BST;

    /// Counts the live nodes. It is a member, so that its constructor and
    /// destructor run for each Node.
    struct Counter {
        Counter() noexcept;
        ~Counter();
        Counter(const Counter&) = delete;
        Counter& operator=(const Counter&) = delete;
        Counter(Counter&&) = delete;
        Counter& operator=(Counter&&) = delete;
    };

    Counter counter_;
    String value_;
    NodeId id_;
    std::unique_ptr<Node> left_;
    std::unique_ptr<Node> right_;
};

}  // namespace strset
