// Tests that render complete frames to text, as "strset --render" does.
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "strset_tui/app.h"
#include "strset_tui/format.h"

namespace {

std::vector<std::string> lines_of(const std::string& frame) {
    std::vector<std::string> lines;
    std::istringstream in{frame};
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

std::string render(std::vector<std::string> setup, const std::string& keys, std::size_t ticks = 0, int width = 120,
                   int height = 36) {
    strset::tui::RenderRequest request;
    request.setup = std::move(setup);
    request.keys = keys;
    request.ticks = ticks;
    request.width = width;
    request.height = height;
    return strset::tui::render_frame(request);
}

/// Every line fits in the width, and the frame has `height` lines.
void require_fits(const std::string& frame, int width, int height) {
    const auto lines = lines_of(frame);
    REQUIRE(lines.size() == static_cast<std::size_t>(height));
    for (const auto& line : lines) {
        INFO(line);
        REQUIRE(strset::tui::display_width(line) <= static_cast<std::size_t>(width));
    }
}

bool contains(const std::string& frame, const std::string& part) { return frame.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("The first frame shows two empty sets and the commands", "[tui][render]") {
    const std::string frame = render({}, "");
    require_fits(frame, 120, 36);
    CHECK(contains(frame, "S1"));
    CHECK(contains(frame, "S2"));
    CHECK(contains(frame, "empty"));
    CHECK(contains(frame, "Ready"));
    CHECK(contains(frame, "a add"));
    CHECK(contains(frame, "q quit"));
    CHECK_FALSE(contains(frame, "\x1b["));  // No escape codes without --color.
}

TEST_CASE("An add frame narrates the comparison and the different character", "[tui][render]") {
    const std::string frame = render({"a mango", "a apple"}, "apear{enter}{space}");
    require_fits(frame, 120, 36);
    INFO(frame);
    CHECK(contains(frame, "\"pear\" > \"mango\": go right."));
    CHECK(contains(frame, "'p' > 'm' at index 0."));
    CHECK(contains(frame, "step 2 of"));
    CHECK(contains(frame, "Paused"));
}

TEST_CASE("The subset verdict is on the last frame", "[tui][render]") {
    const std::string frame = render({"a pear", "a apple", "c", "a fig"}, "bg");
    require_fits(frame, 120, 36);
    INFO(frame);
    CHECK(contains(frame, "Yes: S2 is a proper subset of S1."));
    CHECK(contains(frame, "Done"));
}

TEST_CASE("The prompt row shows the typed text", "[tui][render]") {
    const std::string frame = render({}, "akiwi");
    INFO(frame);
    CHECK(contains(frame, "Add to S1 › kiwi"));
    CHECK(contains(frame, "esc cancel"));
}

TEST_CASE("Help replaces the tree panes", "[tui][render]") {
    const std::string frame = render({}, "?");
    require_fits(frame, 120, 36);
    CHECK(contains(frame, "Commands"));
    CHECK(contains(frame, "Playback"));
}

TEST_CASE("Long chains and long strings fit in small and large terminals", "[tui][render]") {
    std::vector<std::string> setup;
    for (int i = 0; i < 30; ++i) setup.push_back("a item" + std::to_string(100 + i));
    setup.push_back("a " + std::string(200, 'w'));
    for (const auto& [width, height] : std::vector<std::pair<int, int>>{{80, 20}, {100, 30}, {160, 50}}) {
        const std::string frame = render(setup, "c{space}{space}", 0, width, height);
        INFO(frame);
        require_fits(frame, width, height);
    }
}

TEST_CASE("A small terminal asks for a larger one", "[tui][render]") {
    const std::string frame = render({}, "", 0, 60, 15);
    require_fits(frame, 60, 15);
    CHECK(contains(frame, "Please enlarge the terminal."));
}

TEST_CASE("A bad setup command or key script is an error", "[tui][render]") {
    CHECK_THROWS_AS(render({"x"}, ""), std::invalid_argument);
    CHECK_THROWS_AS(render({}, "{nope}"), std::invalid_argument);
}

TEST_CASE("A long path row has no separator after the ellipsis", "[tui][render]") {
    std::vector<std::string> setup;
    for (int i = 10; i < 51; ++i) setup.push_back("a s" + std::to_string(i));
    for (const int width : {80, 100, 120}) {
        const std::string frame = render(setup, "as51{enter}g", 0, width, 22);
        INFO(frame);
        CHECK(contains(frame, "Path: … s"));
        CHECK_FALSE(contains(frame, "…  →"));
        CHECK(contains(frame, "s51 (new)"));
    }
}
