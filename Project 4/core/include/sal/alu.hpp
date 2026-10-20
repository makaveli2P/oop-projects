// alu.hpp - the pure arithmetic of the SAL ALU.
#pragma once

#include <cstdint>
#include <limits>

namespace sal {

/// The full output of one 32-bit addition.
struct AddResult {
    std::int64_t wide_sum;  ///< The exact sum. It cannot overflow in 64 bits.
    std::int32_t result;    ///< The two's-complement 32-bit result.
    bool zero;              ///< True when `result` is 0.
    bool overflow;          ///< True when `wide_sum` is outside the int32 range.
};

/// Adds two 32-bit words without undefined behavior.
/// The function computes the sum in 64 bits and wraps it to 32 bits.
[[nodiscard]] constexpr AddResult add32(std::int32_t a, std::int32_t b) noexcept {
    const std::int64_t wide = std::int64_t{a} + std::int64_t{b};
    // Unsigned arithmetic wraps by definition. The conversion back to a
    // signed type is modular in C++20 and on every two's-complement compiler.
    const auto bits = static_cast<std::uint32_t>(static_cast<std::uint64_t>(wide));
    const auto result = static_cast<std::int32_t>(bits);
    const bool overflow = wide < std::numeric_limits<std::int32_t>::min() ||
                          wide > std::numeric_limits<std::int32_t>::max();
    return AddResult{wide, result, result == 0, overflow};
}

}  // namespace sal
