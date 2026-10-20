#include "strset_tui/controller.h"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <string_view>
#include <type_traits>
#include <utility>

#include "strset_tui/format.h"

namespace strset::tui {

namespace {

using namespace std::chrono_literals;

constexpr std::array<std::chrono::milliseconds, 7> kIntervals{1500ms, 1000ms, 700ms, 450ms, 250ms, 120ms, 50ms};

template <class T>
const T* as(const Step& step) noexcept {
    return std::get_if<T>(&step.event);
}

void mark(TreeMarks& marks, NodeId id, Tone tone, bool bold) { marks.nodes[id] = NodeMark{tone, bold}; }

/// A value in a list of the progress row. A long value is cut with "…".
std::string short_value(const std::string& value) { return truncate(value, 20); }

bool continues_glyph(char ch) noexcept { return (static_cast<unsigned char>(ch) & 0xC0U) == 0x80U; }

/// Adds `value` as tokens: the text before the character at byte `diff`,
/// that character in `tone`, and the text after it. A long value shows as
/// an excerpt around the character. If `diff` is at the end, a muted
/// "(end)" marks the place.
void add_compared(std::vector<Token>& tokens, std::string_view value, std::size_t diff, Tone tone) {
    constexpr std::size_t kBefore = 16;
    constexpr std::size_t kAfter = 8;
    std::size_t first = diff;
    while (first > 0 && first < value.size() && continues_glyph(value[first])) --first;
    std::size_t last = first < value.size() ? first + 1 : value.size();
    while (last < value.size() && continues_glyph(value[last])) ++last;

    std::size_t start = first > kBefore ? first - kBefore : 0;
    while (start > 0 && continues_glyph(value[start])) --start;
    std::size_t end = std::min(value.size(), last + kAfter);
    while (end < value.size() && continues_glyph(value[end])) ++end;

    if (start > 0) tokens.push_back({"…", Tone::Muted});
    tokens.push_back({std::string{value.substr(start, first - start)}, Tone::Plain});
    if (first < value.size()) {
        tokens.push_back({std::string{value.substr(first, last - first)}, tone, true});
        tokens.push_back({std::string{value.substr(last, end - last)}, Tone::Plain});
        if (end < value.size()) tokens.push_back({"…", Tone::Muted});
    } else {
        tokens.push_back({" (end)", Tone::Muted});
    }
}

}  // namespace

Controller::Controller(std::size_t trace_limit) : trace_limit_(trace_limit) {}

// ---- Commands ------------------------------------------------------------------

void Controller::begin_trace(const Command& command) {
    trace_ = Trace{};
    trace_.limit = trace_limit_;
    cursor_ = 0;
    kind_ = command.kind;
    calc_.run(command, &trace_);
    live_ = Snapshot{shape_of(calc_.s1()), shape_of(calc_.s2())};
}

void Controller::start(const Command& command) {
    if (command.kind == CommandKind::Quit || command.kind == CommandKind::Help) {
        return;
    }
    notice_.clear();
    begin_trace(command);
    running_ = trace_.steps.size() > 1;
    if (trace_.truncated) {
        cursor_ = trace_.steps.size() - 1;
        running_ = false;
        notice_ = "This command has too many steps to animate. The screen shows the result.";
    }
}

void Controller::run_quietly(const Command& command) {
    if (command.kind == CommandKind::Quit || command.kind == CommandKind::Help) {
        return;
    }
    calc_.run(command);
    trace_ = Trace{};
    kind_.reset();
    cursor_ = 0;
    running_ = false;
    live_ = Snapshot{shape_of(calc_.s1()), shape_of(calc_.s2())};
}

void Controller::next() {
    running_ = false;
    if (has_trace() && !at_end()) {
        ++cursor_;
    }
}

void Controller::back() {
    running_ = false;
    if (cursor_ > 0) {
        --cursor_;
    }
}

void Controller::to_end() {
    running_ = false;
    if (has_trace()) {
        cursor_ = trace_.steps.size() - 1;
    }
}

void Controller::toggle_run() {
    if (!has_trace() || trace_.steps.size() < 2) {
        return;
    }
    if (running_) {
        running_ = false;
        return;
    }
    if (at_end()) {
        cursor_ = 0;  // Play the command again.
    }
    running_ = true;
}

bool Controller::tick() {
    if (!running_) {
        return false;
    }
    if (!at_end()) {
        ++cursor_;
    }
    if (at_end()) {
        running_ = false;
    }
    return true;
}

void Controller::faster() noexcept {
    if (speed_ + 1 < kIntervals.size()) ++speed_;
}

void Controller::slower() noexcept {
    if (speed_ > 0) --speed_;
}

std::size_t Controller::speed_level_count() noexcept { return kIntervals.size(); }

std::chrono::milliseconds Controller::tick_interval() const noexcept { return kIntervals[speed_]; }

// ---- Prompt --------------------------------------------------------------------

void Controller::open_prompt() {
    prompt_open_ = true;
    prompt_.clear();
    notice_.clear();
}

void Controller::cancel_prompt() {
    prompt_open_ = false;
    prompt_.clear();
}

void Controller::type(const std::string& text) {
    // A space at the start does not count, so that "a pear" typed as one
    // line gives "pear".
    if (prompt_open_ && !(prompt_.empty() && text == " ")) prompt_ += text;
}

void Controller::erase_last() {
    if (!prompt_.empty()) {
        std::size_t end = prompt_.size() - 1;
        while (end > 0 && (static_cast<unsigned char>(prompt_[end]) & 0xC0U) == 0x80U) --end;
        prompt_.erase(end);
    }
}

void Controller::submit_prompt() {
    const std::string value{trim(prompt_)};
    cancel_prompt();
    if (value.empty()) {
        notice_ = "Nothing to add: the string is empty.";
        return;
    }
    start(Command{CommandKind::Add, value});
}

// ---- Observation ---------------------------------------------------------------

const Snapshot& Controller::trees() const noexcept {
    if (const Step* s = step()) {
        return *s->trees;
    }
    return live_;
}

const Step* Controller::step() const noexcept { return has_trace() ? &trace_.steps[cursor_] : nullptr; }

bool Controller::at_end() const noexcept { return !has_trace() || cursor_ + 1 >= trace_.steps.size(); }

std::string Controller::command_text() const {
    if (!has_trace()) {
        return {};
    }
    if (const auto* begin = as<event::Begin>(trace_.steps.front())) {
        return begin->command;
    }
    return {};
}

namespace {

/// The node of an event that must stay visible in `set`, if there is one.
std::optional<NodeId> focus_of(const Event& event, SetName set) {
    return std::visit(
        [set](const auto& e) -> std::optional<NodeId> {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, event::Copy>) {
                return e.set == set ? e.copy : e.original;
            } else if constexpr (std::is_same_v<T, event::Swap>) {
                return set == SetName::S1 ? e.s1_root : e.s2_root;
            } else if constexpr (std::is_same_v<T, event::Compare> || std::is_same_v<T, event::Link> ||
                                 std::is_same_v<T, event::Duplicate> || std::is_same_v<T, event::Found> ||
                                 std::is_same_v<T, event::Free> || std::is_same_v<T, event::Remove> ||
                                 std::is_same_v<T, event::Visit>) {
                if (e.set == set) return e.node;
                return std::nullopt;
            } else {
                return std::nullopt;
            }
        },
        event);
}

/// True if each value of `part` is also a value of `whole`.
bool has_all_values(const Shape& whole, const Shape& part) {
    std::set<std::string_view> values;
    for (const ShapeNode& node : whole.nodes) values.insert(node.value);
    return std::all_of(part.nodes.begin(), part.nodes.end(),
                       [&](const ShapeNode& node) { return values.count(node.value) > 0; });
}

}  // namespace

TreeMarks Controller::marks(SetName set) const {
    TreeMarks marks;
    marks.show_ids = kind_ == CommandKind::Copy || kind_ == CommandKind::Switch;
    const Step* current = step();
    if (current == nullptr) {
        return marks;
    }

    // The search path: the comparisons in this set just before this step.
    for (std::size_t i = cursor_; i > 0; --i) {
        const auto* compare = as<event::Compare>(trace_.steps[i - 1]);
        if (compare == nullptr || compare->set != set) {
            break;
        }
        mark(marks, compare->node, Tone::Target, false);
        marks.lit_links.insert(compare->node);
    }

    // The node that a traversal visits stays cyan while the lookup in the
    // other set goes on.
    for (std::size_t i = cursor_ + 1; i > 0; --i) {
        if (const auto* visit = as<event::Visit>(trace_.steps[i - 1])) {
            if (visit->set == set && trace_.steps[cursor_].trees->of(set).index_of(visit->node) >= 0) {
                mark(marks, visit->node, Tone::Source, i - 1 == cursor_);
                marks.focus = visit->node;
            }
            break;
        }
    }

    std::visit(
        [&](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, event::Compare>) {
                if (e.set == set) {
                    mark(marks, e.node, Tone::Source, true);
                    marks.lit_links.insert(e.node);
                }
            } else if constexpr (std::is_same_v<T, event::Link>) {
                if (e.set == set) {
                    mark(marks, e.node, Tone::Target, true);
                    marks.lit_links.insert(e.node);
                }
            } else if constexpr (std::is_same_v<T, event::Duplicate>) {
                if (e.set == set) {
                    mark(marks, e.node, Tone::Source, true);
                }
            } else if constexpr (std::is_same_v<T, event::Found>) {
                if (e.set == set) {
                    mark(marks, e.node, Tone::Ok, true);
                }
            } else if constexpr (std::is_same_v<T, event::Free>) {
                if (e.set == set) {
                    mark(marks, e.node, Tone::Alert, true);
                }
            } else if constexpr (std::is_same_v<T, event::Copy>) {
                if (e.set == set) {
                    mark(marks, e.copy, Tone::Target, true);
                    marks.lit_links.insert(e.copy);
                } else {
                    mark(marks, e.original, Tone::Source, true);
                }
            } else if constexpr (std::is_same_v<T, event::Remove>) {
                if (e.set == set) {
                    mark(marks, e.node, Tone::Alert, true);
                    if (e.successor) mark(marks, *e.successor, Tone::Target, true);
                }
            } else if constexpr (std::is_same_v<T, event::Swap>) {
                const std::optional<NodeId> root = set == SetName::S1 ? e.s1_root : e.s2_root;
                if (root) {
                    mark(marks, *root, Tone::Target, true);
                }
            }
        },
        current->event);

    // The node that must stay visible. A step without its own node (for
    // example the Done step) keeps the node of the last earlier step that
    // had one, so the result does not scroll away in the Sideways style.
    const Shape& shown = current->trees->of(set);
    if (const std::optional<NodeId> own = focus_of(current->event, set)) {
        marks.focus = own;
    } else if (!marks.focus) {
        for (std::size_t i = cursor_; i > 0; --i) {
            const std::optional<NodeId> earlier = focus_of(trace_.steps[i - 1].event, set);
            if (earlier && shown.index_of(*earlier) >= 0) {
                marks.focus = earlier;
                break;
            }
        }
    }
    return marks;
}

bool Controller::allow_top_down(SetName set, int width, int height) const {
    const bool ids = kind_ == CommandKind::Copy || kind_ == CommandKind::Switch;
    if (!has_trace()) {
        return fits_top_down(live_.of(set), width, height, ids);
    }
    return fits_top_down(trace_.steps.front().trees->of(set), width, height, ids) &&
           fits_top_down(trace_.steps.back().trees->of(set), width, height, ids);
}

const Shape* Controller::layout_reference(SetName set) const noexcept {
    if (!has_trace()) {
        return nullptr;
    }
    const Shape& first = trace_.steps.front().trees->of(set);
    const Shape& last = trace_.steps.back().trees->of(set);
    const Shape* larger = first.nodes.size() > last.nodes.size() ? &first : &last;
    const Shape* smaller = larger == &first ? &last : &first;
    // A command can free one tree and build a new one (c). The reference is
    // then the snapshot that has every value on the screen: the first
    // snapshot while the old nodes are freed, the last snapshot while the
    // new nodes are built. Thus each node keeps one column in each phase.
    const Shape& shown = trees().of(set);
    if (!has_all_values(*larger, shown) && has_all_values(*smaller, shown)) {
        return smaller;
    }
    return larger;
}

std::vector<Token> Controller::progress() const {
    std::vector<Token> tokens;
    if (!has_trace() || !kind_) {
        return tokens;
    }
    const auto add = [&](std::string text, Tone tone) { tokens.push_back({std::move(text), tone}); };
    const auto separator = [&](bool& first) {
        if (!first) add(", ", Tone::Muted);
        first = false;
    };

    switch (*kind_) {
        case CommandKind::List: {
            std::optional<SetName> group;
            bool first = true;
            for (std::size_t i = 0; i <= cursor_; ++i) {
                const auto* visit = as<event::Visit>(trace_.steps[i]);
                if (visit == nullptr) continue;
                if (group != visit->set) {
                    if (group) add("   ", Tone::Plain);
                    add(std::string{to_string(visit->set)} + ": ", Tone::Muted);
                    group = visit->set;
                    first = true;
                }
                separator(first);
                add(short_value(visit->value), i == cursor_ ? Tone::Source : Tone::Plain);
            }
            break;
        }
        case CommandKind::Subset:
        case CommandKind::Intersection: {
            bool first = true;
            const bool subset = *kind_ == CommandKind::Subset;
            for (std::size_t i = 0; i <= cursor_; ++i) {
                const Step& s = trace_.steps[i];
                if (const auto* found = as<event::Found>(s)) {
                    if (tokens.empty()) add(subset ? "In S1: " : "Checked: ", Tone::Muted);
                    separator(first);
                    add(short_value(found->value) + " ✓", Tone::Ok);
                } else if (const auto* missing = as<event::Missing>(s)) {
                    if (tokens.empty()) add(subset ? "In S1: " : "Checked: ", Tone::Muted);
                    separator(first);
                    add(short_value(missing->value) + " ✗", Tone::Alert);
                }
            }
            break;
        }
        case CommandKind::Union: {
            bool first = true;
            for (std::size_t i = 0; i <= cursor_; ++i) {
                if (const auto* link = as<event::Link>(trace_.steps[i])) {
                    if (tokens.empty()) add("Added: ", Tone::Muted);
                    separator(first);
                    add(short_value(link->value), Tone::Target);
                }
            }
            break;
        }
        case CommandKind::Erase:
        case CommandKind::Copy: {
            const bool erase = *kind_ == CommandKind::Erase;
            std::size_t freed = 0;
            std::size_t copied = 0;
            for (std::size_t i = 0; i <= cursor_; ++i) {
                if (as<event::Free>(trace_.steps[i]) != nullptr && i < cursor_) ++freed;
                if (as<event::Copy>(trace_.steps[i]) != nullptr) ++copied;
            }
            const Snapshot& before = *trace_.steps.front().trees;
            if (erase) {
                add("Freed " + std::to_string(freed) + " of " + std::to_string(before.s1.nodes.size()) + " nodes",
                    Tone::Muted);
            } else {
                if (!before.s2.empty()) {
                    add("Freed " + std::to_string(freed) + " of " + std::to_string(before.s2.nodes.size()) +
                            " old nodes of S2   ",
                        Tone::Muted);
                }
                add("Copied " + std::to_string(copied) + " of " + std::to_string(before.s1.nodes.size()) + " nodes",
                    Tone::Muted);
            }
            break;
        }
        case CommandKind::Add: {
            // At a comparison: the two strings, with the first different
            // character in color. Else: the search path so far.
            if (const auto* compare = as<event::Compare>(trace_.steps[cursor_])) {
                const auto diff = static_cast<std::size_t>(compare->diff);
                add("Compare: ", Tone::Muted);
                add_compared(tokens, compare->probe, diff, Tone::Target);
                add(compare->order == 0 ? "  =  " : compare->order < 0 ? "  <  " : "  >  ", Tone::Muted);
                add_compared(tokens, compare->value, diff, Tone::Source);
                break;
            }
            for (std::size_t i = 0; i <= cursor_; ++i) {
                const Step& s = trace_.steps[i];
                if (const auto* compare = as<event::Compare>(s)) {
                    add(tokens.empty() ? "Path: " : " → ", Tone::Muted);
                    add(short_value(compare->value), Tone::Plain);
                } else if (const auto* link = as<event::Link>(s)) {
                    add(tokens.empty() ? "Path: " : " → ", Tone::Muted);
                    add(short_value(link->value) + " (new)", Tone::Target);
                } else if (as<event::Duplicate>(s) != nullptr) {
                    add(" (already in S1)", Tone::Muted);
                }
            }
            break;
        }
        case CommandKind::Switch:
        case CommandKind::Quit:
        case CommandKind::Help: break;
    }
    return tokens;
}

}  // namespace strset::tui
