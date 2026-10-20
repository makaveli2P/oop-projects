// Tests for the animation ticker. Run them with the tsan preset to check
// for data races.
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

#include "strset_tui/ticker.h"

using namespace std::chrono_literals;

namespace {

/// Waits until `condition` is true, for at most two seconds.
template <class F>
bool eventually(F condition) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        if (condition()) return true;
        std::this_thread::sleep_for(1ms);
    }
    return condition();
}

}  // namespace

TEST_CASE("An inactive ticker does not call the callback", "[tui][ticker]") {
    std::atomic<int> calls{0};
    strset::tui::Ticker ticker{[&] { ++calls; }};
    std::this_thread::sleep_for(30ms);
    CHECK(calls == 0);
    CHECK_FALSE(ticker.active());
}

TEST_CASE("An active ticker calls the callback until it is paused", "[tui][ticker]") {
    std::atomic<int> calls{0};
    strset::tui::Ticker ticker{[&] { ++calls; }};
    ticker.set_active(true, 2ms);
    CHECK(ticker.active());
    CHECK(eventually([&] { return calls >= 5; }));

    ticker.set_active(false, 2ms);
    std::this_thread::sleep_for(10ms);  // A call that started before the pause can end.
    const int after_pause = calls;
    std::this_thread::sleep_for(30ms);
    CHECK(calls == after_pause);

    ticker.set_active(true, 1ms);  // Resume at a new interval.
    CHECK(eventually([&] { return calls >= after_pause + 3; }));
}

TEST_CASE("stop joins the thread and ends the calls", "[tui][ticker]") {
    std::atomic<int> calls{0};
    strset::tui::Ticker ticker{[&] { ++calls; }};
    ticker.set_active(true, 1ms);
    CHECK(eventually([&] { return calls >= 2; }));
    ticker.stop();
    const int after_stop = calls;
    std::this_thread::sleep_for(20ms);
    CHECK(calls == after_stop);
    CHECK_FALSE(ticker.active());
    ticker.stop();  // A second stop does nothing.
    ticker.set_active(true, 1ms);  // After stop, nothing restarts the calls.
    std::this_thread::sleep_for(10ms);
    CHECK(calls == after_stop);
    CHECK_FALSE(ticker.active());  // A stopped ticker never reports active.
}

TEST_CASE("The destructor stops an active ticker", "[tui][ticker]") {
    std::atomic<int> calls{0};
    {
        strset::tui::Ticker ticker{[&] { ++calls; }};
        ticker.set_active(true, 1ms);
        CHECK(eventually([&] { return calls >= 1; }));
    }
    const int after = calls;
    std::this_thread::sleep_for(10ms);
    CHECK(calls == after);
}
