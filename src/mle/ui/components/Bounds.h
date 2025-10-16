#pragma once

#include "../Types.h"
#include "Types.h"
#include "mle/math/Types2D.h"

namespace mle::ui {
struct TargetBound {
    enum class Type : u8 {
        PX,          ///< Absolute value.                           Suffix: "px"
        FLEX_SHARE,  ///< Share available space flexibly.           Suffix: "f", "flex"
        PARENT,      ///< Relative to parent element.               Suffix: "%"
        ROOT,        ///< Relative to root element.                 Suffix: "%r"
        PARENT_W,    ///< Width relative to parent.                 Suffix: "%pw"
        PARENT_H,    ///< Height relative to parent.                Suffix: "%ph"
        SELF,        ///< Size based on self content.               Suffix: "%s", "fit"
        SELF_W,      ///< Width based on self content.              Suffix: "%sw"
        SELF_H,      ///< Height based on self content.             Suffix: "%sh"
        DEFAULT      ///< Default behavior.                         Suffix: ""
    };

    f32 val = 0;
    Type type = Type::DEFAULT;

    TargetBound() = default;

    void set(f32 v, Type t);
    void set(const sol::object& obj);
    void set(std::string_view str);
    [[nodiscard]] bool isFit() const { return type == Type::SELF || type == Type::SELF_W || type == Type::SELF_H; }
};

struct Dependency {
    f32 val = 0;
    entt::entity e = entt::null;

    void set(const Entt& e, const sol::object& obj);
};

namespace comp {

struct Bounds {
    Recti parent_px{};

    Bounds() = default;
    explicit Bounds(vec2u size) :
        parent_px({0, 0}, size) {}
};

struct TargetSize {
    TargetBound x, y;
    Dependency xdep, ydep;

    TargetSize() = default;
    explicit TargetSize(const Entt& e, const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);
    static void applyXDep(const Entt& e, const sol::object& obj);
    static void applyYDep(const Entt& e, const sol::object& obj);
};

struct TargetPosition {
    TargetBound x, y;
    Dependency xdep, ydep;

    TargetPosition() = default;
    explicit TargetPosition(const Entt& e, const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);
    static void applyXDep(const Entt& e, const sol::object& obj);
    static void applyYDep(const Entt& e, const sol::object& obj);
};

struct TargetPadding {
    TargetBound t, b, l, r;

    TargetPadding() = default;
    explicit TargetPadding(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyT(const Entt& e, const sol::object& obj);
    static void applyB(const Entt& e, const sol::object& obj);
    static void applyL(const Entt& e, const sol::object& obj);
    static void applyR(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);

    struct PaddingValuesPx {
        int t = 0, b = 0, l = 0, r = 0;
    };
    [[nodiscard]] PaddingValuesPx calcOnRect(const Recti& rect) const;
};

struct TargetMargin {
    TargetBound t, b, l, r;

    TargetMargin() = default;
    explicit TargetMargin(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyT(const Entt& e, const sol::object& obj);
    static void applyB(const Entt& e, const sol::object& obj);
    static void applyL(const Entt& e, const sol::object& obj);
    static void applyR(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);
};

struct TargetOrigin {
    vec2f o = {0.0F, 0.0F};

    TargetOrigin() = default;
    explicit TargetOrigin(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
};

struct TargetAspectRatio {
    f32 o = 0.0F;

    TargetAspectRatio() = default;
    explicit TargetAspectRatio(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
};

}  // namespace comp
}  // namespace mle::ui
