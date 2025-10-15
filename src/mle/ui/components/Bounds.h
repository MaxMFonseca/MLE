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
        SELF,        ///< Size based on self content.               Suffix: "%s"
        SELF_W,      ///< Width based on self content.              Suffix: "%sw"
        SELF_H,      ///< Height based on self content.             Suffix: "%sh"
        DEFAULT      ///< Default behavior.                         Suffix: ""
    };

    f32 val = 0.F;
    Type type = Type::DEFAULT;

    TargetBound() = default;

    void set(f32 v, Type t);
    void set(const sol::object& obj);
    void set(std::string_view str);
};

namespace comp {
struct Bounds {
    Recti parent_px{};
};

struct TargetSize {
    TargetBound x, y;

    explicit TargetSize(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);
};

struct TargetPosition {
    TargetBound x, y;

    explicit TargetPosition(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);
};

struct TargetPadding {
    TargetBound t, b, l, r;

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

    explicit TargetOrigin(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
};

struct TargetAspectRatio {
    f32 o = 0.0F;

    explicit TargetAspectRatio(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
};

struct TargetRelations {
    struct Dep {
        f32 val = 0;
        entt::entity e{};
        enum class Type : u8 { POS_X, POS_Y, SIZE_X, SIZE_Y } type{};
        // a:0.3:pos_x
    };
    std::vector<Dep> o{};

    explicit TargetRelations(const Entt& e, const sol::object& obj);
    void add(const Entt& e, const sol::object& obj);
    void add(const sol::object& obj, const comp::Container& parent);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyAdd(const Entt& e, const sol::object& obj);
};

}  // namespace comp
}  // namespace mle::ui
