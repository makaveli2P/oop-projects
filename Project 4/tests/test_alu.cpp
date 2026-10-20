#include <catch2/catch_test_macros.hpp>
#include <limits>

#include "sal/alu.hpp"

namespace {
constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();
constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();
}  // namespace

TEST_CASE("add32: normal sums", "[alu]") {
    const auto r = sal::add32(2, 3);
    CHECK(r.result == 5);
    CHECK(r.wide_sum == 5);
    CHECK_FALSE(r.zero);
    CHECK_FALSE(r.overflow);
}

TEST_CASE("add32: INT32_MAX + 1 wraps to INT32_MIN with V", "[alu]") {
    const auto r = sal::add32(kMax, 1);
    CHECK(r.result == kMin);
    CHECK(r.wide_sum == std::int64_t{kMax} + 1);
    CHECK(r.overflow);
    CHECK_FALSE(r.zero);
}

TEST_CASE("add32: INT32_MIN + -1 wraps to INT32_MAX with V", "[alu]") {
    const auto r = sal::add32(kMin, -1);
    CHECK(r.result == kMax);
    CHECK(r.overflow);
    CHECK_FALSE(r.zero);
}

TEST_CASE("add32: INT32_MIN + 0 is not an overflow", "[alu]") {
    const auto r = sal::add32(kMin, 0);
    CHECK(r.result == kMin);
    CHECK_FALSE(r.overflow);
    CHECK_FALSE(r.zero);
}

TEST_CASE("add32: -5 + 5 sets Z only", "[alu]") {
    const auto r = sal::add32(-5, 5);
    CHECK(r.result == 0);
    CHECK(r.zero);
    CHECK_FALSE(r.overflow);
}

TEST_CASE("add32: INT32_MIN + INT32_MIN gives 0 with Z and V", "[alu]") {
    const auto r = sal::add32(kMin, kMin);
    CHECK(r.result == 0);
    CHECK(r.wide_sum == -4294967296LL);
    CHECK(r.zero);
    CHECK(r.overflow);
}

TEST_CASE("add32: INT32_MAX + INT32_MAX wraps to -2", "[alu]") {
    const auto r = sal::add32(kMax, kMax);
    CHECK(r.result == -2);
    CHECK(r.overflow);
}

TEST_CASE("add32 is usable at compile time", "[alu]") {
    static_assert(sal::add32(kMax, 1).result == kMin);
    static_assert(sal::add32(kMin, kMin).zero);
    SUCCEED();
}
