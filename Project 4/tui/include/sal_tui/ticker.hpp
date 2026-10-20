// ticker.hpp - a background thread that calls a function at a fixed interval.
//
// The TUI uses the ticker to animate a run. The callback only posts an event
// to the FTXUI loop. All machine state stays on the UI thread, so the
// ticker and the UI share no data except the ticker's own fields, which a
// mutex protects.
#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sal::tui {

class Ticker {
public:
    using Callback = std::function<void()>;

    /// Starts the thread. The ticker is inactive until set_active(true, ...).
    explicit Ticker(Callback callback);
    /// Stops and joins the thread.
    ~Ticker();

    Ticker(const Ticker&) = delete;
    Ticker& operator=(const Ticker&) = delete;
    Ticker(Ticker&&) = delete;
    Ticker& operator=(Ticker&&) = delete;

    /// While `active` is true, the thread calls the callback one time for each
    /// `interval`. A change of the interval starts a new interval. After
    /// stop(), this function does nothing.
    void set_active(bool active, std::chrono::milliseconds interval);

    /// Stops the thread and waits for it. After stop(), the callback is not
    /// called again. A second call does nothing.
    void stop();

    [[nodiscard]] bool active() const;

private:
    void loop();

    Callback callback_;
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    bool active_ = false;
    bool stopping_ = false;
    std::chrono::milliseconds interval_{200};
    std::thread thread_;  // Declared last: it starts after the other fields exist.
};

}  // namespace sal::tui
