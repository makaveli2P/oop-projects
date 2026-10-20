// instruction.hpp - the abstract SAL instruction and one subclass per opcode.
//
// The assembler makes one immutable Instruction object for each source line
// that holds an instruction. The Program owns the objects through
// std::unique_ptr<const Instruction>. Many machines share one Program, so an
// instruction never stores machine state.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "sal/micro_op.hpp"

namespace sal {

class Datapath;

enum class Opcode : std::uint8_t { DEC, LDA, LDB, LDI, STR, XCH, JMP, JZS, JVS, ADD, HLT };

[[nodiscard]] std::string_view to_string(Opcode opcode) noexcept;

/// What the PC does after an instruction executes. The machine applies it.
struct NextPc {
    PcCause cause = PcCause::Increment;
    std::uint8_t target = 0;  ///< Used only for Jump and BranchTaken.
};

class Instruction {
public:
    virtual ~Instruction() = default;
    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    [[nodiscard]] virtual Opcode opcode() const noexcept = 0;
    /// The upper-case mnemonic, for example "LDA".
    [[nodiscard]] std::string_view mnemonic() const noexcept { return to_string(opcode()); }
    /// The operand as text, for example "X" or "-3". Empty if there is none.
    [[nodiscard]] virtual std::string operand() const { return {}; }
    /// One sentence that tells what the instruction does.
    [[nodiscard]] virtual std::string describe() const = 0;
    /// Executes the instruction through the datapath. Returns the PC action.
    [[nodiscard]] virtual NextPc execute(Datapath& dp) const = 0;

protected:
    Instruction() = default;
};

// ---- Instructions with a symbol operand -------------------------------------

/// A base for the instructions that name a data symbol. The assembler
/// resolves the symbol to its data address at load time.
class SymbolInstruction : public Instruction {
public:
    [[nodiscard]] std::string operand() const override { return symbol_; }
    [[nodiscard]] const std::string& symbol() const noexcept { return symbol_; }
    [[nodiscard]] std::uint8_t address() const noexcept { return address_; }

    // The class is abstract, so a public constructor is safe. It must be
    // public because the subclasses inherit it with a using-declaration.
    SymbolInstruction(std::string symbol, std::uint8_t address)
        : symbol_(std::move(symbol)), address_(address) {}

private:
    std::string symbol_;
    std::uint8_t address_;
};

/// DEC symbol: declares a symbol. At run time it changes no data.
class Dec final : public SymbolInstruction {
public:
    using SymbolInstruction::SymbolInstruction;
    Opcode opcode() const noexcept override { return Opcode::DEC; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// LDA symbol: A <- memory[symbol].
class Lda final : public SymbolInstruction {
public:
    using SymbolInstruction::SymbolInstruction;
    Opcode opcode() const noexcept override { return Opcode::LDA; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// LDB symbol: B <- memory[symbol].
class Ldb final : public SymbolInstruction {
public:
    using SymbolInstruction::SymbolInstruction;
    Opcode opcode() const noexcept override { return Opcode::LDB; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// STR symbol: memory[symbol] <- A.
class Str final : public SymbolInstruction {
public:
    using SymbolInstruction::SymbolInstruction;
    Opcode opcode() const noexcept override { return Opcode::STR; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

// ---- LDI ----------------------------------------------------------------------

/// LDI value: A <- value.
class Ldi final : public Instruction {
public:
    explicit Ldi(std::int32_t value) noexcept : value_(value) {}
    Opcode opcode() const noexcept override { return Opcode::LDI; }
    std::string operand() const override { return std::to_string(value_); }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
    [[nodiscard]] std::int32_t value() const noexcept { return value_; }

private:
    std::int32_t value_;
};

// ---- Instructions without an operand ------------------------------------------

/// XCH: swaps A and B.
class Xch final : public Instruction {
public:
    Opcode opcode() const noexcept override { return Opcode::XCH; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// ADD: A <- A + B (32-bit wrap). Sets Z and V.
class Add final : public Instruction {
public:
    Opcode opcode() const noexcept override { return Opcode::ADD; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// HLT: stops the machine. The PC stays at the HLT address.
class Hlt final : public Instruction {
public:
    Opcode opcode() const noexcept override { return Opcode::HLT; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

// ---- Jumps ----------------------------------------------------------------------

/// A base for the instructions that take a code address.
class JumpInstruction : public Instruction {
public:
    [[nodiscard]] std::string operand() const override { return std::to_string(target_); }
    [[nodiscard]] std::uint8_t target() const noexcept { return target_; }

    // Public for the same reason as SymbolInstruction.
    explicit JumpInstruction(std::uint8_t target) noexcept : target_(target) {}

protected:
    /// Reads `flag`, then jumps if it is set. JZS and JVS use this.
    [[nodiscard]] NextPc branch_if(Datapath& dp, Flag flag) const;

private:
    std::uint8_t target_;
};

/// JMP number: PC <- number.
class Jmp final : public JumpInstruction {
public:
    using JumpInstruction::JumpInstruction;
    Opcode opcode() const noexcept override { return Opcode::JMP; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// JZS number: PC <- number if Z is set.
class Jzs final : public JumpInstruction {
public:
    using JumpInstruction::JumpInstruction;
    Opcode opcode() const noexcept override { return Opcode::JZS; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

/// JVS number: PC <- number if V is set.
class Jvs final : public JumpInstruction {
public:
    using JumpInstruction::JumpInstruction;
    Opcode opcode() const noexcept override { return Opcode::JVS; }
    std::string describe() const override;
    NextPc execute(Datapath& dp) const override;
};

}  // namespace sal
