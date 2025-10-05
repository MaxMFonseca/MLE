#pragma once

#include <mutex>
#include <random>

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class RNG {
  public:
    RNG() = default;
    ~RNG() = default;

    MLE_NO_COPY_MOVE(RNG)

    void reseed(u64 seed) { rng_.seed(seed); }

    i64 rngi(i64 max = mle::max<i64>(), i64 min = mle::min<i64>());
    u64 rngu(u64 max = mle::max<u64>(), u64 min = mle::min<u64>());
    f32 rngf(f32 max = 1.0F, f32 min = 0.F);
    bool maybe(f32 chance = 0.5);

  private:
    std::mt19937_64 rng_{std::random_device{}()};
};

struct ThreadRNG {
    static RNG& i() {
        thread_local RNG instance;
        return instance;
    }
};
}  // namespace mle
