#include "strset/trace.h"

#include <cstdio>
#include <type_traits>
#include <utility>

namespace strset {

namespace {

template <class... F>
struct Overloaded : F... {
    using F::operator()...;
};
template <class... F>
Overloaded(F...) -> Overloaded<F...>;

std::string text_of(const String& value) { return std::string{value.view()}; }

std::string node_name(NodeId id) { return "node #" + std::to_string(id); }

bool is_continuation(char ch) noexcept { return (static_cast<unsigned char>(ch) & 0xC0U) == 0x80U; }

/// One character for the narration: 'k', or a byte code for a byte that
/// is not printable ASCII.
std::string char_text(std::string_view value, std::size_t index) {
    if (index >= value.size()) {
        return "the end";
    }
    const auto byte = static_cast<unsigned char>(value[index]);
    if (byte >= 0x20 && byte < 0x7F) {
        return std::string{"'"} + static_cast<char>(byte) + "'";
    }
    char code[8];
    std::snprintf(code, sizeof code, "0x%02X", static_cast<unsigned>(byte));
    return std::string{"byte "} + code;
}

std::string describe_compare(const event::Compare& e, std::size_t max) {
    const auto diff = static_cast<std::size_t>(e.diff);
    const std::string probe = quoted(e.probe, max, diff);
    const std::string value = quoted(e.value, max, diff);
    if (e.order == 0) {
        return probe + " = " + value + ": a match.";
    }
    const bool left = e.order < 0;
    std::string text = probe + (left ? " < " : " > ") + value + (left ? ": go left." : ": go right.");
    const std::string_view shorter = e.probe.size() < e.value.size() ? e.probe : e.value;
    if (diff >= shorter.size()) {
        const std::string_view longer = e.probe.size() < e.value.size() ? e.value : e.probe;
        text += " " + quoted(shorter, max, diff) + " is a prefix of " + quoted(longer, max, diff) + ".";
    } else {
        text += " " + char_text(e.probe, diff) + (left ? " < " : " > ") + char_text(e.value, diff) +
                " at index " + std::to_string(diff) + ".";
    }
    return text;
}

std::string ordinal(std::size_t n) {
    const std::size_t mod100 = n % 100;
    const char* suffix = "th";
    if (mod100 < 11 || mod100 > 13) {
        switch (n % 10) {
            case 1: suffix = "st"; break;
            case 2: suffix = "nd"; break;
            case 3: suffix = "rd"; break;
            default: break;
        }
    }
    return std::to_string(n) + suffix;
}

SetName other(SetName name) noexcept { return name == SetName::S1 ? SetName::S2 : SetName::S1; }

}  // namespace

const char* to_string(SetName name) noexcept { return name == SetName::S1 ? "S1" : "S2"; }

std::optional<NodeId> Shape::root() const {
    if (nodes.empty()) {
        return std::nullopt;
    }
    return nodes.front().id;
}

int Shape::index_of(NodeId id) const noexcept {
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Shape shape_of(const BST& tree) {
    Shape shape;
    shape.nodes.reserve(tree.size());
    // Preorder with a stack. Each entry has the node and the index of the
    // parent's link that must point to it.
    struct Job {
        const Node* node;
        int parent;
        bool left;
    };
    std::vector<Job> stack;
    if (tree.root() != nullptr) {
        stack.push_back({tree.root(), -1, false});
    }
    while (!stack.empty()) {
        const Job job = stack.back();
        stack.pop_back();
        const int index = static_cast<int>(shape.nodes.size());
        shape.nodes.push_back({job.node->id(), text_of(job.node->value()), -1, -1});
        if (job.parent >= 0) {
            ShapeNode& parent = shape.nodes[static_cast<std::size_t>(job.parent)];
            (job.left ? parent.left : parent.right) = index;
        }
        if (job.node->right() != nullptr) stack.push_back({job.node->right(), index, false});
        if (job.node->left() != nullptr) stack.push_back({job.node->left(), index, true});
    }
    return shape;
}

std::string quoted(std::string_view value, std::size_t max, std::size_t focus) {
    if (max < 4) {
        max = 4;
    }
    if (value.size() <= max) {
        return "\"" + std::string{value} + "\"";
    }
    // A window of `max` bytes that has `focus` near its end, so that the
    // text before the different character stays visible.
    std::size_t start = 0;
    if (focus + 4 > max) {
        start = focus + 4 - max;
    }
    if (start + max > value.size()) {
        start = value.size() - max;
    }
    std::size_t end = start + max;
    while (start > 0 && is_continuation(value[start])) --start;
    while (end < value.size() && is_continuation(value[end])) ++end;
    std::string text = "\"";
    if (start > 0) text += "…";
    text += value.substr(start, end - start);
    if (end < value.size()) text += "…";
    return text + "\"";
}

std::string describe(const Event& event, std::size_t max) {
    return std::visit(
        Overloaded{
            [](const event::Begin& e) { return e.text; },
            [max](const event::Compare& e) { return describe_compare(e, max); },
            [max](const event::Link& e) {
                std::string text = "New " + node_name(e.node) + " " + quoted(e.value, max);
                if (!e.parent) {
                    return text + " is the root of " + to_string(e.set) + ".";
                }
                return text + " is the " + (e.side == Side::Left ? "left" : "right") + " child of " +
                       node_name(*e.parent) + ".";
            },
            [max](const event::Duplicate& e) {
                return quoted(e.value, max) + " is already in " + to_string(e.set) + ". The set does not change.";
            },
            [max](const event::Visit& e) {
                const std::string value = quoted(e.value, max);
                const char* set = to_string(e.set);
                const char* other_set = to_string(other(e.set));
                switch (e.reason) {
                    case VisitFor::List:
                        return std::string{set} + " in order, " + ordinal(e.index + 1) + ": " + value + ".";
                    case VisitFor::Union:
                        return "Take " + value + " from " + set + " and insert it into " + other_set + ".";
                    case VisitFor::Intersection:
                    case VisitFor::Subset:
                        return "Is " + value + " of " + set + " also in " + other_set + "? Search " + other_set + ".";
                }
                return value;
            },
            [max](const event::Found& e) {
                return quoted(e.value, max) + " is in " + to_string(e.set) + " (" + node_name(e.node) + ").";
            },
            [max](const event::Missing& e) {
                return quoted(e.value, max) + " is not in " + to_string(e.set) + ".";
            },
            [max](const event::Free& e) {
                return "Delete " + node_name(e.node) + " " + quoted(e.value, max) + " of " + to_string(e.set) +
                       ". Its String buffer is freed too.";
            },
            [max](const event::Copy& e) {
                return "New " + node_name(e.copy) + " in " + to_string(e.set) + " copies " + node_name(e.original) +
                       " " + quoted(e.value, max) + ". It has its own String buffer.";
            },
            [max](const event::Remove& e) {
                std::string text = "Remove " + node_name(e.node) + " " + quoted(e.value, max) + " from " +
                                   to_string(e.set) + ".";
                if (e.successor) {
                    return text + " Its successor " + node_name(*e.successor) + " " +
                           quoted(e.successor_value, max) + " takes its place.";
                }
                return text + (e.leaf ? " It is a leaf." : " Its child takes its place.");
            },
            [](const event::Swap&) {
                return std::string{"S1 and S2 swap their root pointers. No node moves."};
            },
            [](const event::Done& e) { return e.summary; },
        },
        event);
}

std::optional<SetName> set_of(const Event& event) noexcept {
    return std::visit(
        [](const auto& e) -> std::optional<SetName> {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, event::Begin> || std::is_same_v<T, event::Swap> ||
                          std::is_same_v<T, event::Done>) {
                return std::nullopt;
            } else {
                return e.set;
            }
        },
        event);
}

std::optional<NodeId> node_of(const Event& event) noexcept {
    return std::visit(
        [](const auto& e) -> std::optional<NodeId> {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, event::Copy>) {
                return e.copy;
            } else if constexpr (std::is_same_v<T, event::Compare> || std::is_same_v<T, event::Link> ||
                                 std::is_same_v<T, event::Duplicate> || std::is_same_v<T, event::Visit> ||
                                 std::is_same_v<T, event::Found> || std::is_same_v<T, event::Free> ||
                                 std::is_same_v<T, event::Remove>) {
                return e.node;
            } else {
                return std::nullopt;
            }
        },
        event);
}

// ---- Recorder -------------------------------------------------------------------

Recorder::Recorder(const BST& s1, const BST& s2, Trace& trace) : s1_(s1), s2_(s2), trace_(trace) {}

SetName Recorder::name_of(const BST& tree) const noexcept { return &tree == &s1_ ? SetName::S1 : SetName::S2; }

void Recorder::push(Event event, bool changed) {
    const bool final_step = std::holds_alternative<event::Done>(event);
    if (changed) {
        stale_ = true;
    }
    if (!final_step && trace_.steps.size() >= trace_.limit) {
        trace_.truncated = true;
        return;
    }
    if (stale_ || !snapshot_) {
        snapshot_ = std::make_shared<const Snapshot>(Snapshot{shape_of(s1_), shape_of(s2_)});
        stale_ = false;
    }
    trace_.steps.push_back({std::move(event), snapshot_});
}

void Recorder::begin(std::string command, std::string text) {
    push(event::Begin{std::move(command), std::move(text)}, true);
}

void Recorder::swapped() {
    const auto root_id = [](const BST& tree) -> std::optional<NodeId> {
        if (tree.root() == nullptr) {
            return std::nullopt;
        }
        return tree.root()->id();
    };
    push(event::Swap{root_id(s1_), root_id(s2_)}, true);
}

void Recorder::done(std::string summary, std::optional<bool> verdict) {
    push(event::Done{std::move(summary), verdict}, false);
}

void Recorder::compared(const BST& tree, const Node& node, const String& probe, int order) {
    push(event::Compare{name_of(tree), node.id(), text_of(probe), text_of(node.value()), order,
                        first_difference(probe, node.value())},
         false);
}

void Recorder::linked(const BST& tree, const Node& node, const Node* parent, Side side) {
    std::optional<NodeId> parent_id;
    if (parent != nullptr) {
        parent_id = parent->id();
    }
    push(event::Link{name_of(tree), node.id(), parent_id, side, text_of(node.value())}, true);
}

void Recorder::duplicate(const BST& tree, const Node& node) {
    push(event::Duplicate{name_of(tree), node.id(), text_of(node.value())}, false);
}

void Recorder::visited(const BST& tree, const Node& node, std::size_t index) {
    push(event::Visit{name_of(tree), node.id(), text_of(node.value()), index, reason_}, false);
}

void Recorder::found(const BST& tree, const Node& node) {
    push(event::Found{name_of(tree), node.id(), text_of(node.value())}, false);
}

void Recorder::missing(const BST& tree, const String& value) {
    push(event::Missing{name_of(tree), text_of(value)}, false);
}

void Recorder::freeing(const BST& tree, const Node& node) {
    push(event::Free{name_of(tree), node.id(), text_of(node.value())}, false);
    stale_ = true;  // The node goes away after this call.
}

void Recorder::copied(const BST& tree, const Node& original, const Node& copy) {
    push(event::Copy{name_of(tree), original.id(), copy.id(), text_of(copy.value())}, true);
}

void Recorder::removing(const BST& tree, const Node& node, const Node* successor) {
    event::Remove e{name_of(tree), node.id(), text_of(node.value()), node.is_leaf(), std::nullopt, {}};
    if (successor != nullptr) {
        e.successor = successor->id();
        e.successor_value = text_of(successor->value());
    }
    push(std::move(e), false);
    stale_ = true;  // The tree changes after this call.
}

}  // namespace strset
