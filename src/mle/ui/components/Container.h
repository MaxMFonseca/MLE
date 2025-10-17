#pragma once

#include "../Types.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Utils.h"

namespace mle::ui::comp {
struct Container {
    enum class ListDirection : u8 { HORIZONTAL, VERTICAL, HORIZONTAL_REVERSED, VERTICAL_REVERSED };
    enum class ListAlignCross : u8 { START, CENTER, END };
    enum class ListAlignMain : u8 { START, CENTER, END, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };
    enum class WrapMode : u8 { NO, WRAP, WRAP_REVERSE };

    EntityStorage o;
    int offset_x = 0, offset_y = 0;
    ListDirection list_direction = ListDirection::VERTICAL;
    ListAlignCross list_align_cross = ListAlignCross::START;
    ListAlignMain list_align_main = ListAlignMain::START;
    WrapMode wrap_mode = WrapMode::NO;
    bool scrollable = true;

    TargetBound gap;

    vec2u internal_size = {0, 0};

    sol::table element_base;

    void addChild(const Entt& e, std::string name, const sol::object& obj, usize pos = max<usize>());

    void computeChildrenBounds(const Entt& e, vec2u max_size);

    static vec2u sizeProvider(const Entt& e, vec2u max_size);
    static void setAsSizeProvider(const Entt& e);

    // TODO: this
    static void setAsRenderProvider(const Entt& e);

    Container() = default;
    explicit Container(const Entt& e, const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyAdd(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::comp
