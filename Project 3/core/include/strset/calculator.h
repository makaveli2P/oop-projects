// calculator.h - the set calculator: the two sets S1 and S2 and the commands.
//
// The classic mode and the visualizer both use this class. Each command can
// record a Trace of its steps. Without a trace (nullptr), a command does no
// extra work.
#pragma once

#include <optional>
#include <string>

#include "strset/BST.h"
#include "strset/command.h"
#include "strset/trace.h"

namespace strset {

/// The result of one command.
struct Outcome {
    /// One sentence about the result, for example "Added \"pear\" to S1."
    std::string message;
    /// For the subset command: true if S2 is a proper subset of S1.
    std::optional<bool> verdict;
    /// True if S1 or S2 changed.
    bool changed = false;
};

class SetCalculator {
public:
    [[nodiscard]] const BST& s1() const noexcept { return s1_; }
    [[nodiscard]] const BST& s2() const noexcept { return s2_; }

    /// e: deletes the content of S1.
    Outcome erase(Trace* trace = nullptr);
    /// s: swaps S1 and S2. Only the root pointers change.
    Outcome switch_sets(Trace* trace = nullptr);
    /// b: tells if S2 is a proper subset of S1 (S2 ⊆ S1 and S2 ≠ S1).
    Outcome subset(Trace* trace = nullptr) const;
    /// c: deletes the content of S2 and makes S2 a deep copy of S1.
    Outcome copy(Trace* trace = nullptr);
    /// l: walks S1 and then S2 in alphabetical order.
    Outcome list(Trace* trace = nullptr) const;
    /// a: adds `value` to S1. An empty value is an error, and nothing changes.
    Outcome add(const String& value, Trace* trace = nullptr);
    /// u: S1 = S1 ∪ S2.
    Outcome unite(Trace* trace = nullptr);
    /// i: S1 = S1 ∩ S2.
    Outcome intersect(Trace* trace = nullptr);

    /// Runs one of the commands above. Quit and Help do nothing here.
    Outcome run(const Command& command, Trace* trace = nullptr);

private:
    BST s1_;
    BST s2_;
};

/// "{"apple", "pear"}": the values of a set in alphabetical order.
[[nodiscard]] std::string format_set(const BST& set);

}  // namespace strset
