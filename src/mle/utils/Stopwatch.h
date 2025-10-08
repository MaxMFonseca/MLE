/**
 * @file Stopwatch.h
 * @brief Defines the Stopwatch utility for measuring elapsed time.
 */

#pragma once

#include <chrono>

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
/// A high-resolution timer utility class.
class Stopwatch final {
  public:
    Stopwatch() { reset(); }
    ~Stopwatch() = default;

    Stopwatch(const Stopwatch&) = default;
    Stopwatch& operator=(const Stopwatch&) = default;
    Stopwatch(Stopwatch&&) = default;
    Stopwatch& operator=(Stopwatch&&) = default;

    /// Resets the stopwatch to the current time.
    void reset() { start_ = std::chrono::steady_clock::now(); }

    /**
     *  @brief Returns the elapsed time since creation or last reset.
     *
     *  @tparam DurationT The chrono duration type to return (e.g., std::chrono::milliseconds).
     *  @return Elapsed time as a DurationT object.
     */
    template <typename DurationT>
    [[nodiscard]] DurationT elapsed() const {
        return duration_cast<DurationT>(std::chrono::steady_clock::now() - start_);
    }

    [[nodiscard]] int elapsedSecInt() const { return as<int>(elapsed<std::chrono::seconds>().count()); }
    [[nodiscard]] f32 elapsedSecFloat() const { return as<f32>(elapsed<std::chrono::milliseconds>().count()) / 1'000.F; }
    [[nodiscard]] f32 elapsedMSFloat() const { return as<f32>(elapsed<std::chrono::nanoseconds>().count()) / 1'000'000.0F; }

  private:
    std::chrono::steady_clock::time_point start_;  ///< Start time of the stopwatch.
};

}  // namespace mle
