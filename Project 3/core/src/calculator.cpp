#include "strset/calculator.h"

#include <utility>

namespace strset {

namespace {

std::string q(const String& value) { return "\"" + std::string{value.view()} + "\""; }

std::string count(std::size_t n, const char* noun) {
    return std::to_string(n) + " " + noun + (n == 1 ? "" : "s");
}

/// Begins a trace if there is one. The recorder lives in an optional, so
/// that a command without a trace makes no recorder.
class Recording {
public:
    Recording(const BST& s1, const BST& s2, Trace* trace) {
        if (trace != nullptr) {
            recorder_.emplace(s1, s2, *trace);
        }
    }
    [[nodiscard]] BstObserver* observer() noexcept { return recorder_ ? &*recorder_ : nullptr; }
    void begin(std::string command, std::string text) {
        if (recorder_) recorder_->begin(std::move(command), std::move(text));
    }
    void visit_for(VisitFor reason) noexcept {
        if (recorder_) recorder_->visit_for(reason);
    }
    void swapped() {
        if (recorder_) recorder_->swapped();
    }
    Outcome done(Outcome outcome) {
        if (recorder_) recorder_->done(outcome.message, outcome.verdict);
        return outcome;
    }

private:
    std::optional<Recorder> recorder_;
};

}  // namespace

Outcome SetCalculator::erase(Trace* trace) {
    Recording rec{s1_, s2_, trace};
    if (s1_.empty()) {
        rec.begin("e", "Erase S1. S1 is already empty.");
        return rec.done({"S1 is already empty.", std::nullopt, false});
    }
    rec.begin("e", "Erase S1: delete its nodes one by one, leaves first.");
    const std::size_t freed = s1_.size();
    s1_.clear(rec.observer());
    return rec.done({"Erased S1: freed " + count(freed, "node") + ".", std::nullopt, true});
}

Outcome SetCalculator::switch_sets(Trace* trace) {
    Recording rec{s1_, s2_, trace};
    rec.begin("s", "Switch S1 and S2: swap the two root pointers.");
    s1_.swap(s2_);
    rec.swapped();
    return rec.done({"Switched S1 and S2.", std::nullopt, true});
}

Outcome SetCalculator::subset(Trace* trace) const {
    Recording rec{s1_, s2_, trace};
    rec.begin("b", "Is S2 a proper subset of S1? Search S1 for each string of S2.");
    rec.visit_for(VisitFor::Subset);
    const bool proper = s2_.is_proper_subset_of(s1_, rec.observer());
    if (proper) {
        return rec.done({"Yes: S2 is a proper subset of S1.", true, false});
    }
    // Find the reason: the first string of S2 that S1 does not have, or
    // else the two sets are equal.
    for (const Node* node : s2_.nodes_in_order()) {
        if (!s1_.contains(node->value())) {
            return rec.done({"No: S2 is not a proper subset of S1. " + q(node->value()) +
                                 " is in S2 but not in S1.",
                             false, false});
        }
    }
    return rec.done({"No: S2 is not a proper subset of S1. S1 and S2 have the same strings.", false, false});
}

Outcome SetCalculator::copy(Trace* trace) {
    Recording rec{s1_, s2_, trace};
    rec.begin("c", "Copy S1 into S2: delete the nodes of S2, then copy S1 node by node.");
    s2_.assign_copy(s1_, rec.observer());
    return rec.done({"Copied S1 into S2: " + count(s2_.size(), "new node") + ".", std::nullopt, true});
}

Outcome SetCalculator::list(Trace* trace) const {
    Recording rec{s1_, s2_, trace};
    rec.begin("l", "List S1 and S2 in alphabetical order: an in-order walk of each tree.");
    rec.visit_for(VisitFor::List);
    s1_.walk(rec.observer());
    s2_.walk(rec.observer());
    return rec.done({"S1 and S2 in alphabetical order:", std::nullopt, false});
}

Outcome SetCalculator::add(const String& value, Trace* trace) {
    Recording rec{s1_, s2_, trace};
    if (value.empty()) {
        rec.begin("a", "Add a string to S1.");
        return rec.done({"Nothing to add: the string is empty.", std::nullopt, false});
    }
    rec.begin("a " + std::string{value.view()}, "Add " + q(value) + " to S1. The search starts at the root.");
    if (s1_.insert(value, rec.observer())) {
        return rec.done({"Added " + q(value) + " to S1.", std::nullopt, true});
    }
    return rec.done({q(value) + " is already in S1.", std::nullopt, false});
}

Outcome SetCalculator::unite(Trace* trace) {
    Recording rec{s1_, s2_, trace};
    rec.begin("u", "Union: insert each string of S2 into S1.");
    rec.visit_for(VisitFor::Union);
    const std::size_t added = s1_.unite(s2_, rec.observer());
    return rec.done({"S1 = S1 ∪ S2: added " + count(added, "string") + ".", std::nullopt, added > 0});
}

Outcome SetCalculator::intersect(Trace* trace) {
    Recording rec{s1_, s2_, trace};
    rec.begin("i", "Intersection: remove from S1 each string that is not in S2.");
    rec.visit_for(VisitFor::Intersection);
    const std::size_t removed = s1_.intersect(s2_, rec.observer());
    return rec.done({"S1 = S1 ∩ S2: removed " + count(removed, "string") + ".", std::nullopt, removed > 0});
}

Outcome SetCalculator::run(const Command& command, Trace* trace) {
    switch (command.kind) {
        case CommandKind::Erase: return erase(trace);
        case CommandKind::Switch: return switch_sets(trace);
        case CommandKind::Subset: return subset(trace);
        case CommandKind::Copy: return copy(trace);
        case CommandKind::List: return list(trace);
        case CommandKind::Add: return add(String{std::string_view{command.argument}}, trace);
        case CommandKind::Union: return unite(trace);
        case CommandKind::Intersection: return intersect(trace);
        case CommandKind::Quit:
        case CommandKind::Help: break;
    }
    return {};
}

std::string format_set(const BST& set) {
    std::string text = "{";
    bool first = true;
    for (const Node* node : set.nodes_in_order()) {
        if (!first) text += ", ";
        text += q(node->value());
        first = false;
    }
    return text + "}";
}

}  // namespace strset
