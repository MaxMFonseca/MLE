#pragma once

#include "mle/math/Types.h"
#include "mle/math/Types2D.h"

namespace mle {
struct RectPacker {
    explicit RectPacker(vec2u extent = {}, vec2u padding = {2, 2}, u32 bin_min_size = 16) :
        padding_(padding),
        extent_(extent),
        bin_min_size_(bin_min_size) {
        bins_.emplace_back(Rectu{{0, 0}, extent_});
    }

    // This will pock in order, and if not all fit, will return as many as possible. So it's up to the caller to check if all fit.
    [[nodiscard]] std::vector<vec2u> pack(std::span<const vec2u> rect_sizes);
    [[nodiscard]] std::optional<vec2u> tryPackOne(vec2u packing_size);

  private:
    vec2u padding_{};
    vec2u extent_{};
    u32 bin_min_size_;
    std::vector<Rectu> bins_;
};
}  // namespace mle
