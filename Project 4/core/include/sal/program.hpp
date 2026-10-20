// program.hpp - an assembled SAL program: the code lines and the symbol table.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "sal/instruction.hpp"

namespace sal {

/// One instruction in code memory.
struct CodeLine {
    std::unique_ptr<const Instruction> instruction;
    std::string text;  ///< The source text, without the comment and the outer spaces.
    int line = 0;      ///< The 1-based source line number.
};

/// One declared symbol and its data address.
struct Symbol {
    std::string name;
    std::uint8_t address = 0;
};

/// An immutable program. Machines share it through
/// std::shared_ptr<const Program>, so a machine copy is cheap.
///
/// The constructor enforces the invariants that Machine relies on:
/// - 1 to 128 code lines, and each line holds an instruction.
/// - At most 128 symbols. Each name is unique and not empty.
/// - Symbol i is at data address 128 + i.
/// If an invariant is false, the constructor throws std::invalid_argument.
class Program {
public:
    Program(std::vector<CodeLine> code, std::vector<Symbol> symbols);

    /// The number of instructions. It is 1-128.
    [[nodiscard]] std::size_t size() const noexcept { return code_.size(); }
    [[nodiscard]] const std::vector<CodeLine>& code() const noexcept { return code_; }
    [[nodiscard]] const CodeLine& at(std::size_t address) const { return code_.at(address); }

    /// The symbols in declaration order. Symbol i is at address 128 + i.
    [[nodiscard]] const std::vector<Symbol>& symbols() const noexcept { return symbols_; }
    [[nodiscard]] std::optional<std::uint8_t> address_of(std::string_view name) const noexcept;
    /// The symbol at a data address, or nullptr.
    [[nodiscard]] const Symbol* symbol_at(std::uint8_t address) const noexcept;

private:
    std::vector<CodeLine> code_;
    std::vector<Symbol> symbols_;
};

}  // namespace sal
