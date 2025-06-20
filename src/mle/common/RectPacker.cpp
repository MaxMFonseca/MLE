#include "RectPacker.h"

#include <algorithm>
#include <numeric>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Utils.h"

namespace mle {
void RectPacker::sort() {
    clear();
    std::ranges::sort(rects_, [](const Rect& a, const Rect& b) { return a.size.x * a.size.y > b.size.x * b.size.y; });
}

void RectPacker::clear() {
    packed_positions_.clear();
    bins_.clear();
    current_extent_ = {};
}

namespace {
inline bool aFitsIn(vec2u a, vec2u in) {
    return a.x <= in.x && a.y <= in.y;
}

inline u32 calcArea(const std::vector<RectPacker::Rect>& rects) {
    return std::accumulate(rects.begin(), rects.end(), 0U, [](u32 acc, const RectPacker::Rect& r) { return acc + (r.size.x * r.size.y); });  // NOLINT
}
}  // namespace

bool RectPacker::pack(vec2u ov_extent) {
    if (ov_extent.x == 0 || ov_extent.y == 0) {
        ov_extent = max_extent;
    }

    if (bins_.empty()) {
        bins_.emplace_back(Rectu{{0, 0}, ov_extent});
    }

    bins_.reserve(rects_.size() + 3);

    bool all_fit = true;
    packed_positions_.reserve(rects_.size());
    for (usize i_rect = packed_positions_.size(); i_rect < rects_.size(); i_rect++) {
        auto rect_size = rects_.at(i_rect).size + padding;

        for (int i_bin = as<int>(bins_.size() - 1); i_bin >= 0; i_bin--) {
            auto bin = bins_.at(i_bin);
            if (aFitsIn(rect_size, bin.size)) {
                packed_positions_.emplace_back(bin.pos);

                current_extent_.x = std::max(current_extent_.x, bin.pos.x + rect_size.x - padding.x);
                current_extent_.y = std::max(current_extent_.y, bin.pos.y + rect_size.y - padding.y);

                Rectu big_remainder;
                Rectu small_remainder;

                if (bin.size.x - rect_size.x > bin.size.y - rect_size.y) {
                    big_remainder.pos = {bin.pos.x + rect_size.x, bin.pos.y};
                    big_remainder.size = {bin.size.x - rect_size.x, bin.size.y};
                    small_remainder.pos = {bin.pos.x, bin.pos.y + rect_size.y};
                    small_remainder.size = {rect_size.x, bin.size.y - rect_size.y};
                } else {
                    big_remainder.pos = {bin.pos.x, bin.pos.y + rect_size.y};
                    big_remainder.size = {bin.size.x, bin.size.y - rect_size.y};
                    small_remainder.pos = {bin.pos.x + rect_size.x, bin.pos.y};
                    small_remainder.size = {bin.size.x - rect_size.x, rect_size.y};
                }

                MLE_ASSERT(big_remainder.size.x + big_remainder.pos.x <= ov_extent.x);
                MLE_ASSERT(big_remainder.size.y + big_remainder.pos.y <= ov_extent.y);

                bins_.erase(bins_.begin() + as<i64>(i_bin));

                auto insert_sorted = [this](const Rectu& rem) {
                    if (rem.size.x < padding.x || rem.size.y < padding.y) {
                        return;
                    }
                    const u32 area = rem.size.x * rem.size.y;
                    auto it = std::ranges::lower_bound(bins_, area, std::greater{}, [](const Rectu& bin) { return bin.size.x * bin.size.y; });
                    bins_.insert(it, rem);
                };

                insert_sorted(big_remainder);
                insert_sorted(small_remainder);

                break;
            }
            if (i_bin == 0) {
                all_fit = false;
                packed_positions_.emplace_back(max<u32>());
            }
        }
    }

    return all_fit;
}

bool RectPacker::tryPackOptimal(f32 scale) {
    bool all_fit = false;

    vec2u try_extent{as<u32>(roughSqrt(as<f32>(calcArea(rects_))))};

    u32 max_iter = 10;
    while (!all_fit && (try_extent.x <= max_extent.x && try_extent.y <= max_extent.y)) {
        if (max_iter == 0) {
            MLE_W("Unable to pack optimal for scale {}. max iter", scale);
            return false;
        }
        max_iter--;

        try_extent.x = as<u32>(std::min(as<f32>(try_extent.x) * scale, as<f32>(max_extent.x)));
        try_extent.y = as<u32>(std::min(as<f32>(try_extent.y) * scale, as<f32>(max_extent.y)));

        clear();

        all_fit = pack(try_extent);
    }

    return all_fit;
}

Rectu RectPacker::getIdx(usize i) const {
    if (packed_positions_.at(i).x == max<u32>()) {
        return {0, 0, 0, 0};
    }
    return {packed_positions_.at(i), rects_.at(i).size};
}

Rectu RectPacker::getID(u32 id) const {
    auto it = std::ranges::find_if(rects_, [id](const Rect& r) { return r.id == id; });
    if (it == rects_.end()) {
        return {0, 0, 0, 0};
    }
    usize idx = std::distance(rects_.begin(), it);
    return getIdx(idx);
}
}  // namespace mle
