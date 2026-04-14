#include "RNG.h"

namespace mle {
i64 RNG::rngi(i64 max, i64 min) {
    std::uniform_int_distribution<i64> dist(min, max);
    return dist(rng_);
}

u64 RNG::rngu(u64 max, u64 min) {
    std::uniform_int_distribution<u64> dist(min, max);
    return dist(rng_);
}

f32 RNG::rngf(f32 max, f32 min) {
    std::uniform_real_distribution<f32> dist(min, max);
    return dist(rng_);
}

bool RNG::maybe(f32 chance) {
    chance = std::clamp(chance, 0.0F, 1.0F);
    std::bernoulli_distribution dist(chance);
    return dist(rng_);
}
}  // namespace mle
