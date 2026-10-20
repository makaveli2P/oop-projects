// BST.h - BST, a set of Strings in an unbalanced binary search tree.
//
// The tree owns its nodes through std::unique_ptr links (see Node.h). The
// copy constructor makes a deep copy: new nodes with new String buffers. The
// destructor frees every node. All the algorithms are iterative, so a
// degenerate tree (a long chain after sorted inserts) cannot overflow the
// call stack.
//
// Each operation can tell a BstObserver about each step: a comparison, a new
// link, a node that it frees, and so on. The visualizer records these steps.
// Without an observer (nullptr), the operations do no extra work.
#pragma once

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <vector>

#include "strset/Node.h"
#include "strset/aString.h"

namespace strset {

class BST;

enum class Side : std::uint8_t { Left, Right };

/// Receives the steps of the BST operations. Each function has an empty
/// default, so that an observer implements only the steps that it needs.
/// The tree in each call is the tree that the step is about.
class BstObserver {
public:
    BstObserver() = default;
    BstObserver(const BstObserver&) = default;
    BstObserver& operator=(const BstObserver&) = default;
    BstObserver(BstObserver&&) = default;
    BstObserver& operator=(BstObserver&&) = default;
    virtual ~BstObserver() = default;

    /// A search compared `probe` with the value of `node`. `order` is
    /// probe.compare(node.value()): < 0 goes left, > 0 goes right, 0 is a match.
    virtual void compared(const BST& /*tree*/, const Node& /*node*/, const String& /*probe*/, int /*order*/) {}
    /// An insert linked the new `node` below `parent` (nullptr: the new root).
    virtual void linked(const BST& /*tree*/, const Node& /*node*/, const Node* /*parent*/, Side /*side*/) {}
    /// An insert found the value at `node`, so the set does not change.
    virtual void duplicate(const BST& /*tree*/, const Node& /*node*/) {}
    /// A traversal arrived at `node`. `index` counts from 0.
    virtual void visited(const BST& /*tree*/, const Node& /*node*/, std::size_t /*index*/) {}
    /// A lookup found its value at `node`.
    virtual void found(const BST& /*tree*/, const Node& /*node*/) {}
    /// A lookup did not find `value`.
    virtual void missing(const BST& /*tree*/, const String& /*value*/) {}
    /// clear() frees `node` next. The node is a leaf. The call comes before the free.
    virtual void freeing(const BST& /*tree*/, const Node& /*node*/) {}
    /// A copy made the new node `copy` in `tree` from `original`.
    virtual void copied(const BST& /*tree*/, const Node& /*original*/, const Node& /*copy*/) {}
    /// remove() frees `node` next. If the node has two children, `successor`
    /// is the node that takes its place. The call comes before the change.
    virtual void removing(const BST& /*tree*/, const Node& /*node*/, const Node* /*successor*/) {}
};

class BST {
public:
    BST() noexcept = default;
    BST(std::initializer_list<String> values);

    /// A deep copy: each node and each String buffer is a new allocation.
    BST(const BST& other);
    BST(BST&& other) noexcept;
    /// Copy and swap: if an allocation throws, *this does not change.
    BST& operator=(const BST& other);
    BST& operator=(BST&& other) noexcept;
    /// Frees all nodes, leaf by leaf, without recursion.
    ~BST();

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] const Node* root() const noexcept { return root_.get(); }
    /// The number of levels: 0 for an empty tree, 1 for a root only.
    [[nodiscard]] std::size_t height() const;

    /// Adds `value` if it is not in the set. Returns true if it added the value.
    bool insert(String value, BstObserver* observer = nullptr);
    /// True if `value` is in the set.
    [[nodiscard]] bool contains(const String& value, BstObserver* observer = nullptr) const;
    /// Removes `value` if it is in the set. Returns true if it removed the value.
    bool remove(const String& value, BstObserver* observer = nullptr);
    /// Removes all values.
    void clear(BstObserver* observer = nullptr);

    /// Makes *this a deep copy of `source`, node by node, in preorder. The old
    /// nodes of *this are freed first. If an allocation throws, *this has a
    /// part of the copy (the basic guarantee). `source` must not be *this.
    void assign_copy(const BST& source, BstObserver* observer = nullptr);

    /// *this = *this ∪ other. Inserts the values of `other` in preorder.
    /// Returns the number of new values.
    std::size_t unite(const BST& other, BstObserver* observer = nullptr);
    /// *this = *this ∩ other. Removes each value that `other` does not have.
    /// Returns the number of removed values.
    std::size_t intersect(const BST& other, BstObserver* observer = nullptr);
    /// True if each value of *this is in `other` and `other` has more values.
    /// It stops at the first value that `other` does not have.
    [[nodiscard]] bool is_proper_subset_of(const BST& other, BstObserver* observer = nullptr) const;

    /// Visits each node in order (alphabetical order).
    void walk(BstObserver* observer) const;
    /// The values in alphabetical order.
    [[nodiscard]] std::vector<String> values() const;
    /// The nodes in order. The pointers are valid until the tree changes.
    [[nodiscard]] std::vector<const Node*> nodes_in_order() const;
    /// The nodes in preorder: each node before its children.
    [[nodiscard]] std::vector<const Node*> nodes_in_preorder() const;

    /// Swaps the roots (and the sizes) of two trees. No node moves.
    void swap(BST& other) noexcept;
    friend void swap(BST& a, BST& b) noexcept { a.swap(b); }

    /// True if the two sets have the same values. The shapes can be different.
    friend bool operator==(const BST& a, const BST& b);
    friend bool operator!=(const BST& a, const BST& b) { return !(a == b); }

private:
    using Link = std::unique_ptr<Node>;

    /// The link that points to `value`, or the empty link where it must go.
    /// `parent` gets the node that holds the link (nullptr for the root).
    Link& find_link(const String& value, const Node*& parent, BstObserver* observer);
    [[nodiscard]] const Node* find(const String& value, BstObserver* observer) const;
    void remove_at(Link& link, BstObserver* observer);

    Link root_;
    std::size_t size_ = 0;
};

}  // namespace strset
