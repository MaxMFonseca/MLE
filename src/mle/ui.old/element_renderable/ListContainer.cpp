#include "ListContainer.h"

#include "mle/lua/Lua.h"

namespace mle::ui::element_renderable {
ListContainer::ListContainer(ElementRef element) :
    Element::Container(element) {
    MLE_LOG_THIS;
}

void ListContainer::set(const sol::table& obj) {
    removeChildren();

    if (auto r_spacing = obj["spacing"]; r_spacing.valid()) {
        spacing_.val = r_spacing;
    }
    if (auto r_static_share_size = obj["static_share_size"]; r_static_share_size.valid()) {
        static_share_size_ = r_static_share_size;
    }
    if (auto r_layout = obj["layout"]; r_layout.valid()) {
        auto s = r_layout.get<std::string>();
        if (s == "rows" || s == "row" || s == "r") {
            layout_ = Layout::ROWS;
        } else if (s == "cols" || s == "col" || s == "c") {
            layout_ = Layout::COLS;
        } else {
            MLE_UNREACHABLE_LOG("Invalid layout: {}", s);
        }
    }

    MLE_ASSERT(obj["list"].valid());
    addChildren(obj["list"]);
}

void ListContainer::updateChildrenBounds() {
    MLE_T("Updating children bounds");
    if (layout_ == Layout::ROWS) {
        updateChildrenBoundsRows();
    } else {
        updateChildrenBoundsCols();
    }
    children_to_update_internals_.clear();
    children_bounds_dirty_ = false;
}

void ListContainer::updateChildrenBoundsRows() {
    f32 remaining_size = 1.0F;
    f32 shares = 0.0F;
    std::vector<Element::NewBounds> new_bounds_vec;
    new_bounds_vec.reserve(children_.size());
    std::vector<RemainginSizeHelper> remaining_helper;
    remaining_helper.reserve(children_.size());

    for (auto& i : children_) {
        auto& child = *i.second;
        auto& child_new_bounds = new_bounds_vec.emplace_back();
        const auto& child_target = child.getTargetBounds();
        auto& rem = remaining_helper.emplace_back();

        calcMainBound(child_target.size_y, child_new_bounds.content.size.y, rem.content, remaining_size, shares, false, true);
        calcMainBound(child_target.padding_t, child_new_bounds.content.pos.y, rem.padding_a, remaining_size, shares, false);
        calcMainBound(child_target.padding_b, child_new_bounds.padding.size.y, rem.padding_b, remaining_size, shares, false);
        calcMainBound(child_target.margin_t, child_new_bounds.padding.pos.y, rem.margin_a, remaining_size, shares, false);
        calcMainBound(child_target.margin_b, child_new_bounds.margin.size.y, rem.margin_b, remaining_size, shares, false);
    }

    remaining_size -= static_cast<f32>(children_.size() - 1) * spacing_.val;
    f32 share_size = std::max(shares != 0.0F ? remaining_size / shares : 0, 0.0F);
    f32 current_pos = 0;

    for (usize i = 0; i < children_.size(); ++i) {
        auto& child = *children_.at(i).second;
        auto& child_new_bounds = new_bounds_vec.at(i);
        auto& rem_y = remaining_helper.at(i);
        RemainginSizeHelper rem_x;
        const auto& child_target = child.getTargetBounds();
        f32 child_ar = child.getTargetAspectRatio();

        finishMainBoundRemaining(child_new_bounds.content.size.y, rem_y.content, share_size);
        finishMainBoundRemaining(child_new_bounds.content.pos.y, rem_y.padding_a, share_size);
        finishMainBoundRemaining(child_new_bounds.padding.size.y, rem_y.padding_b, share_size);
        finishMainBoundRemaining(child_new_bounds.padding.pos.y, rem_y.margin_a, share_size);
        finishMainBoundRemaining(child_new_bounds.margin.size.y, rem_y.margin_b, share_size);

        {
            child_new_bounds.margin.pos.x = child_target.pos_x.val;

            f32 remaining_size = 1.0F - child_new_bounds.margin.pos.x;
            f32 shares = 0.0F;

            calcOffBound(child_target.size_x, child_new_bounds.content.size.x, rem_x.content, shares, remaining_size, true, true, child_ar,
                         child_new_bounds.content.size.y);
            calcOffBound(child_target.padding_l, child_new_bounds.content.pos.x, rem_x.padding_a, shares, remaining_size, true);
            calcOffBound(child_target.padding_r, child_new_bounds.padding.size.x, rem_x.padding_b, shares, remaining_size, true);
            calcOffBound(child_target.margin_l, child_new_bounds.padding.pos.x, rem_x.margin_a, shares, remaining_size, true);
            calcOffBound(child_target.margin_r, child_new_bounds.margin.size.x, rem_x.margin_b, shares, remaining_size, true);

            f32 share_size = std::max(shares != 0.0F ? remaining_size / shares : 0, 0.0F);

            child_new_bounds.content.size.x += share_size * rem_x.content;
            child_new_bounds.content.pos.x += share_size * rem_x.padding_a;
            child_new_bounds.padding.size.x += share_size * rem_x.padding_b;
            child_new_bounds.padding.pos.x += share_size * rem_x.margin_a;
            child_new_bounds.margin.size.x += share_size * rem_x.margin_b;
        }

        child_new_bounds.padding.size += child_new_bounds.content.size + child_new_bounds.content.pos;
        child_new_bounds.margin.size += child_new_bounds.padding.size + child_new_bounds.padding.pos;

        auto origin = child_target.origin * child_new_bounds.margin.size;

        child_new_bounds.margin.pos.y = current_pos - origin.y;
        child_new_bounds.padding.pos.y += child_new_bounds.margin.pos.y;
        child_new_bounds.content.pos.y += child_new_bounds.padding.pos.y;
        current_pos += child_new_bounds.margin.size.y + spacing_.val;

        child_new_bounds.margin.pos.x -= origin.x;
        child_new_bounds.padding.pos.x += child_new_bounds.margin.pos.x;
        child_new_bounds.content.pos.x += child_new_bounds.padding.pos.x;

        finishChildBoundsCalc(children_.at(i).second.get(), child_new_bounds);
    }
}

void ListContainer::updateChildrenBoundsCols() {
    f32 remaining_size = 1.0F;
    f32 shares = 0.0F;
    std::vector<Element::NewBounds> new_bounds_vec;
    new_bounds_vec.reserve(children_.size());
    std::vector<RemainginSizeHelper> remaining_helper;
    remaining_helper.reserve(children_.size());

    for (auto& i : children_) {
        auto& child = *i.second;
        auto& child_new_bounds = new_bounds_vec.emplace_back();
        const auto& child_target = child.getTargetBounds();
        auto& rem = remaining_helper.emplace_back();

        calcMainBound(child_target.size_x, child_new_bounds.content.size.x, rem.content, remaining_size, shares, true, true);
        calcMainBound(child_target.padding_l, child_new_bounds.content.pos.x, rem.padding_a, remaining_size, shares, true);
        calcMainBound(child_target.padding_r, child_new_bounds.padding.size.x, rem.padding_b, remaining_size, shares, true);
        calcMainBound(child_target.margin_l, child_new_bounds.padding.pos.x, rem.margin_a, remaining_size, shares, true);
        calcMainBound(child_target.margin_r, child_new_bounds.margin.size.x, rem.margin_b, remaining_size, shares, true);
    }

    remaining_size -= static_cast<f32>(children_.size() - 1) * spacing_.val;
    f32 share_size = std::max(shares != 0.0F ? remaining_size / shares : 0, 0.0F);
    f32 current_pos = 0;

    for (usize i = 0; i < children_.size(); ++i) {
        auto& child = *children_.at(i).second;
        auto& child_new_bounds = new_bounds_vec.at(i);
        auto& rem_x = remaining_helper.at(i);
        RemainginSizeHelper rem_y;
        const auto& child_target = child.getTargetBounds();
        f32 child_ar = child.getTargetAspectRatio();

        finishMainBoundRemaining(child_new_bounds.content.size.x, rem_x.content, share_size);
        finishMainBoundRemaining(child_new_bounds.content.pos.x, rem_x.padding_a, share_size);
        finishMainBoundRemaining(child_new_bounds.padding.size.x, rem_x.padding_b, share_size);
        finishMainBoundRemaining(child_new_bounds.padding.pos.x, rem_x.margin_a, share_size);
        finishMainBoundRemaining(child_new_bounds.margin.size.x, rem_x.margin_b, share_size);

        {
            child_new_bounds.margin.pos.y = child_target.pos_y.val;

            f32 remaining_size = 1.0F - child_new_bounds.margin.pos.y;
            f32 shares = 0.0F;

            calcOffBound(child_target.size_y, child_new_bounds.content.size.y, rem_y.content, shares, remaining_size, false, true, child_ar,
                         child_new_bounds.content.size.x);
            calcOffBound(child_target.padding_t, child_new_bounds.content.pos.y, rem_y.padding_a, shares, remaining_size, false);
            calcOffBound(child_target.padding_b, child_new_bounds.padding.size.y, rem_y.padding_b, shares, remaining_size, false);
            calcOffBound(child_target.margin_t, child_new_bounds.padding.pos.y, rem_y.margin_a, shares, remaining_size, false);
            calcOffBound(child_target.margin_b, child_new_bounds.margin.size.y, rem_y.margin_b, shares, remaining_size, false);

            f32 share_size = std::max(shares != 0.0F ? remaining_size / shares : 0, 0.0F);

            child_new_bounds.content.size.y += share_size * rem_y.content;
            child_new_bounds.content.pos.y += share_size * rem_y.padding_a;
            child_new_bounds.padding.size.y += share_size * rem_y.padding_b;
            child_new_bounds.padding.pos.y += share_size * rem_y.margin_a;
            child_new_bounds.margin.size.y += share_size * rem_y.margin_b;
        }

        child_new_bounds.padding.size += child_new_bounds.content.size + child_new_bounds.content.pos;
        child_new_bounds.margin.size += child_new_bounds.padding.size + child_new_bounds.padding.pos;

        auto origin = child_target.origin * child_new_bounds.margin.size;

        child_new_bounds.margin.pos.x = current_pos - origin.x;
        child_new_bounds.padding.pos.x += child_new_bounds.margin.pos.x;
        child_new_bounds.content.pos.x += child_new_bounds.padding.pos.x;
        current_pos += child_new_bounds.margin.size.x + spacing_.val;

        child_new_bounds.margin.pos.y -= origin.y;
        child_new_bounds.padding.pos.y += child_new_bounds.margin.pos.y;
        child_new_bounds.content.pos.y += child_new_bounds.padding.pos.y;

        finishChildBoundsCalc(children_.at(i).second.get(), child_new_bounds);
    }
}

void ListContainer::calcMainBound(const Element::Bound& target, f32& new_bound, f32& rem, f32& remaining_size, f32& shares, bool is_x, bool is_content) const {
    switch (target.type) {
        case Element::Bound::Type::PARENT: {
            if (target.val != 0.0F) {
                new_bound = target.val;
            } else if (is_content) {
                rem = 1;
            }
        } break;
        case Element::Bound::Type::REMAINING_SHARE: {
            rem = target.val;
        } break;
        case Element::Bound::Type::STATIC_SHARE: {
            new_bound = static_share_size_ * target.val;
        } break;
        case Element::Bound::Type::PARENT_SIZE_X: {
            if (is_x) {
                new_bound = target.val;
            } else {
                new_bound = target.val * element_->getAspectRatio();
            }
        } break;
        case Element::Bound::Type::PARENT_SIZE_Y: {
            if (is_x) {
                new_bound = target.val / element_->getAspectRatio();
            } else {
                new_bound = target.val;
            }
        } break;

        default: {
            MLE_UNREACHABLE_LOG("Invalid type: {}", target.type);
        }
    }

    remaining_size -= new_bound;
    shares += rem;
}

void ListContainer::calcOffBound(const Element::Bound& target, f32& new_bound, f32& rem, f32& shares, f32& remaining_size, bool is_x, bool is_content,
                                 f32 child_ar, f32 main_new_bound) const {
    switch (target.type) {
        case Element::Bound::Type::PARENT: {
            if (target.val != 0.0F) {
                new_bound = target.val;
            } else if (is_content) {
                if (child_ar != 0.0F) {
                    if (is_x) {
                        new_bound = (main_new_bound * child_ar) / element_->getAspectRatio();
                    } else {
                        new_bound = main_new_bound / (child_ar * element_->getAspectRatio());
                    }
                } else {
                    rem = 1;
                }
            }
        } break;
        case Element::Bound::Type::REMAINING_SHARE: {
            rem = target.val;
        } break;
        case Element::Bound::Type::PARENT_SIZE_X: {
            if (is_x) {
                new_bound = target.val;
            } else {
                new_bound = target.val * element_->getAspectRatio();
            }
        } break;
        case Element::Bound::Type::PARENT_SIZE_Y: {
            if (is_x) {
                new_bound = target.val / element_->getAspectRatio();
            } else {
                new_bound = target.val;
            }
        } break;
        default: {
            MLE_UNREACHABLE_LOG("Invalid type: {}", target.type);
        }
    }
    shares += rem;
    remaining_size -= new_bound;
}

void ListContainer::finishMainBoundRemaining(f32& new_bound, f32 rem, f32 share_size) {
    if (rem != 0.0F) {
        new_bound = share_size * rem;
    }
}

void ListContainer::finishChildBoundsCalc(ElementRef child, Element::NewBounds& child_new_bounds) {
    Rectf e_box = element_->getRectOnRoot();
    child_new_bounds.on_root.pos = e_box.pos + child_new_bounds.content.pos * e_box.size;
    child_new_bounds.on_root.size = child_new_bounds.content.size * e_box.size;
    child_new_bounds.on_root_clamped = clamp(child_new_bounds.on_root, e_box);

    f32 aspect_ratio_on_parent = child_new_bounds.content.size.x / child_new_bounds.content.size.y;
    child_new_bounds.aspect_ratio = aspect_ratio_on_parent * element_->getAspectRatio();

    child->updateInternalBounds(child_new_bounds);
}

void ListContainer::registerLuaTypes() {
    auto text_ut =
        lua::newUsertype<ListContainer>("ui_ListContainer", sol::no_constructor, sol::base_classes, sol::bases<Element::Container, Element::Renderable>());
    text_ut["newRaw"] = [](ElementRef element) { return new ListContainer(element); };
}
}  // namespace mle::ui::element_renderable
