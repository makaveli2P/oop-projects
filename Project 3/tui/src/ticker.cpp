#include "strset_tui/ticker.h"

#include <utility>

namespace strset::tui {

Ticker::Ticker(Callback callback) : callback_(std::move(callback)), thread_([this] { loop(); }) {}

Ticker::~Ticker() { stop(); }

void Ticker::set_active(bool active, std::chrono::milliseconds interval) {
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        if (stopping_) {
            return;  // After stop(), the ticker stays inactive.
        }
        if (active_ == active && interval_ == interval) {
            return;
        }
        active_ = active;
        interval_ = interval;
    }
    wake_.notify_all();
}

bool Ticker::active() const {
    const std::lock_guard<std::mutex> lock{mutex_};
    return active_;
}

void Ticker::stop() {
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        stopping_ = true;
        active_ = false;
    }
    wake_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Ticker::loop() {
    std::unique_lock<std::mutex> lock{mutex_};
    while (!stopping_) {
        if (!active_) {
            wake_.wait(lock, [this] { return stopping_ || active_; });
            continue;
        }
        const auto interval = interval_;
        // Wait one interval. A stop, a pause or a new interval ends the wait early.
        const bool interrupted = wake_.wait_for(lock, interval, [this, interval] {
            return stopping_ || !active_ || interval_ != interval;
        });
        if (interrupted) {
            continue;
        }
        // Call the callback without the lock, so that set_active() never waits
        // for the callback.
        lock.unlock();
        callback_();
        lock.lock();
    }
}

}  // namespace strset::tui
