#include "RectPacker.h"

#include <algorithm>
#include <numeric>

namespace mle {
namespace {
inline bool aFitsIn(vec2u a, vec2u in) {
    return a.x <= in.x && a.y <= in.y;
}
}  // namespace

std::vector<vec2u> RectPacker::pack(std::span<const vec2u> rect_sizes) {
    bins_.reserve(bins_.size() + rect_sizes.size() + 3);

    std::vector<vec2u> packed_positions;
    packed_positions.reserve(rect_sizes.size());

    for (vec2u size : rect_sizes) {
        if (auto pos = tryPackOne(size)) {
            packed_positions.emplace_back(*pos);
        } else {
            break;
        }
    }
    return packed_positions;
}

std::optional<vec2u> RectPacker::tryPackOne(vec2u packing_size) {
    for (i64 i_bin = static_cast<i64>(bins_.size()) - 1; i_bin >= 0; --i_bin) {
        const Rectu bin = bins_[as<usize>(i_bin)];
        vec2u test_size = packing_size;
        if (bin.pos().x + bin.size().x >= extent_.x) {
            test_size.x += padding_.x;
        }
        if (bin.pos().y + bin.size().y >= extent_.y) {
            test_size.y += padding_.y;
        }
        if (!aFitsIn(test_size, bin.size())) {
            continue;
        }
        packing_size += padding_;

        const vec2u placed_pos = bin.pos();

        Rectu big_remainder;
        Rectu small_remainder;

        const u32 rem_w = bin.size().x - packing_size.x;
        const u32 rem_h = bin.size().y - packing_size.y;

        if (rem_w > rem_h) {
            big_remainder.setPos({bin.pos().x + packing_size.x, bin.pos().y});
            big_remainder.setSize({rem_w, bin.size().y});

            small_remainder.setPos({bin.pos().x, bin.pos().y + packing_size.y});
            small_remainder.setSize({packing_size.x, rem_h});
        } else {
            big_remainder.setPos({bin.pos().x, bin.pos().y + packing_size.y});
            big_remainder.setSize({bin.size().x, rem_h});

            small_remainder.setPos({bin.pos().x + packing_size.x, bin.pos().y});
            small_remainder.setSize({rem_w, packing_size.y});
        }

        bins_.erase(bins_.begin() + static_cast<isize>(i_bin));

        auto insert_sorted = [this](const Rectu& rem) {
            if (rem.width() < bin_min_size_ + padding_.x) {
                return;
            }
            if (rem.height() < bin_min_size_ + padding_.y) {
                return;
            }

            const u32 area = rem.size().x * rem.size().y;
            auto it = std::ranges::lower_bound(bins_, area, std::greater<>{}, [](const Rectu& b) { return b.size().x * b.size().y; });
            bins_.insert(it, rem);
        };

        insert_sorted(big_remainder);
        insert_sorted(small_remainder);

        return placed_pos;
    }

    return std::nullopt;
}

}  // namespace mle
