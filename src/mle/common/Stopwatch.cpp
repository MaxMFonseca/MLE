#include "Stopwatch.h"

namespace mle {
Stopwatch::Stopwatch() {
    reset();
}

void Stopwatch::reset() {
    start_ = std::chrono::steady_clock::now();
}

f32 Stopwatch::elapsedSecFloat() const {
    return static_cast<f32>(elapsed<std::chrono::milliseconds>().count()) / 1000.F;
}
}  // namespace mle
