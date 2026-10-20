// helpers.hpp - small helpers that the tests share.
#pragma once

#include <catch2/catch_test_macros.hpp>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "sal/sal.hpp"

namespace test {

/// Assembles `source`. The test fails if the source has errors.
inline sal::Machine load(std::string_view source) {
    const sal::AssembleResult result = sal::assemble(source);
    if (!result.ok()) {
        for (const auto& e : result.errors()) {
            UNSCOPED_INFO(sal::to_string(e));
        }
    }
    REQUIRE(result.ok());
    return sal::Machine{result.program()};
}

/// Assembles `source` and returns the errors. The test fails if there are none.
inline std::vector<sal::ParseError> errors_of(std::string_view source) {
    const sal::AssembleResult result = sal::assemble(source);
    REQUIRE_FALSE(result.ok());
    return result.errors();
}

/// The name of the alternative that a micro-operation holds.
inline std::string kind(const sal::MicroOp& op) {
    static const char* const names[] = {"Fetch",   "Decode",    "RegRead", "RegWrite",
                                        "FlagRead", "FlagWrite", "MemRead", "MemWrite",
                                        "AluAdd",  "PcWrite",   "SymbolInfo", "Halt"};
    static_assert(std::size(names) == std::variant_size_v<sal::MicroOp>);
    return names[op.index()];
}

inline std::vector<std::string> kinds(const sal::StepResult& step) {
    std::vector<std::string> result;
    for (const auto& op : step.micro_ops) {
        result.push_back(kind(op));
    }
    return result;
}

/// Executes `count` steps.
inline void steps(sal::Machine& m, int count) {
    for (int i = 0; i < count; ++i) {
        m.step();
    }
}

}  // namespace test

namespace test {

/// Assembles `source` and returns the shared program. The test fails on errors.
inline std::shared_ptr<const sal::Program> program(std::string_view source) {
    const sal::AssembleResult result = sal::assemble(source);
    REQUIRE(result.ok());
    return result.program();
}

/// Assembles a sample program from programs/.
inline std::shared_ptr<const sal::Program> sample(const std::string& name) {
    const sal::AssembleResult result = sal::assemble_file(std::string{SAL_PROGRAMS_DIR} + "/" + name);
    REQUIRE(result.ok());
    return result.program();
}

}  // namespace test
