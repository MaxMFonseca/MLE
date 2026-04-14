#include "ContainerUtils.h"

namespace mle::ui::comp {
void finishChildBounds(const Entt& centt, ChildBoundsCalcData& cbcd, PaddingPx padding_px, vec2i padded_size) {
    vec2i origin_lt = {padding_px.l, padding_px.t};

    comp::Bounds old_bounds = cbcd.bounds;

    cbcd.bounds.parent_px.setPos(cbcd.new_position + origin_lt);
    cbcd.bounds.parent_px.setSize(cbcd.new_size);

    if (cbcd.force_fit) {
        if (cbcd.bounds.parent_px.pos().x + cbcd.bounds.parent_px.size().x > padded_size.x + padding_px.l) {
            cbcd.bounds.parent_px.setPosX(padded_size.x + padding_px.l - cbcd.bounds.parent_px.size().x);
        }
        if (cbcd.bounds.parent_px.pos().y + cbcd.bounds.parent_px.size().y > padded_size.y + padding_px.t) {
            cbcd.bounds.parent_px.setPosY(padded_size.y + padding_px.t - cbcd.bounds.parent_px.size().y);
        }
        if (cbcd.bounds.parent_px.pos().x < padding_px.l) {
            cbcd.bounds.parent_px.setPosX(padding_px.l);
        }
        if (cbcd.bounds.parent_px.pos().y < padding_px.t) {
            cbcd.bounds.parent_px.setPosY(padding_px.t);
        }
    }

    bool size_changed = old_bounds.parent_px.size() != cbcd.bounds.parent_px.size();

    if (size_changed && cbcd.size_provider) {
        cbcd.size_provider->call(centt, cbcd.bounds.parent_px.size());
    }

    if (cbcd.has_target_border) {
        auto largest_size = std::max({cbcd.new_size.x, cbcd.new_size.y});
        auto roundc = [&](const TargetBound& round_tb) {
            switch (round_tb.type) {
                case TargetBound::Type::PX: {
                    return as<int>(round_tb.val);
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE: {
                    return as<int>(as<f32>(largest_size) * round_tb.val);
                } break;
                case TargetBound::Type::RELATIVE_W: {
                    return as<int>(as<f32>(cbcd.new_size.x) * round_tb.val);
                } break;
                case TargetBound::Type::RELATIVE_H: {
                    return as<int>(as<f32>(cbcd.new_size.y) * round_tb.val);
                } break;
                default: {
                    // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                    MLE_W("Invalid border round type: {}. Treating as 0.", round_tb.type);
                    return 0;
                } break;
            }
        };

        centt.patchOrEmplace<comp::Border>([&](comp::Border& b) {
            b.t = cbcd.new_border.t;
            b.b = cbcd.new_border.b;
            b.l = cbcd.new_border.l;
            b.r = cbcd.new_border.r;
            b.round_lt = roundc(cbcd.target.border.round_lt);
            b.round_rt = roundc(cbcd.target.border.round_rt);
            b.round_lb = roundc(cbcd.target.border.round_lb);
            b.round_rb = roundc(cbcd.target.border.round_rb);
        });
    }
};

void finishChildrenBounds(const Entt& e, CBCDs& cbcds, std::span<const entt::entity> to_update, PaddingPx padding_px, vec2i padded_size) {
    for (auto c : to_update) {
        auto centt = Entt(e.ui(), c);
        auto& cbcd = cbcds.at(c);
        finishChildBounds(centt, cbcd, padding_px, padded_size);
    }
}

std::pair<vec2i, vec2i> findChildrenMaxMin(const CBCDs& cbcds, bool pack_children) {
    if (cbcds.empty()) {
        return {};
    }

    vec2i min_pos{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    vec2i max_pos{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

    for (const auto& [_, cbcd] : cbcds) {
        vec2i child_min = cbcd.bounds.parent_px.pos() - vec2i{cbcd.new_border.l, cbcd.new_border.t};
        vec2i child_max = cbcd.bounds.parent_px.pos() + cbcd.bounds.parent_px.size() + vec2i{cbcd.new_border.r, cbcd.new_border.b};

        if (!pack_children) {
            child_min -= vec2i{cbcd.new_margin.l, cbcd.new_margin.t};
            child_max += vec2i{cbcd.new_margin.r, cbcd.new_margin.b};
        }

        min_pos.x = std::min(min_pos.x, child_min.x);
        min_pos.y = std::min(min_pos.y, child_min.y);
        max_pos.x = std::max(max_pos.x, child_max.x);
        max_pos.y = std::max(max_pos.y, child_max.y);
    }

    if (min_pos.x == std::numeric_limits<int>::max() || min_pos.y == std::numeric_limits<int>::max()) {
        return {};
    }

    return {min_pos, max_pos};
}
}  // namespace mle::ui::comp
