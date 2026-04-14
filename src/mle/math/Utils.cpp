#include "Utils.h"

#include "mle/utils/Utils.h"

namespace mle {
// Rough square root approximation based on IEEE 754 bit hack.
// Inspired by techniques like the Quake III fast inverse square root.
// Public domain origins; adapted variant.
f32 roughSqrt(f32 x) {
    if (x == 0) {
        return 0;
    }
    int i = *rAs<int*>(&x);
    i = (i >> 1) + 0x1fc00000;  // NOLINT hack
    return *rAs<float*>(&i);
}
}  // namespace mle
