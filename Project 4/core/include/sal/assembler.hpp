// assembler.hpp - the load step: source text -> Program, or a list of errors.
//
// The assembler never throws and never crashes on bad input. It reads the
// complete source and reports every error that it finds.
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "sal/program.hpp"

namespace sal {

/// One load-time error. `line` and `column` are 1-based. `column` is 0 when
/// the error is about the line as a whole (or about the file).
struct ParseError {
    int line = 0;
    int column = 0;
    std::string message;
};

/// "line 3, column 5: unknown opcode 'FOO'".
[[nodiscard]] std::string to_string(const ParseError& error);

/// The result of the load step: a program or a non-empty list of errors.
/// It works like a small std::expected (C++23) for C++17.
class AssembleResult {
public:
    explicit AssembleResult(std::shared_ptr<const Program> program) : value_(std::move(program)) {}
    explicit AssembleResult(std::vector<ParseError> errors) : value_(std::move(errors)) {}

    [[nodiscard]] bool ok() const noexcept { return value_.index() == 0; }
    explicit operator bool() const noexcept { return ok(); }

    /// Precondition: ok().
    [[nodiscard]] const std::shared_ptr<const Program>& program() const { return std::get<0>(value_); }
    /// Precondition: !ok().
    [[nodiscard]] const std::vector<ParseError>& errors() const { return std::get<1>(value_); }

private:
    std::variant<std::shared_ptr<const Program>, std::vector<ParseError>> value_;
};

/// Assembles SAL source text. The rules:
/// - One instruction per line. Blank lines and comments (';' or '#' to the
///   end of the line) take no address.
/// - Opcodes are case-insensitive. Symbols are case-sensitive and alphabetic.
/// - CRLF line ends and trailing spaces are accepted.
/// - DEC gives data addresses 128, 129, ... in declaration order.
/// - A symbol operand must be declared somewhere in the program.
/// - An LDI value must fit in int32. A jump target must be an instruction address.
/// - A program holds 1-128 instructions and at most 128 symbols.
[[nodiscard]] AssembleResult assemble(std::string_view source);

/// Reads a file and assembles it. A file that cannot be read gives one error.
[[nodiscard]] AssembleResult assemble_file(const std::filesystem::path& path);

}  // namespace sal
