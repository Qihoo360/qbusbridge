#pragma once

#include <stdint.h>
#include <limits.h>
#include <chrono>

namespace qbus {
namespace pulsar {

class Timer {
   public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<std::chrono::microseconds>;

    void start() noexcept;

    // Returns the millseconds between stop() and last time start() was called.
    // If the result is greater than interval_ms_, call start() immediately.
    int64_t stop() noexcept;

    int64_t intervalMs() const noexcept { return interval_ms_; }
    void setIntervalMs(int64_t interval_ms) noexcept { interval_ms_ = interval_ms; }

   private:
    TimePoint before_;
    int64_t interval_ms_ = INT64_MAX;
};

}  // namespace pulsar
}  // namespace qbus
