#pragma once

#include "mle/common/Assert.h"
#include "mle/common/math/Types2D.h"
#include "mle/lua/Types.h"
#include "mle/ui/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::element::comp {
struct TargetBound {
    enum class Type : u8 {
        PX,          ///< Absolute value in pixels. Suffix: "px"
        FIT,         ///< Content fit. Suffix: "fit"
        PARENT,      ///< Relative to parent element (percentage). Suffix: "%"
        PARENT_W,    ///< Width relative to parent. Suffix: "pw"
        PARENT_H,    ///< Height relative to parent. Suffix: "ph"
        ROOT,        ///< Relative to root element (percentage). Suffix: "r%"
        FLEX_SHARE,  ///< Share available space flexibly. Suffix: "f"
        SELF,        ///< Size based on self content. Suffix: "s"
        SELF_W,      ///< Width based on self content. Suffix: "sw"
        SELF_H,      ///< Height based on self content. Suffix: "sh"
        DEFAULT      ///< Default behavior. No suffix or unrecognized.
    };

    f32 val = 0.F;
    Type type = Type::DEFAULT;

    TargetBound() = default;

    void apply(const sol::object& obj);
    void applyStr(const std::string& str);

    static constexpr bool typeIsPercentage(Type v) {
        return v == Type::PARENT || v == Type::PARENT_W || v == Type::PARENT_H || v == Type::ROOT || v == Type::SELF_W || v == Type::SELF_H;
    }
};

struct Bounds {
    Recti parent_px{};

    void apply(entt::entity self, const sol::object& obj);
};

struct Padding {
    TargetBound t, b, l, r;

    void apply(entt::entity self, const sol::object& obj);

    // BoxFace enum
    [[nodiscard]] std::array<int, 4> calcOnRect(Recti rect) const;
};

struct TargetSize {
    TargetBound x;
    TargetBound y;

    void apply(entt::entity self, const sol::object& obj);
    void applyX(entt::entity self, const sol::object& obj);
    void applyY(entt::entity self, const sol::object& obj);
};

struct TargetPosition {
    TargetBound x;
    TargetBound y;

    void apply(entt::entity self, const sol::object& obj);
    void applyX(entt::entity self, const sol::object& obj);
    void applyY(entt::entity self, const sol::object& obj);
};

struct TargetMargin {
    TargetBound t, b, l, r;

    void apply(entt::entity self, const sol::object& obj);
};

struct TargetOrigin {
    vec2f v = {0.0F, 0.0F};

    void apply(entt::entity self, const sol::object& obj);
};

struct TargetAspectRatio {
    f32 v = 0.0F;

    void apply(entt::entity self, const sol::object& obj);
};

struct TargetRelations {
    struct Dep {
        f32 val = 0;
        entt::entity e{};
        enum class Type : u8 { POS_X, POS_Y, SIZE_X, SIZE_Y } type{};
    };
    std::vector<Dep> deps{};

    void apply(entt::entity self, const sol::object& obj);
};
}  // namespace mle::ui::element::comp

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
            case Type::ROOT:
                return formatter<std::string>::format("r%", ctx);
            case Type::FLEX_SHARE:
                return formatter<std::string>::format("f(flex)", ctx);
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
