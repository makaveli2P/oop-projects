#include "sal/program.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "sal/memory.hpp"

namespace sal {

Program::Program(std::vector<CodeLine> code, std::vector<Symbol> symbols)
    : code_(std::move(code)), symbols_(std::move(symbols)) {
    if (code_.empty() || code_.size() > kCodeSize) {
        throw std::invalid_argument("A program must have 1 to " + std::to_string(kCodeSize) +
                                    " instructions, not " + std::to_string(code_.size()) + ".");
    }
    for (std::size_t address = 0; address < code_.size(); ++address) {
        if (code_[address].instruction == nullptr) {
            throw std::invalid_argument("The code line at address " + std::to_string(address) +
                                        " has no instruction.");
        }
    }
    if (symbols_.size() > kDataSize) {
        throw std::invalid_argument("A program must have at most " + std::to_string(kDataSize) +
                                    " symbols, not " + std::to_string(symbols_.size()) + ".");
    }
    for (std::size_t i = 0; i < symbols_.size(); ++i) {
        const Symbol& symbol = symbols_[i];
        if (symbol.name.empty()) {
            throw std::invalid_argument("Symbol " + std::to_string(i) + " has no name.");
        }
        if (symbol.address != kDataBase + i) {
            throw std::invalid_argument("Symbol " + symbol.name + " must be at address " +
                                        std::to_string(kDataBase + i) + ", not " +
                                        std::to_string(symbol.address) + ".");
        }
        const auto earlier = symbols_.begin() + static_cast<std::ptrdiff_t>(i);
        if (std::any_of(symbols_.begin(), earlier,
                        [&](const Symbol& s) { return s.name == symbol.name; })) {
            throw std::invalid_argument("Symbol " + symbol.name + " has two declarations.");
        }
    }
}

std::optional<std::uint8_t> Program::address_of(std::string_view name) const noexcept {
    const auto it = std::find_if(symbols_.begin(), symbols_.end(),
                                 [name](const Symbol& s) { return s.name == name; });
    if (it == symbols_.end()) {
        return std::nullopt;
    }
    return it->address;
}

const Symbol* Program::symbol_at(std::uint8_t address) const noexcept {
    if (!is_data_address(address)) {
        return nullptr;
    }
    const std::size_t index = address - std::size_t{kDataBase};
    return index < symbols_.size() ? &symbols_[index] : nullptr;
}

}  // namespace sal
