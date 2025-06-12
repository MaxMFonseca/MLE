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
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/Utils.h"

// I will use lkh as short for Lua Key Handler

namespace mle::ui::element {
void applyTable(entt::entity entity, const sol::table& table, entt::entity parent = entt::null);
bool isFitX(entt::entity e);
bool isFitY(entt::entity e);
bool isFit(entt::entity e);

// template <typename T>
// bool has(entt::entity e) {
//     return getRegistry().any_of<T>(e);
// }

// entt::entity findFirstFixedSizedX(entt::entity e);
// entt::entity findFirstFixedSizedY(entt::entity e);

namespace comp {
struct Bounds {
    Recti bounds{};
    bool immutable = false;  // static would be a better name, but it is reserved
};
struct Parent {
    entt::entity parent = entt::null;
};
struct Name {
    std::string name;
};
struct SolidBackground {
    Color color{};

    static renderer::PipelineRef getPipeline();
};
/**
 * @brief Represents a bound or constraint for UI element positioning or sizing.
 *
 * TargetBound specifies how a UI element's position or size should be calculated,
 * supporting various modes such as absolute pixels, fitting to content, or relative
 * to parent/root elements.
 */
struct TargetBound {
    /**
     * @brief Types of bounds for UI elements.
     */
    enum class Type : u8 {
        PX,          ///< Absolute value in pixels. Suffix: "px"
        FIT,         ///< Fit to available space or content. Suffix: "fit"
        PARENT,      ///< Relative to parent element (percentage). Suffix: "%"
        PARENT_W,    ///< Width relative to parent. Suffix: "pw"
        PARENT_H,    ///< Height relative to parent. Suffix: "ph"
        PARENT_PX,   ///< Parent-relative with absolute pixel offset. Suffix: "ppx"
        ROOT,        ///< Relative to root element (percentage). Suffix: "r%"
        ROOT_PX,     ///< Root-relative with absolute pixel offset. Suffix: "rpx"
        FLEX_SHARE,  ///< Share available space flexibly. Suffix: "flex"
        SELF,        ///< Size based on self/content. Suffix: "s"
        SELF_W,      ///< Width based on self/content. Suffix: "sw"
        SELF_H,      ///< Height based on self/content. Suffix: "sh"
        DEFAULT      ///< Default behavior. No suffix or unrecognized.
    };

    f32 val = 0.F;              ///< Value associated with the bound (e.g., pixel value or ratio).
    Type type = Type::DEFAULT;  ///< Type of the bound.

    /**
     * @brief Default constructor.
     */
    TargetBound() = default;

    /**
     * @brief Initializes the TargetBound from a Lua object.
     * @param obj The Lua object to parse.
     */
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

namespace fmt {
template <>
struct formatter<mle::ui::element::comp::TargetBound::Type> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::element::comp::TargetBound::Type v, FormatContext& ctx) const {
        using Type = mle::ui::element::comp::TargetBound::Type;
        switch (v) {
            case Type::PX:
                return formatter<std::string>::format("px", ctx);
            case Type::FIT:
                return formatter<std::string>::format("fit", ctx);
            case Type::PARENT:
                return formatter<std::string>::format("%", ctx);
            case Type::PARENT_W:
                return formatter<std::string>::format("pw", ctx);
            case Type::PARENT_H:
                return formatter<std::string>::format("ph", ctx);
            case Type::PARENT_PX:
                return formatter<std::string>::format("ppx", ctx);
            case Type::ROOT:
                return formatter<std::string>::format("r%", ctx);
            case Type::ROOT_PX:
                return formatter<std::string>::format("rpx", ctx);
            case Type::FLEX_SHARE:
                return formatter<std::string>::format("flex", ctx);
            case Type::SELF:
                return formatter<std::string>::format("s", ctx);
            case Type::SELF_W:
                return formatter<std::string>::format("sw", ctx);
            case Type::SELF_H:
                return formatter<std::string>::format("sh", ctx);
            case Type::DEFAULT:
                return formatter<std::string>::format("default", ctx);
        }
        MLE_TODO;
    }
};
};  // namespace fmt
