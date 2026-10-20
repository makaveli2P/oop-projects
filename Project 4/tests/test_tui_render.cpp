// Tests that render complete frames to text, as "ali --render" does.
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "helpers.hpp"
#include "sal_tui/app.hpp"
#include "sal_tui/format.hpp"

namespace {

std::string sample_path(const std::string& name) { return std::string{SAL_PROGRAMS_DIR} + "/" + name; }

std::vector<std::string> lines_of(const std::string& frame) {
    std::vector<std::string> lines;
    std::istringstream in{frame};
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

std::string render(const std::string& file, std::size_t steps, std::size_t ops, int width = 120,
                   int height = 36, const std::string& keys = "") {
    sal::tui::RenderRequest request;
    request.file = file;
    request.steps = steps;
    request.ops = ops;
    request.width = width;
    request.height = height;
    request.keys = keys;
    return sal::tui::render_frame(request);
}

/// Every line fits in the width, and the frame has `height` lines.
void require_fits(const std::string& frame, int width, int height) {
    const auto lines = lines_of(frame);
    REQUIRE(lines.size() == static_cast<std::size_t>(height));
    for (const auto& line : lines) {
        INFO(line);
        REQUIRE(sal::tui::display_width(line) <= static_cast<std::size_t>(width));
    }
}

bool contains(const std::string& frame, const std::string& part) { return frame.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("The initial frame shows the three panes", "[tui][render]") {
    const std::string frame = render(sample_path("test1.sal"), 0, 0);
    require_fits(frame, 120, 36);
    CHECK(contains(frame, "Program"));
    CHECK(contains(frame, "Datapath"));
    CHECK(contains(frame, "Memory"));
    CHECK(contains(frame, "DEC INITIAL"));
    CHECK(contains(frame, "Ready"));
    CHECK(contains(frame, "space next"));
    CHECK_FALSE(contains(frame, "\x1b["));  // No escape codes without --color.
}

TEST_CASE("A frame in the middle of an instruction shows the micro-operation", "[tui][render]") {
    const std::string fetch = render(sample_path("test1.sal"), 0, 1);
    CHECK(contains(fetch, "Fetch: Memory[0] -> IR: \"DEC INITIAL\""));
    CHECK(contains(fetch, "step 1  ·  micro-op 1 of 4"));
    // The moving value, next to the bus.
    CHECK((contains(fetch, "DEC INITIAL ▶") || contains(fetch, "DEC INITIAL▶")));

    // The third ADD of test1: the ALU result wraps.
    const std::string add = render(sample_path("test1.sal"), 17, 5);
    CHECK(contains(add, "ALU: 2001000000 + 1000000000 = 3001000000, wraps to -1293967296"));
    CHECK((contains(add, "-1293967296 ▶") || contains(add, "-1293967296▶")));
}

TEST_CASE("Halted frames name the reason", "[tui][render]") {
    CHECK(contains(render(sample_path("test1.sal"), 100, 0), "Halted: HLT executed"));
    CHECK(contains(render(sample_path("swap.sal"), 100, 0), "Halted: ran past end of program"));
}

TEST_CASE("Frames fit at the minimum size and at larger sizes", "[tui][render]") {
    for (const auto& [w, h] : std::vector<std::pair<int, int>>{{100, 30}, {120, 36}, {160, 50}, {220, 60}}) {
        for (std::size_t ops : {0U, 1U, 3U, 7U, 40U}) {
            require_fits(render(sample_path("test3.sal"), 10, ops, w, h), w, h);
            require_fits(render(sample_path("test3.sal"), 10, ops, w, h, "x"), w, h);
        }
    }
}

TEST_CASE("A small terminal shows a short message", "[tui][render]") {
    const std::string frame = render(sample_path("test1.sal"), 0, 0, 80, 24);
    require_fits(frame, 80, 24);
    CHECK(contains(frame, "Please enlarge the terminal."));
    CHECK(contains(frame, "This terminal is 80 × 24."));
}

TEST_CASE("A program with errors shows the error screen", "[tui][render]") {
    const auto path = std::filesystem::temp_directory_path() / "sal_tui_render_errors.sal";
    {
        std::ofstream out{path};
        out << "DEC X\nLDA Y\nFOO 3\n";
    }
    const std::string frame = render(path.string(), 0, 0, 100, 30);
    std::filesystem::remove(path);
    require_fits(frame, 100, 30);
    CHECK(contains(frame, "The program has 2 errors. ALI cannot run it."));
    CHECK(contains(frame, "line 2, column 5"));
    CHECK(contains(frame, "undeclared symbol 'Y'"));
    CHECK(contains(frame, "3 │ FOO 3"));
}

TEST_CASE("Keys in a render request act like key presses", "[tui][render]") {
    // Select address 1, set a breakpoint, run to it.
    const std::string frame = render(sample_path("test1.sal"), 0, 0, 120, 36, "jjjjjjjEg");
    CHECK(contains(frame, "Stopped at the breakpoint at address 7."));
    CHECK(contains(frame, "●"));
}

namespace {

/// Splits a UTF-8 line into code points. Each code point is one terminal cell here.
std::vector<std::string> cells_of(const std::string& line) {
    std::vector<std::string> cells;
    std::size_t i = 0;
    while (i < line.size()) {
        std::size_t length = 1;
        while (i + length < line.size() && (static_cast<unsigned char>(line[i + length]) & 0xC0U) == 0x80U) {
            ++length;
        }
        cells.push_back(line.substr(i, length));
        i += length;
    }
    return cells;
}

bool is_junction(const std::string& cell) {
    for (const char* glyph : {"┬", "┴", "┼", "└", "┘", "┌", "┐", "├", "┤"}) {
        if (cell == glyph) return true;
    }
    return false;
}

/// The row of the PC box top. The diagram moves up or down with the text
/// below it, so the tests find the bus row from this row.
std::size_t pc_row(const std::vector<std::string>& lines) {
    for (std::size_t row = 0; row < lines.size(); ++row) {
        if (lines[row].find("┌ PC") != std::string::npos) return row;
    }
    FAIL("The frame has no PC box.");
    return 0;
}

/// The bus row of a frame, as an offset from the PC box row, and the columns
/// of its junctions. The initial frame has no label on the bus, so all
/// junctions show.
std::pair<std::size_t, std::vector<std::size_t>> bus_junctions(const std::string& frame) {
    const auto lines = lines_of(frame);
    for (std::size_t row = pc_row(lines); row < lines.size(); ++row) {
        const auto cells = cells_of(lines[row]);
        std::vector<std::size_t> columns;
        bool up = false;
        bool down = false;
        for (std::size_t x = 0; x < cells.size(); ++x) {
            if (is_junction(cells[x])) columns.push_back(x);
            up = up || cells[x] == "┴";
            down = down || cells[x] == "┬";
        }
        if (up && down) return {row - pc_row(lines), columns};
    }
    FAIL("The frame has no bus row.");
    return {};
}

}  // namespace

TEST_CASE("The label on the bus never covers a bus junction", "[tui][render]") {
    for (const char* name : {"test1.sal", "test2.sal", "test3.sal", "swap.sal", "overflow.sal"}) {
        for (const auto& [w, h] : std::vector<std::pair<int, int>>{{100, 30}, {120, 36}, {160, 50}}) {
            const auto [bus_offset, columns] = bus_junctions(render(sample_path(name), 0, 0, w, h));
            REQUIRE(columns.size() >= 9);  // The ends and one stub for each unit.
            sal::Machine machine{test::sample(name)};
            for (std::size_t step = 0; step < 40 && !machine.halted(); ++step) {
                const std::size_t ops = machine.step().micro_ops.size();
                for (std::size_t op = 1; op <= ops; ++op) {
                    const auto lines = lines_of(render(sample_path(name), step, op, w, h));
                    const std::size_t bus_row = pc_row(lines) + bus_offset;
                    REQUIRE(bus_row < lines.size());
                    const auto cells = cells_of(lines[bus_row]);
                    for (const std::size_t x : columns) {
                        INFO(name << " " << w << "x" << h << " step " << step << " op " << op << "\n"
                                  << lines[bus_row]);
                        REQUIRE(x < cells.size());
                        REQUIRE(is_junction(cells[x]));
                    }
                }
            }
        }
    }
}

TEST_CASE("DEC marks the symbol, not a written or changed value", "[tui][render]") {
    // test1 step 1, micro-op 3: "Symbol INITIAL is at address 128".
    sal::tui::RenderRequest request;
    request.file = sample_path("test1.sal");
    request.ops = 3;
    const std::string plain = sal::tui::render_frame(request);
    REQUIRE(contains(plain, "Symbol INITIAL is at address 128"));
    CHECK_FALSE(contains(plain, "◂128"));  // The write marker.
    CHECK(contains(plain, "•128 INITIAL"));

    request.color = true;
    const auto lines = lines_of(sal::tui::render_frame(request));
    bool found = false;
    for (const auto& line : lines) {
        const auto at = line.rfind("128");
        if (at == std::string::npos || line.find("INITIAL", at) == std::string::npos) continue;
        found = true;
        // After the name, the value 0 has no amber (target) color.
        const std::string after_name = line.substr(line.find("INITIAL", at) + 7);
        INFO(after_name);
        CHECK(after_name.find("\x1b[33m") == std::string::npos);
    }
    CHECK(found);
}

TEST_CASE("The memory pane shows full symbol names at the minimum width", "[tui][render]") {
    // LDB ADDER at the minimum size: a 10-digit value and a 5-letter name.
    const std::string frame = render(sample_path("test1.sal"), 10, 3, 100, 30);
    require_fits(frame, 100, 30);
    CHECK(contains(frame, "▸129 ADDER"));
    CHECK(contains(frame, " 128 INITIAL"));
    CHECK(contains(frame, "1000000000 │"));  // The value on its own row.
    for (const std::string& hex : {std::string{""}, std::string{"x"}}) {
        const std::string halted = render(sample_path("test3.sal"), 1000, 0, 100, 30, hex);
        require_fits(halted, 100, 30);
        CHECK(contains(halted, "129 RESULT"));
        CHECK(contains(halted, "130 NTHTERM"));
        CHECK(contains(halted, "131 DECREMENTOR"));
    }
}

TEST_CASE("The error screen tells how many errors it does not show", "[tui][render]") {
    const auto path = std::filesystem::temp_directory_path() / "sal_tui_render_many_errors.sal";
    {
        std::ofstream out{path};
        for (int i = 0; i < 15; ++i) out << "FOO " << i << "\n";
    }
    const std::string frame = render(path.string(), 0, 0, 100, 30);
    const std::string big = render(path.string(), 0, 0, 160, 200);
    std::filesystem::remove(path);
    require_fits(frame, 100, 30);
    CHECK(contains(frame, "The program has 15 errors."));
    // Count the errors on the screen, then check the "more" line.
    std::size_t shown = 0;
    for (const auto& line : lines_of(frame)) {
        if (line.find("unknown opcode") != std::string::npos) ++shown;
    }
    REQUIRE(shown > 0);
    REQUIRE(shown < 15);
    CHECK(contains(frame, "... and " + std::to_string(15 - shown) +
                              " more errors. ALI prints all of them when you quit."));
    // When all errors fit, there is no "more" line.
    require_fits(big, 160, 200);
    CHECK_FALSE(contains(big, "more error"));
}
