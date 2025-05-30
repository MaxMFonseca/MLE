#include "FlexContainer.h"

#include "mle/common/Logger.h"
#include "mle/common/Utils.h"

namespace mle::ui::element_renderable {
FlexContainer::FlexContainer(ElementRef element) :
    Element::Container(element) {
    MLE_LOG_THIS;
}

void FlexContainer::set(const sol::table& obj) {
    MLE_LOG_THIS;
    if (auto r_flex = obj["flex"]; r_flex.valid()) {
        removeChildren();
        addChildren(obj["flex"]);
    }
}

void FlexContainer::updateChildrenBounds() {
    MLE_T("Updating children bounds");
    std::vector<Element::NewBounds> new_bounds;
    new_bounds.reserve(children_.size());

    for (auto& [_, child] : children_) {
        updateChildBounds(new_bounds, child.get());
    }

    children_to_update_internals_.clear();
    children_bounds_dirty_ = false;
}

void FlexContainer::updateChildBounds(std::vector<Element::NewBounds>& new_bounds_vec, ElementRef child) {
    MLE_T("Updating child bounds: {}", child->getName());

    const auto& child_target = child->getTargetBounds();
    f32 child_target_ar = child->getTargetAspectRatio();
    auto& child_new_bounds = new_bounds_vec.emplace_back();

    child_new_bounds.margin.pos.x = child_target.pos_x.val;
    if (!child_target.dep_pos_x.name.empty()) {
        auto& dep = new_bounds_vec.at(getChildPos(child_target.dep_pos_x.name));
        child_new_bounds.margin.pos.x += dep.margin.pos.x + child_target.dep_pos_x.val * dep.margin.size.x;
    }

    child_new_bounds.margin.pos.y = child_target.pos_y.val;
    if (!child_target.dep_pos_y.name.empty()) {
        auto& dep = new_bounds_vec.at(getChildPos(child_target.dep_pos_y.name));
        child_new_bounds.margin.pos.y += dep.margin.pos.y + child_target.dep_pos_y.val * dep.margin.size.y;
    }

    f32 offset_padding_x = 0.0F;
    f32 offset_margin_x = 0.0F;
    f32 offset_padding_y = 0.0F;
    f32 offset_margin_y = 0.0F;

    {
        TryComputeChildBoundData data{
            .new_bounds_vec = new_bounds_vec, .target = child_target, .target_ar = child_target_ar, .rem_x = {}, .rem_y = {}, .complete_flags = {}};

        bool complete = false;
        int max_iter = 10;
        while (max_iter-- > 0 && !complete) {
            complete = tryComputeChildBoundContentSizeX(data);
            complete &= tryComputeChildBoundContentSizeY(data);

            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::PAD_L, data.target.padding_l, new_bounds_vec.back().padding.size.x,
                                                  data.rem_x.padding_a, true, &offset_padding_x);
            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::PAD_R, data.target.padding_r, new_bounds_vec.back().padding.size.x,
                                                  data.rem_x.padding_b, true);
            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::MARG_L, data.target.margin_l, new_bounds_vec.back().margin.size.x,
                                                  data.rem_x.margin_a, true, &offset_margin_x);
            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::MARG_R, data.target.margin_r, new_bounds_vec.back().margin.size.x,
                                                  data.rem_x.margin_b, true);

            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::PAD_T, data.target.padding_t, new_bounds_vec.back().padding.size.y,
                                                  data.rem_y.padding_a, false, &offset_padding_y);
            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::PAD_B, data.target.padding_b, new_bounds_vec.back().padding.size.y,
                                                  data.rem_y.padding_b, false);
            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::MARG_T, data.target.margin_t, new_bounds_vec.back().margin.size.y,
                                                  data.rem_y.margin_a, false, &offset_margin_y);
            complete &= tryComputeChildBoundExtra(data, BoundCompleteFlagBits::MARG_B, data.target.margin_b, new_bounds_vec.back().margin.size.y,
                                                  data.rem_y.margin_b, false);
            if (!complete) {
                complete &= tryResolveRemainingX(data);
                complete &= tryResolveRemainingY(data);
            }
        }
        MLE_ASSERT_LOG(max_iter >= 0, "Iteration limit reached while computing bounds for child: {}, flags: {}", child->getName(), data.complete_flags);
    }

    child_new_bounds.padding.size += child_new_bounds.content.size;
    child_new_bounds.margin.size += child_new_bounds.padding.size;

    vec2f origin_offset = child_target.origin * child_new_bounds.margin.size;

    child_new_bounds.margin.pos -= origin_offset;

    child_new_bounds.padding.pos.x = child_new_bounds.margin.pos.x + offset_margin_x;
    child_new_bounds.padding.pos.y = child_new_bounds.margin.pos.y + offset_margin_y;

    child_new_bounds.content.pos.x = child_new_bounds.padding.pos.x + offset_padding_x;
    child_new_bounds.content.pos.y = child_new_bounds.padding.pos.y + offset_padding_y;

    Rectf e_box = element_->isRoot() ? Rectf{0, 0, 1, 1} : element_->getRectOnRoot();
    child_new_bounds.on_root.pos = e_box.pos + child_new_bounds.content.pos * e_box.size;
    child_new_bounds.on_root.size = child_new_bounds.content.size * e_box.size;
    child_new_bounds.on_root_clamped = clamp(child_new_bounds.on_root, e_box);

    f32 aspect_ratio_on_parent = child_new_bounds.content.size.x / child_new_bounds.content.size.y;
    child_new_bounds.aspect_ratio = aspect_ratio_on_parent * element_->getAspectRatio();

    child->updateInternalBounds(child_new_bounds);
}

bool FlexContainer::tryComputeChildBoundContentSizeX(TryComputeChildBoundData& data) {
    if (data.complete_flags.have(BoundCompleteFlagBits::CONTENT_X)) {
        return true;
    }
    bool complete = false;

    const auto& target = data.target;
    auto& complete_flags = data.complete_flags;
    auto& new_bounds_vec = data.new_bounds_vec;
    auto& new_bound = new_bounds_vec.back();
    const auto& dep_vec = target.dep_size_x;
    auto& rem = data.rem_x;

    switch (target.size_x.type) {
        case Element::Bound::Type::PARENT: {
            if (!dep_vec.empty()) {
                for (const auto& [val, name, is_x] : dep_vec) {
                    const auto& dep_bbox = new_bounds_vec[getChildPos(name)].content;
                    if (is_x) {
                        new_bound.content.size.x += dep_bbox.size.x * val;
                    } else {
                        new_bound.content.size.x += dep_bbox.size.y * val / element_->getAspectRatio();
                    }
                }
                if (target.size_x.val != 0) {
                    new_bound.content.size.x = target.size_x.val;
                }
                complete = true;
            } else if (target.size_x.val != 0) {
                new_bound.content.size.x = target.size_x.val;
                complete = true;
            } else if (data.target_ar != 0) {
                if (complete_flags.have(BoundCompleteFlagBits::CONTENT_Y)) {
                    new_bound.content.size.x = data.target_ar * new_bounds_vec.back().content.size.y;
                    new_bound.content.size.x /= element_->getAspectRatio();
                    complete = true;
                }
            } else {
                rem.content = 1;
            }

        } break;
        case Element::Bound::Type::REMAINING_SHARE: {
            rem.content = target.size_x.val;
        } break;
        case Element::Bound::Type::SELF_Y: {
            if (complete_flags.have(BoundCompleteFlagBits::CONTENT_Y)) {
                new_bound.content.size.x = target.size_x.val * new_bound.content.size.y;
                new_bound.content.size.x /= element_->getAspectRatio();
                complete = true;
            }
        } break;
        case Element::Bound::Type::PARENT_SIZE_X: {
            new_bound.content.size.x = target.size_x.val;
            complete = true;
        } break;
        case Element::Bound::Type::PARENT_SIZE_Y: {
            new_bound.content.size.x = target.size_x.val / element_->getAspectRatio();
            complete = true;
        } break;
        default: {
            MLE_UNREACHABLE_LOG("Invalid size type: {}", target.size_x.type);
        } break;
    }

    if (complete) {
        complete_flags.set(BoundCompleteFlagBits::CONTENT_X);
        return true;
    }
    return false;
}

bool FlexContainer::tryComputeChildBoundContentSizeY(TryComputeChildBoundData& data) {
    if (data.complete_flags.have(BoundCompleteFlagBits::CONTENT_Y)) {
        return true;
    }
    bool complete = false;
    const auto& target = data.target;
    auto& complete_flags = data.complete_flags;
    auto& new_bounds_vec = data.new_bounds_vec;
    auto& new_bound = new_bounds_vec.back();
    const auto& dep_vec = target.dep_size_y;
    auto& rem = data.rem_y;

    switch (target.size_y.type) {
        case Element::Bound::Type::PARENT: {
            if (!dep_vec.empty()) {
                for (const auto& [val, name, is_x] : dep_vec) {
                    const auto& dep_bbox = new_bounds_vec[getChildPos(name)].content;
                    if (is_x) {
                        new_bound.content.size.y += dep_bbox.size.x * val * element_->getAspectRatio();
                    } else {
                        new_bound.content.size.y += dep_bbox.size.y * val;
                    }
                }
                if (target.size_y.val != 0) {
                    new_bound.content.size.y = target.size_y.val;
                }
                complete = true;
            } else if (target.size_y.val != 0) {
                new_bound.content.size.y = target.size_y.val;
                complete = true;
            } else if (data.target_ar != 0) {
                if (complete_flags.have(BoundCompleteFlagBits::CONTENT_X)) {
                    new_bound.content.size.y = new_bounds_vec.back().content.size.x / data.target_ar;
                    new_bound.content.size.y *= element_->getAspectRatio();
                    complete = true;
                }
            } else {
                rem.content = 1;
            }
        } break;
        case Element::Bound::Type::REMAINING_SHARE: {
            rem.content = target.size_y.val;
        } break;
        case Element::Bound::Type::SELF_X: {
            if (complete_flags.have(BoundCompleteFlagBits::CONTENT_X)) {
                new_bound.content.size.y = target.size_y.val * new_bound.content.size.x;
                new_bound.content.size.y *= element_->getAspectRatio();
                complete = true;
            }
        } break;
        case Element::Bound::Type::PARENT_SIZE_X: {
            new_bound.content.size.y = target.size_y.val * element_->getAspectRatio();
            complete = true;
        } break;
        case Element::Bound::Type::PARENT_SIZE_Y: {
            new_bound.content.size.y = target.size_y.val;
            complete = true;
        } break;
        default: {
            MLE_UNREACHABLE_LOG("Invalid size type: {}", target.size_y.type);
        } break;
    }

    if (complete) {
        complete_flags.set(BoundCompleteFlagBits::CONTENT_Y);
        return true;
    }
    return false;
}

bool FlexContainer::tryComputeChildBoundExtra(TryComputeChildBoundData& data, BoundCompleteFlagBits flag_bit, Element::Bound target_bound, f32& new_bound,
                                              f32& greedy, bool is_x, f32* set_offset) {
    if (data.complete_flags.have(flag_bit)) {
        return true;
    }
    auto& complete_flags = data.complete_flags;
    bool complete = false;
    auto& new_bounds = data.new_bounds_vec.back();

    switch (target_bound.type) {
        case Element::Bound::Type::PARENT: {
            new_bound += target_bound.val;
            complete = true;
        } break;
        case Element::Bound::Type::REMAINING_SHARE: {
            greedy = target_bound.val;
        } break;
        case Element::Bound::Type::SELF_X: {
            if (complete_flags.have(BoundCompleteFlagBits::CONTENT_X)) {
                auto temp = new_bounds.content.size.x * target_bound.val;
                if (!is_x) {
                    temp *= element_->getAspectRatio();
                }
                new_bound += temp;
                complete = true;
            }
        } break;
        case Element::Bound::Type::SELF_Y: {
            if (complete_flags.have(BoundCompleteFlagBits::CONTENT_Y)) {
                auto temp = new_bounds.content.size.y * target_bound.val;
                if (is_x) {
                    temp /= element_->getAspectRatio();
                }
                new_bound += temp;
                complete = true;
            }
        } break;
        default: {
            MLE_UNREACHABLE_LOG("Invalid padding type: {}", target_bound.type);
        } break;
    }

    if (complete) {
        if (set_offset != nullptr) {
            *set_offset = new_bound;
        }
        complete_flags.set(flag_bit);
        return true;
    }
    return false;
}

bool FlexContainer::tryResolveRemainingX(FlexContainer::TryComputeChildBoundData& data) {
    if (data.complete_flags.have(BoundCompleteFlagBits::REM_X)) {
        return true;
    }
    auto& new_bound = data.new_bounds_vec.back();

    auto& rem = data.rem_x;
    // clang-format off
    if ((rem.content != 0 || data.complete_flags.have(BoundCompleteFlagBits::CONTENT_X)) && 
        (rem.padding_a != 0 || data.complete_flags.have(BoundCompleteFlagBits::PAD_L)) && 
        (rem.padding_b != 0 || data.complete_flags.have(BoundCompleteFlagBits::PAD_R)) && 
        (rem.margin_a != 0 || data.complete_flags.have(BoundCompleteFlagBits::MARG_L)) &&
        (rem.margin_b != 0 || data.complete_flags.have(BoundCompleteFlagBits::MARG_R))) {
        // clang-format on

        f32 remaining_size = 1.0F - new_bound.content.size.x - data.new_bounds_vec.back().padding.size.x - data.new_bounds_vec.back().margin.size.x;
        f32 greedy_share_size = remaining_size / std::max(rem.content + rem.padding_a + rem.padding_b + rem.margin_a + rem.margin_b, 1.0F);

        if (rem.content != 0) {
            new_bound.content.size.x += rem.content * greedy_share_size;
        }
        if (rem.padding_a != 0) {
            new_bound.padding.size.x += rem.padding_a * greedy_share_size;
        }
        if (rem.padding_b != 0) {
            new_bound.padding.size.x += rem.padding_b * greedy_share_size;
        }
        if (rem.margin_a != 0) {
            new_bound.margin.size.x += rem.margin_a * greedy_share_size;
        }
        if (rem.margin_b != 0) {
            new_bound.margin.size.x += rem.margin_b * greedy_share_size;
        }

        data.complete_flags.set(BoundCompleteFlagBits::CONTENT_X);
        data.complete_flags.set(BoundCompleteFlagBits::PAD_L);
        data.complete_flags.set(BoundCompleteFlagBits::PAD_R);
        data.complete_flags.set(BoundCompleteFlagBits::MARG_L);
        data.complete_flags.set(BoundCompleteFlagBits::MARG_R);
        data.complete_flags.set(BoundCompleteFlagBits::REM_X);
        return true;
    }

    return false;
}

bool FlexContainer::tryResolveRemainingY(TryComputeChildBoundData& data) {
    if (data.complete_flags.have(BoundCompleteFlagBits::REM_Y)) {
        return true;
    }
    auto& new_bound = data.new_bounds_vec.back();
    auto& rem = data.rem_y;

    // clang-format off
    if ((rem.content != 0 || data.complete_flags.have(BoundCompleteFlagBits::CONTENT_Y)) && 
        (rem.padding_a != 0 || data.complete_flags.have(BoundCompleteFlagBits::PAD_T)) && 
        (rem.padding_b != 0 || data.complete_flags.have(BoundCompleteFlagBits::PAD_B)) && 
        (rem.margin_a != 0 || data.complete_flags.have(BoundCompleteFlagBits::MARG_T)) &&
        (rem.margin_b != 0 || data.complete_flags.have(BoundCompleteFlagBits::MARG_B))) {
        // clang-format on
        f32 remaining_size = 1.0F - new_bound.content.size.y - data.new_bounds_vec.back().padding.size.y - data.new_bounds_vec.back().margin.size.y;
        f32 greedy_share_size = remaining_size / std::max(rem.content + rem.padding_a + rem.padding_b + rem.margin_a + rem.margin_b, 1.0F);

        if (rem.content != 0) {
            new_bound.content.size.y += rem.content * greedy_share_size;
        }
        if (rem.padding_a != 0) {
            new_bound.padding.size.y += rem.padding_a * greedy_share_size;
        }
        if (rem.padding_b != 0) {
            new_bound.padding.size.y += rem.padding_b * greedy_share_size;
        }
        if (rem.margin_a != 0) {
            new_bound.margin.size.y += rem.margin_a * greedy_share_size;
        }
        if (rem.margin_b != 0) {
            new_bound.margin.size.y += rem.margin_b * greedy_share_size;
        }

        data.complete_flags.set(BoundCompleteFlagBits::CONTENT_Y);
        data.complete_flags.set(BoundCompleteFlagBits::PAD_T);
        data.complete_flags.set(BoundCompleteFlagBits::PAD_B);
        data.complete_flags.set(BoundCompleteFlagBits::MARG_T);
        data.complete_flags.set(BoundCompleteFlagBits::MARG_B);
        data.complete_flags.set(BoundCompleteFlagBits::REM_Y);

        return true;
    }
    return false;
}

void FlexContainer::registerLuaTypes() {
    auto text_ut =
        lua::newUsertype<FlexContainer>("ui_FlexContainer", sol::no_constructor, sol::base_classes, sol::bases<Element::Container, Element::Renderable>());
    text_ut["newRaw"] = [](ElementRef element) { return new FlexContainer(element); };
}
}  // namespace mle::ui::element_renderable
