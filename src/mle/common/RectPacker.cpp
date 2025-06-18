#include "RectPacker.h"

#include <algorithm>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"

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
}  // namespace

bool RectPacker::pack() {
    if (bins_.empty()) {
        bins_.emplace_back(Rectu{{0, 0}, max_extent});
    }

    bins_.reserve(rects_.size() + 3);

    bool all_fit = true;
    packed_positions_.reserve(rects_.size());
    for (usize i_rect = packed_positions_.size(); i_rect < rects_.size(); i_rect++) {
        auto rect_size = rects_.at(i_rect).size + padding;

        MLE_VC(bins_.size());

        for (int i_bin = as<int>(bins_.size() - 1); i_bin >= 0; i_bin--) {
            auto bin = bins_.at(i_bin);
            if (aFitsIn(rect_size, bin.size)) {
                packed_positions_.emplace_back(bin.pos);
                MLE_C("Packed rect {} at pos {}", rects_.at(i_rect).id, bin.pos);

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

                MLE_ASSERT(big_remainder.size.x + big_remainder.pos.x <= max_extent.x);
                MLE_ASSERT(big_remainder.size.y + big_remainder.pos.y <= max_extent.y);

                bins_.erase(bins_.begin() + as<i64>(i_bin));

                auto insert_sorted = [this](const Rectu& rem) {
                    if (rem.size.x == 0 || rem.size.y == 0) {
                        return;
                    }

                    const u32 area = rem.size.x * rem.size.y;

                    auto it = std::ranges::lower_bound(bins_, area, {},  // No projection
                                                       [](const Rectu& bin) { return bin.size.x * bin.size.y; });

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

    MLE_C(bins_.size());
    MLE_C(rects_.size());

    return all_fit;
}
}  // namespace mle
