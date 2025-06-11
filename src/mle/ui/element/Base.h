/**
 * @file
 * @brief This defines the basic components for an element.
 * Components defined here can be present in any element.
 * This file also declares core functionality for ui elements.
 */
#pragma once

#include <entt/entt.hpp>

#include "mle/common/Color.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/renderer/Types.h"

// I will use lkh as short for Lua Key Handler

namespace mle::ui::element {
void applyTable(entt::entity entity, const sol::table& table, entt::entity parent = entt::null);

namespace comp {
struct Bounds {
    Rectf rect_on_parent = {0.0F, 0.0F, 0.0F, 0.0F};
    Rectf rect_on_screen = {0.0F, 0.0F, 0.0F, 0.0F};
};
struct Parent {
    entt::entity parent = entt::null;
};
struct Name {
    std::string name;

    static void lkhName(entt::entity e, const sol::object& obj);
};
struct SolidBackground {
    Color color{};
};
struct RootImage {
    renderer::ImageHnd image_handle{};
    Color clear_color{};
};

struct TargetBound {
    enum class Type : u8 {
        PX,  ///< Absolute
        FIT,
        PARENT,
        PARENT_PX,  ///< Position only, parent root + absolute px
        ROOT,
        ROOT_PX,  ///< Position only, root + absolute px
        FLEX_SHARE,
        CONTENT,
        CONTENT_WIDTH,
        CONTENT_HEIGHT,
        PARENT_WIDTH,
        PARENT_HEIGHT,
        DEFAULT
    };

    f32 val = 0.F;
    Type type = Type::DEFAULT;

    TargetBound() = default;

    void fromLua(const sol::object& obj);
};

struct TargetSize {
    TargetBound x;
    TargetBound y;
};
struct TargetPosition {
    TargetBound x;
    TargetBound y;
};
struct TargetMargin {
    TargetBound l, t, r, b;
};
struct TargetPadding {
    TargetBound l, t, r, b;
};
struct Origin {
    vec2f origin = {0.0F, 0.0F};
};
}  // namespace comp
}  // namespace mle::ui::element
