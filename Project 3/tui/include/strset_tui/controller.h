// controller.h - the state and the commands of the visualizer.
//
// The controller has no FTXUI code and no threads. The views read it, and the
// key handler calls its commands. The tests drive it without a terminal.
//
// A command runs at once on the SetCalculator and records a Trace. The
// screen then shows the steps of the trace one at a time. Each step has a
// snapshot of both trees, so to step back is only to show an earlier step.
#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "strset/calculator.h"
#include "strset/command.h"
#include "strset/trace.h"
#include "strset_tui/tree_layout.h"

namespace strset::tui {

/// A piece of text with a tone, for the progress line.
struct Token {
    std::string text;
    Tone tone = Tone::Plain;
    bool bold = false;
};

class Controller {
public:
    explicit Controller(std::size_t trace_limit = kDefaultTraceLimit);

    // ---- Commands ------------------------------------------------------------

    /// Runs a calculator command (not Quit or Help) and starts its animation.
    /// The animation of the previous command jumps to its end first.
    void start(const Command& command);
    /// Runs a command without animation (the --render --setup option).
    void run_quietly(const Command& command);

    /// Shows the next step. Pauses the run.
    void next();
    /// Shows the previous step. Pauses the run.
    void back();
    /// Shows the last step. Pauses the run.
    void to_end();
    /// Starts or pauses the animated run. At the end, a run starts again
    /// from the first step.
    void toggle_run();
    void pause() noexcept { running_ = false; }
    [[nodiscard]] bool running() const noexcept { return running_; }
    /// One animation tick: shows the next step. Returns false if the run is paused.
    bool tick();

    void faster() noexcept;
    void slower() noexcept;
    [[nodiscard]] std::size_t speed_level() const noexcept { return speed_; }
    [[nodiscard]] static std::size_t speed_level_count() noexcept;
    [[nodiscard]] std::chrono::milliseconds tick_interval() const noexcept;

    // ---- The add prompt and the help -------------------------------------------

    void open_prompt();
    void cancel_prompt();
    /// Adds UTF-8 text to the prompt.
    void type(const std::string& text);
    void erase_last();
    /// Runs "a <text>". An empty text closes the prompt with a message.
    void submit_prompt();
    [[nodiscard]] bool prompt_open() const noexcept { return prompt_open_; }
    [[nodiscard]] const std::string& prompt_text() const noexcept { return prompt_; }

    void toggle_help() noexcept { help_open_ = !help_open_; }
    void close_help() noexcept { help_open_ = false; }
    [[nodiscard]] bool help_open() const noexcept { return help_open_; }

    /// A short message that is not a step, for example "Nothing to add".
    void set_notice(std::string text) { notice_ = std::move(text); }
    [[nodiscard]] const std::string& notice() const noexcept { return notice_; }

    // ---- Observation ---------------------------------------------------------

    [[nodiscard]] const SetCalculator& calculator() const noexcept { return calc_; }
    /// The trees on the screen: the snapshot of the current step.
    [[nodiscard]] const Snapshot& trees() const noexcept;
    [[nodiscard]] bool has_trace() const noexcept { return !trace_.steps.empty(); }
    [[nodiscard]] const Trace& trace() const noexcept { return trace_; }
    /// The step on the screen, or nullptr before the first command.
    [[nodiscard]] const Step* step() const noexcept;
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }
    [[nodiscard]] bool at_end() const noexcept;
    /// The command that the trace shows, for example "a pear".
    [[nodiscard]] std::string command_text() const;
    [[nodiscard]] std::optional<CommandKind> command_kind() const noexcept { return kind_; }

    /// How to draw each tree for the current step.
    [[nodiscard]] TreeMarks marks(SetName set) const;
    /// True if a top-down style fits all the trees of this trace. Thus the
    /// style does not change in the middle of an animation.
    [[nodiscard]] bool allow_top_down(SetName set, int width, int height) const;
    /// The tree of this set at the first or the last step, for a stable
    /// layout: the larger one, or the one that has each value on the screen
    /// if only that one has them (for example while c builds the new S2).
    /// nullptr if there is no trace.
    [[nodiscard]] const Shape* layout_reference(SetName set) const noexcept;
    /// The result so far: the listed strings, the subset checks, the added
    /// or the removed strings.
    [[nodiscard]] std::vector<Token> progress() const;

private:
    void begin_trace(const Command& command);
    void load_snapshot();

    SetCalculator calc_;
    std::size_t trace_limit_;
    Trace trace_;
    std::optional<CommandKind> kind_;
    std::size_t cursor_ = 0;
    Snapshot live_;  ///< The trees when there is no trace.

    bool running_ = false;
    std::size_t speed_ = 2;
    bool prompt_open_ = false;
    std::string prompt_;
    bool help_open_ = false;
    std::string notice_;
};

}  // namespace strset::tui
