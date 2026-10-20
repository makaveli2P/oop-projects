// trace.h - the steps of one calculator command, for the visualizer.
//
// A Trace is a list of Steps. Each step has one Event (what happened) and a
// Snapshot of both trees at that moment. For a step that frees a node, the
// snapshot is from before the free, so that the visualizer can show the node
// that goes away. Steps that do not change a tree share one snapshot.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "strset/BST.h"
#include "strset/Node.h"

namespace strset {

enum class SetName : std::uint8_t { S1, S2 };

[[nodiscard]] const char* to_string(SetName name) noexcept;

/// One node of a tree shape. `left` and `right` are indexes in Shape::nodes,
/// or -1 for no child.
struct ShapeNode {
    NodeId id = 0;
    std::string value;
    int left = -1;
    int right = -1;
};

/// The shape of a tree: the nodes in preorder, so nodes[0] is the root.
struct Shape {
    std::vector<ShapeNode> nodes;

    [[nodiscard]] bool empty() const noexcept { return nodes.empty(); }
    [[nodiscard]] std::optional<NodeId> root() const;
    /// The index of the node with this id, or -1.
    [[nodiscard]] int index_of(NodeId id) const noexcept;
};

[[nodiscard]] Shape shape_of(const BST& tree);

struct Snapshot {
    Shape s1;
    Shape s2;

    [[nodiscard]] const Shape& of(SetName name) const noexcept { return name == SetName::S1 ? s1 : s2; }
};

/// Why a traversal visits the nodes.
enum class VisitFor : std::uint8_t { List, Union, Intersection, Subset };

namespace event {

/// The state before the command. `command` is the command text, for example
/// "a pear". `text` tells what the command does.
struct Begin {
    std::string command;
    std::string text;
};
/// A search compared `probe` with the value of `node`. `order` < 0: go left,
/// > 0: go right, 0: equal. `diff` is the index of the first character that
/// is different (see first_difference()).
struct Compare {
    SetName set;
    NodeId node;
    std::string probe;
    std::string value;
    int order;
    int diff;
};
/// A new node is in the tree, below `parent` (none: the new root).
struct Link {
    SetName set;
    NodeId node;
    std::optional<NodeId> parent;
    Side side;
    std::string value;
};
/// An insert found the value, so the set does not change.
struct Duplicate {
    SetName set;
    NodeId node;
    std::string value;
};
/// A traversal arrived at a node. `index` counts from 0.
struct Visit {
    SetName set;
    NodeId node;
    std::string value;
    std::size_t index;
    VisitFor reason;
};
/// A lookup found its value.
struct Found {
    SetName set;
    NodeId node;
    std::string value;
};
/// A lookup did not find its value.
struct Missing {
    SetName set;
    std::string value;
};
/// The node is freed next (the snapshot still shows it).
struct Free {
    SetName set;
    NodeId node;
    std::string value;
};
/// A copy made the node `copy` in `set` from `original` of the other set.
struct Copy {
    SetName set;
    NodeId original;
    NodeId copy;
    std::string value;
};
/// The node is removed next. If it has two children, `successor` takes its place.
struct Remove {
    SetName set;
    NodeId node;
    std::string value;
    bool leaf;
    std::optional<NodeId> successor;
    std::string successor_value;
};
/// S1 and S2 swapped their roots. The ids are the roots after the swap.
struct Swap {
    std::optional<NodeId> s1_root;
    std::optional<NodeId> s2_root;
};
/// The command is complete. `summary` is the result sentence.
/// `verdict` is set for the subset command.
struct Done {
    std::string summary;
    std::optional<bool> verdict;
};

}  // namespace event

using Event = std::variant<event::Begin, event::Compare, event::Link, event::Duplicate, event::Visit,
                           event::Found, event::Missing, event::Free, event::Copy, event::Remove,
                           event::Swap, event::Done>;

/// One or two sentences about an event, for example
/// "\"kiwi\" < \"pear\": go left. 'k' < 'p' at index 0." A value that is
/// longer than `max_value` bytes shows as an excerpt with "…". For a
/// comparison, the excerpt keeps the first different character.
[[nodiscard]] std::string describe(const Event& event, std::size_t max_value = 24);

/// `value` in double quotes. If it is longer than `max` bytes, an excerpt
/// around byte `focus` with "…" at the cut ends. A cut never splits a UTF-8
/// character.
[[nodiscard]] std::string quoted(std::string_view value, std::size_t max = 24, std::size_t focus = 0);

/// The set that an event is about, if there is one.
[[nodiscard]] std::optional<SetName> set_of(const Event& event) noexcept;
/// The node that an event is about, if there is one.
[[nodiscard]] std::optional<NodeId> node_of(const Event& event) noexcept;

struct Step {
    Event event;
    std::shared_ptr<const Snapshot> trees;
};

/// The recorder keeps at most this many steps (and then the Done step).
inline constexpr std::size_t kDefaultTraceLimit = 2000;

struct Trace {
    std::vector<Step> steps;
    /// The recorder keeps at most this many steps, and then the Done step.
    std::size_t limit = kDefaultTraceLimit;
    /// True if the command had more steps than the limit. Then the trace has
    /// the first steps and the Done step.
    bool truncated = false;
};

/// Records the steps of the BST operations of a command into a Trace. It
/// knows the two trees, so that it can name the set of each step and make
/// snapshots.
class Recorder final : public BstObserver {
public:
    /// The recorder adds steps to `trace` and respects trace.limit.
    Recorder(const BST& s1, const BST& s2, Trace& trace);

    void begin(std::string command, std::string text);
    /// The reason for the next Visit steps.
    void visit_for(VisitFor reason) noexcept { reason_ = reason; }
    void swapped();
    void done(std::string summary, std::optional<bool> verdict = std::nullopt);

    void compared(const BST& tree, const Node& node, const String& probe, int order) override;
    void linked(const BST& tree, const Node& node, const Node* parent, Side side) override;
    void duplicate(const BST& tree, const Node& node) override;
    void visited(const BST& tree, const Node& node, std::size_t index) override;
    void found(const BST& tree, const Node& node) override;
    void missing(const BST& tree, const String& value) override;
    void freeing(const BST& tree, const Node& node) override;
    void copied(const BST& tree, const Node& original, const Node& copy) override;
    void removing(const BST& tree, const Node& node, const Node* successor) override;

private:
    [[nodiscard]] SetName name_of(const BST& tree) const noexcept;
    /// Adds a step. `changed` is true if a tree changed since the last snapshot.
    void push(Event event, bool changed);

    const BST& s1_;
    const BST& s2_;
    Trace& trace_;
    std::shared_ptr<const Snapshot> snapshot_;
    bool stale_ = true;  ///< A tree will change, or changed, after the last snapshot.
    VisitFor reason_ = VisitFor::List;
};

}  // namespace strset
