#include "timer.h"
#include <iostream>

namespace qbus {
namespace pulsar {

void Timer::start() noexcept { before_ = Clock::now(); }

int64_t Timer::stop() noexcept {
    using namespace std::chrono;
    auto now = Clock::now();
    auto result = static_cast<int64_t>(duration_cast<microseconds>(now - before_).count());
    result /= 1000;
    if (result >= interval_ms_) {
        before_ = now;
    }
    return result;
}

}  // namespace pulsar
}  // namespace qbus
