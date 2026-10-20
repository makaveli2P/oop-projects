#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "helpers.hpp"

using Kinds = std::vector<std::string>;

TEST_CASE("Micro-ops: LDA", "[micro_ops]") {
    auto m = test::load("DEC X\nLDI 9\nSTR X\nLDA X\nHLT\n");
    test::steps(m, 3);
    const auto step = m.step();
    REQUIRE(test::kinds(step) == Kinds{"Fetch", "Decode", "MemRead", "RegWrite", "PcWrite"});
    const auto& fetch = std::get<sal::Fetch>(step.micro_ops[0]);
    CHECK(fetch.address == 3);
    CHECK(fetch.text == "LDA X");
    const auto& decode = std::get<sal::Decode>(step.micro_ops[1]);
    CHECK(decode.opcode == "LDA");
    CHECK(decode.operand == "X");
    const auto& read = std::get<sal::MemRead>(step.micro_ops[2]);
    CHECK(read.address == 128);
    CHECK(read.symbol == "X");
    CHECK(read.value == 9);
    const auto& write = std::get<sal::RegWrite>(step.micro_ops[3]);
    CHECK(write.reg == sal::Register::A);
    CHECK(write.old_value == 9);
    CHECK(write.new_value == 9);
    const auto& pc = std::get<sal::PcWrite>(step.micro_ops[4]);
    CHECK(pc.old_value == 3);
    CHECK(pc.new_value == 4);
    CHECK(pc.cause == sal::PcCause::Increment);
}

TEST_CASE("Micro-ops: STR", "[micro_ops]") {
    auto m = test::load("DEC X\nLDI -4\nSTR X\nHLT\n");
    test::steps(m, 2);
    const auto step = m.step();
    REQUIRE(test::kinds(step) == Kinds{"Fetch", "Decode", "RegRead", "MemWrite", "PcWrite"});
    CHECK(std::get<sal::RegRead>(step.micro_ops[2]).value == -4);
    const auto& write = std::get<sal::MemWrite>(step.micro_ops[3]);
    CHECK(write.address == 128);
    CHECK(write.symbol == "X");
    CHECK(write.old_value == 0);
    CHECK(write.new_value == -4);
}

TEST_CASE("Micro-ops: ADD", "[micro_ops]") {
    auto m = test::load("LDI 2147483647\nXCH\nLDI 1\nADD\nHLT\n");
    test::steps(m, 3);
    const auto step = m.step();
    REQUIRE(test::kinds(step) == Kinds{"Fetch", "Decode", "RegRead", "RegRead", "AluAdd",
                                       "RegWrite", "FlagWrite", "FlagWrite", "PcWrite"});
    const auto& alu = std::get<sal::AluAdd>(step.micro_ops[4]);
    CHECK(alu.a == 1);
    CHECK(alu.b == 2147483647);
    CHECK(alu.wide_sum == 2147483648LL);
    CHECK(alu.result == -2147483647 - 1);
    const auto& z = std::get<sal::FlagWrite>(step.micro_ops[6]);
    CHECK(z.flag == sal::Flag::Z);
    CHECK_FALSE(z.new_value);
    const auto& v = std::get<sal::FlagWrite>(step.micro_ops[7]);
    CHECK(v.flag == sal::Flag::V);
    CHECK_FALSE(v.old_value);
    CHECK(v.new_value);
}

TEST_CASE("Micro-ops: JZS taken and not taken", "[micro_ops]") {
    SECTION("not taken") {
        auto m = test::load("JZS 2\nHLT\nHLT\n");
        const auto step = m.step();
        REQUIRE(test::kinds(step) == Kinds{"Fetch", "Decode", "FlagRead", "PcWrite"});
        CHECK_FALSE(std::get<sal::FlagRead>(step.micro_ops[2]).value);
        const auto& pc = std::get<sal::PcWrite>(step.micro_ops[3]);
        CHECK(pc.new_value == 1);
        CHECK(pc.cause == sal::PcCause::BranchNotTaken);
    }
    SECTION("taken") {
        auto m = test::load("ADD\nJZS 3\nHLT\nHLT\n");
        m.step();
        const auto step = m.step();
        REQUIRE(test::kinds(step) == Kinds{"Fetch", "Decode", "FlagRead", "PcWrite"});
        const auto& flag = std::get<sal::FlagRead>(step.micro_ops[2]);
        CHECK(flag.flag == sal::Flag::Z);
        CHECK(flag.value);
        const auto& pc = std::get<sal::PcWrite>(step.micro_ops[3]);
        CHECK(pc.old_value == 1);
        CHECK(pc.new_value == 3);
        CHECK(pc.cause == sal::PcCause::BranchTaken);
    }
}

TEST_CASE("Micro-ops: JMP, XCH, LDI, DEC, HLT", "[micro_ops]") {
    auto m = test::load("DEC X\nLDI 3\nXCH\nJMP 4\nHLT\n");
    const auto dec = m.step();
    REQUIRE(test::kinds(dec) == Kinds{"Fetch", "Decode", "SymbolInfo", "PcWrite"});
    CHECK(std::get<sal::SymbolInfo>(dec.micro_ops[2]).name == "X");
    CHECK(std::get<sal::SymbolInfo>(dec.micro_ops[2]).address == 128);
    CHECK(test::kinds(m.step()) == Kinds{"Fetch", "Decode", "RegWrite", "PcWrite"});
    CHECK(test::kinds(m.step()) ==
          Kinds{"Fetch", "Decode", "RegRead", "RegRead", "RegWrite", "RegWrite", "PcWrite"});
    const auto jmp = m.step();
    REQUIRE(test::kinds(jmp) == Kinds{"Fetch", "Decode", "PcWrite"});
    CHECK(std::get<sal::PcWrite>(jmp.micro_ops[2]).cause == sal::PcCause::Jump);
    const auto hlt = m.step();
    REQUIRE(test::kinds(hlt) == Kinds{"Fetch", "Decode", "PcWrite", "Halt"});
    const auto& pc = std::get<sal::PcWrite>(hlt.micro_ops[2]);
    CHECK(pc.old_value == 4);
    CHECK(pc.new_value == 4);
    CHECK(pc.cause == sal::PcCause::Halt);
    CHECK(std::get<sal::Halt>(hlt.micro_ops[3]).reason == sal::HaltReason::HltExecuted);
}

TEST_CASE("describe() gives one sentence per micro-op", "[micro_ops]") {
    CHECK(sal::describe(sal::Fetch{3, 3, "LDA X"}) == "Fetch: Memory[3] -> IR: \"LDA X\"");
    CHECK(sal::describe(sal::Decode{"LDA", "X"}) == "Decode: opcode LDA, operand X");
    CHECK(sal::describe(sal::Decode{"ADD", ""}) == "Decode: opcode ADD");
    CHECK(sal::describe(sal::RegRead{sal::Register::B, 7}) == "Read B: 7");
    CHECK(sal::describe(sal::RegWrite{sal::Register::A, 1, 2}) == "Write A: 1 -> 2");
    CHECK(sal::describe(sal::FlagRead{sal::Flag::V, true}) == "Test V: 1");
    CHECK(sal::describe(sal::FlagWrite{sal::Flag::Z, false, true}) == "Set Z: 0 -> 1");
    CHECK(sal::describe(sal::MemRead{130, std::string{"X"}, 5}) == "Read Memory[130] (X): 5");
    CHECK(sal::describe(sal::MemWrite{131, std::nullopt, 0, 9}) == "Write Memory[131]: 0 -> 9");
    CHECK(sal::describe(sal::AluAdd{1, 2, 3, 3}) == "ALU: 1 + 2 = 3");
    CHECK(sal::describe(sal::AluAdd{2147483647, 1, 2147483648LL, -2147483647 - 1}) ==
          "ALU: 2147483647 + 1 = 2147483648, wraps to -2147483648");
    CHECK(sal::describe(sal::PcWrite{4, 7, sal::PcCause::BranchTaken}) == "PC: 4 -> 7 (branch taken)");
    CHECK(sal::describe(sal::SymbolInfo{"X", 128}) == "Symbol X is at address 128");
    CHECK(sal::describe(sal::Halt{sal::HaltReason::RanPastEnd}) == "Halt: ran past end of program");
}
