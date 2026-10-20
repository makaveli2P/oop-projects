// assembler.cpp - the SAL load step.
//
// Pass 1 splits the source into lines and tokens, counts the instructions and
// gives each DEC symbol a data address. Pass 2 makes one Instruction object
// for each instruction line and resolves the symbol operands.
#include "sal/assembler.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <unordered_map>

#include "sal/memory.hpp"

namespace sal {

namespace {

struct Token {
    std::string_view text;
    int column;  // 1-based.
};

struct SourceLine {
    int number;               // 1-based.
    std::string_view text;    // Without the comment and the outer spaces.
    std::vector<Token> tokens;
};

constexpr bool is_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

constexpr bool is_alpha(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

bool is_symbol(std::string_view text) noexcept {
    return !text.empty() && std::all_of(text.begin(), text.end(), is_alpha);
}

bool all_digits(std::string_view text) noexcept {
    return !text.empty() && std::all_of(text.begin(), text.end(), is_digit);
}

std::string upper(std::string_view text) {
    std::string result{text};
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return result;
}

std::string_view trim(std::string_view text) noexcept {
    while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
    while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
    return text;
}

/// Splits one raw line into tokens. A comment starts at ';' or '#'.
SourceLine tokenize(int number, std::string_view raw) {
    const std::size_t comment = raw.find_first_of(";#");
    const std::string_view code = raw.substr(0, comment);

    SourceLine line{number, trim(code), {}};
    std::size_t i = 0;
    while (i < code.size()) {
        while (i < code.size() && is_space(code[i])) ++i;
        const std::size_t start = i;
        while (i < code.size() && !is_space(code[i])) ++i;
        if (i > start) {
            line.tokens.push_back(Token{code.substr(start, i - start), static_cast<int>(start) + 1});
        }
    }
    return line;
}

std::optional<Opcode> parse_opcode(std::string_view text) {
    static const std::unordered_map<std::string, Opcode> table{
        {"DEC", Opcode::DEC}, {"LDA", Opcode::LDA}, {"LDB", Opcode::LDB}, {"LDI", Opcode::LDI},
        {"STR", Opcode::STR}, {"XCH", Opcode::XCH}, {"JMP", Opcode::JMP}, {"JZS", Opcode::JZS},
        {"JVS", Opcode::JVS}, {"ADD", Opcode::ADD}, {"HLT", Opcode::HLT},
    };
    const auto it = table.find(upper(text));
    if (it == table.end()) {
        return std::nullopt;
    }
    return it->second;
}

constexpr std::size_t operand_count(Opcode opcode) noexcept {
    switch (opcode) {
        case Opcode::XCH:
        case Opcode::ADD:
        case Opcode::HLT: return 0;
        default: return 1;
    }
}

/// Parses an unsigned decimal number. Returns nothing on overflow.
std::optional<std::uint64_t> parse_unsigned(std::string_view digits) noexcept {
    std::uint64_t value = 0;
    const auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (ec != std::errc{} || end != digits.data() + digits.size()) {
        return std::nullopt;
    }
    return value;
}

/// Parses an optional sign and decimal digits into an int32.
std::optional<std::int32_t> parse_int32(std::string_view text) noexcept {
    bool negative = false;
    if (!text.empty() && (text.front() == '+' || text.front() == '-')) {
        negative = text.front() == '-';
        text.remove_prefix(1);
    }
    if (!all_digits(text)) {
        return std::nullopt;
    }
    const auto magnitude = parse_unsigned(text);
    constexpr auto max = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
    if (!magnitude || *magnitude > max + (negative ? 1U : 0U)) {
        return std::nullopt;
    }
    const auto wide = static_cast<std::int64_t>(*magnitude);
    return static_cast<std::int32_t>(negative ? -wide : wide);
}

class Assembler {
public:
    explicit Assembler(std::string_view source) : source_(source) {}

    AssembleResult run() {
        split_lines();
        allocate_symbols();
        std::vector<CodeLine> code;
        for (const SourceLine& line : lines_) {
            if (auto instruction = build(line)) {
                code.push_back(CodeLine{std::move(instruction), std::string{line.text}, line.number});
            }
        }
        if (lines_.empty()) {
            errors_.push_back({0, 0, "the program contains no instructions"});
        }
        if (!errors_.empty()) {
            std::stable_sort(errors_.begin(), errors_.end(), [](const ParseError& l, const ParseError& r) {
                return l.line != r.line ? l.line < r.line : l.column < r.column;
            });
            return AssembleResult{std::move(errors_)};
        }
        return AssembleResult{std::make_shared<const Program>(std::move(code), std::move(symbols_))};
    }

private:
    [[nodiscard]] const Symbol* find_symbol(std::string_view name) const noexcept {
        const auto it = std::find_if(symbols_.begin(), symbols_.end(),
                                     [name](const Symbol& s) { return s.name == name; });
        return it == symbols_.end() ? nullptr : &*it;
    }

    void error(int line, int column, std::string message) {
        errors_.push_back(ParseError{line, column, std::move(message)});
    }

    /// Keeps only the lines that hold an instruction.
    void split_lines() {
        std::string_view rest = source_;
        if (rest.substr(0, 3) == "\xEF\xBB\xBF") {
            rest.remove_prefix(3);  // UTF-8 byte order mark.
        }
        int number = 0;
        while (!rest.empty()) {
            ++number;
            const std::size_t end = rest.find('\n');
            const std::string_view raw = rest.substr(0, end);
            rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end + 1);
            SourceLine line = tokenize(number, raw);
            if (line.tokens.empty()) {
                continue;  // Blank or comment only: no address.
            }
            if (lines_.size() == kCodeSize) {
                error(number, line.tokens.front().column,
                      "the program has more than " + std::to_string(kCodeSize) + " instructions");
            }
            lines_.push_back(std::move(line));
        }
    }

    /// Pass 1 for DEC: data addresses 128, 129, ... in declaration order.
    void allocate_symbols() {
        for (const SourceLine& line : lines_) {
            if (parse_opcode(line.tokens.front().text) != Opcode::DEC || line.tokens.size() != 2) {
                continue;  // Pass 2 reports other problems.
            }
            const Token& name = line.tokens[1];
            if (!is_symbol(name.text)) {
                continue;
            }
            if (find_symbol(name.text) != nullptr) {
                error(line.number, name.column, "duplicate declaration of symbol '" + std::string{name.text} + "'");
                continue;
            }
            if (symbols_.size() == kDataSize) {
                error(line.number, name.column,
                      "the program declares more than " + std::to_string(kDataSize) + " symbols");
                continue;
            }
            const auto address = static_cast<std::uint8_t>(kDataBase + symbols_.size());
            symbols_.push_back(Symbol{std::string{name.text}, address});
        }
    }

    std::optional<std::uint8_t> resolve(const SourceLine& line, const Token& name) {
        if (!is_symbol(name.text)) {
            error(line.number, name.column,
                  "'" + std::string{name.text} + "' is not a valid symbol (use letters only)");
            return std::nullopt;
        }
        const Symbol* symbol = find_symbol(name.text);
        if (symbol == nullptr) {
            error(line.number, name.column, "undeclared symbol '" + std::string{name.text} + "'");
            return std::nullopt;
        }
        return symbol->address;
    }

    std::optional<std::uint8_t> jump_target(const SourceLine& line, const Token& token) {
        const auto value = all_digits(token.text) ? parse_unsigned(token.text) : std::nullopt;
        if (!value || *value >= lines_.size() || *value >= kCodeSize) {
            error(line.number, token.column,
                  "jump target '" + std::string{token.text} + "' is not an instruction address (0-" +
                      std::to_string(std::min(lines_.size(), kCodeSize) - 1) + ")");
            return std::nullopt;
        }
        return static_cast<std::uint8_t>(*value);
    }

    /// Pass 2: makes the Instruction object for one line, or reports errors.
    std::unique_ptr<const Instruction> build(const SourceLine& line) {
        const Token& op_token = line.tokens.front();
        const auto opcode = parse_opcode(op_token.text);
        if (!opcode) {
            error(line.number, op_token.column, "unknown opcode '" + std::string{op_token.text} + "'");
            return nullptr;
        }
        const std::size_t expected = operand_count(*opcode);
        const std::size_t found = line.tokens.size() - 1;
        if (found != expected) {
            const int column = found > expected ? line.tokens[expected + 1].column : op_token.column;
            error(line.number, column,
                  std::string{to_string(*opcode)} + " expects " + std::to_string(expected) +
                      (expected == 1 ? " operand" : " operands") + ", found " + std::to_string(found));
            return nullptr;
        }

        switch (*opcode) {
            case Opcode::XCH: return std::make_unique<const Xch>();
            case Opcode::ADD: return std::make_unique<const Add>();
            case Opcode::HLT: return std::make_unique<const Hlt>();
            case Opcode::LDI: {
                const Token& operand = line.tokens[1];
                const auto value = parse_int32(operand.text);
                if (!value) {
                    error(line.number, operand.column,
                          "'" + std::string{operand.text} + "' is not a 32-bit signed integer");
                    return nullptr;
                }
                return std::make_unique<const Ldi>(*value);
            }
            case Opcode::JMP:
            case Opcode::JZS:
            case Opcode::JVS: {
                const auto target = jump_target(line, line.tokens[1]);
                if (!target) return nullptr;
                if (*opcode == Opcode::JMP) return std::make_unique<const Jmp>(*target);
                if (*opcode == Opcode::JZS) return std::make_unique<const Jzs>(*target);
                return std::make_unique<const Jvs>(*target);
            }
            case Opcode::DEC:
            case Opcode::LDA:
            case Opcode::LDB:
            case Opcode::STR: {
                const Token& name = line.tokens[1];
                if (*opcode == Opcode::DEC && is_symbol(name.text) && find_symbol(name.text) == nullptr) {
                    return nullptr;  // Pass 1 rejected this DEC and reported why.
                }
                const auto address = resolve(line, name);
                if (!address) return nullptr;
                std::string symbol{name.text};
                switch (*opcode) {
                    case Opcode::DEC: return std::make_unique<const Dec>(std::move(symbol), *address);
                    case Opcode::LDA: return std::make_unique<const Lda>(std::move(symbol), *address);
                    case Opcode::LDB: return std::make_unique<const Ldb>(std::move(symbol), *address);
                    default: return std::make_unique<const Str>(std::move(symbol), *address);
                }
            }
        }
        return nullptr;
    }

    std::string_view source_;
    std::vector<SourceLine> lines_;
    std::vector<Symbol> symbols_;
    std::vector<ParseError> errors_;
};

}  // namespace

std::string to_string(const ParseError& error) {
    std::string text;
    if (error.line > 0) {
        text = "line " + std::to_string(error.line);
        if (error.column > 0) {
            text += ", column " + std::to_string(error.column);
        }
        text += ": ";
    }
    return text + error.message;
}

AssembleResult assemble(std::string_view source) {
    try {
        return Assembler{source}.run();
    } catch (const std::exception& e) {  // Only std::bad_alloc is possible.
        return AssembleResult{std::vector<ParseError>{{0, 0, std::string{"internal error: "} + e.what()}}};
    }
}

AssembleResult assemble_file(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        return AssembleResult{std::vector<ParseError>{{0, 0, "cannot open file '" + path.string() + "'"}}};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return assemble(buffer.str());
}

}  // namespace sal
