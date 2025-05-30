/**
 * @file Stopwatch.h
 * @brief Defines the Stopwatch utility for measuring elapsed time.
 */

#pragma once

#include <chrono>

#include "mle/common/math/Types.h"

namespace mle {
/// A high-resolution timer utility class.
class Stopwatch final {
  public:
    Stopwatch();
    ~Stopwatch() = default;

    Stopwatch(const Stopwatch&) = delete;
    Stopwatch& operator=(const Stopwatch&) = delete;
    Stopwatch(Stopwatch&&) = delete;
    Stopwatch& operator=(Stopwatch&&) = delete;

    /// Resets the stopwatch to the current time.
    void reset();

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

    /// Returns the elapsed time in seconds as a float.
    [[nodiscard]] f32 elapsedSecFloat() const;

  private:
    std::chrono::steady_clock::time_point start_;  ///< Start time of the stopwatch.
};

}  // namespace mle
