#pragma once

#include <optional>
#include <vector>

#include "mle/common/Types.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"

// This is heavy on dallocs, but it should be fine
// Maybe add a fixed size later?
namespace mle {
struct RectPacker {
    vec2u padding{};

    vec2u max_extent{max<u32>()};

    struct Rect {
        vec2u size;
        u32 id{0};  // For external usage, maybe opt out
    };
    std::vector<Rect> rects_;

    bool pack(vec2u ov_extent = {});  // This will pack from packed_positions_.size() up to rects_.size();

    bool tryPackOptimal(f32 scale = 1.05);  // This will clear and try pack with different max_extent until a valid packing is found.

    void sort();  // This calls clear

    void clear();

    [[nodiscard]] auto currentExtent() const { return current_extent_; }
    [[nodiscard]] std::optional<vec2u> get(usize i) const {
        if (packed_positions_.at(i).x == max<u32>()) {
            return std::nullopt;
        }
        return packed_positions_.at(i);
    }
    [[nodiscard]] auto packedCound() const { return packed_positions_.size(); }

  private:
    std::vector<Rectu> bins_;
    std::vector<vec2u> packed_positions_;  // This matches rects_
    vec2u current_extent_{};
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::RectPacker> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::RectPacker& p, FormatContext& ctx) const {
        auto out = ctx.out();
        out = format_to(out, "RectPacker(\n  padding={},\n  max_extent={},\n  rects_=[", p.padding, p.max_extent);
        for (size_t i = 0; i < p.packedCound(); ++i) {
            auto ii = p.get(i);
            if (!ii) {
                out = format_to(out, "{}: <not packed>", p.rects_[i].id);
            } else {
                mle::Rectu r{ii.value(), p.rects_[i].size};
                out = format_to(out, "{}: {}", p.rects_[i].id, r);
            }
            if (i + 1 < p.rects_.size()) {
                out = format_to(out, ", ");
            }
        }
        auto extent = p.currentExtent();
        out = format_to(out, "],\n  current_extent={}\n)", extent);
        return out;
    }
};
}  // namespace fmt
