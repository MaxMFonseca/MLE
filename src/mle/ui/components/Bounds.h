#pragma once

#include "../Types.h"
#include "mle/core/Logger.h"
#include "mle/math/Types2D.h"
#include "mle/utils/Color.h"

namespace mle::ui {
struct TargetBound {
    enum class Type : u8 {
        PX,          ///< Absolute value.                           Suffix: "px"
        FLEX_SHARE,  ///< Share available space flexibly.           Suffix: "f", "flex"
        FIT,         ///< Fit to content size.                      Suffix: "fit"
        ROOT,        ///< Relative to root.                         Suffix: "%r"
        RELATIVE,    ///< Relative.                                 Suffix: "%"
        RELATIVE_W,  ///< Relative width.                           Suffix: "%w"
        RELATIVE_H,  ///< Relative height.                          Suffix: "%h"
        DEFAULT      ///< Default behavior.                         Suffix: ""
    };

    f32 val = 0;
    Type type = Type::DEFAULT;

    TargetBound() = default;

    void set(f32 v, Type t);
    void set(const sol::object& obj);
    void set(std::string_view str);
};

struct Dependency {
    TargetBound dep_tb{};
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

struct Border {
    int round_lt = 0, round_rt = 0, round_lb = 0, round_rb = 0;
    int t = 0, b = 0, l = 0, r = 0;
    Color color = Color::ZERO;
};

struct SizeProvider {
    using Fn = std::move_only_function<vec2u(const Entt& e, vec2u max_size)>;

    vec2u call(const Entt& e, vec2u max_size) {
        if (!done()) {
            size_ = fn_(e, max_size);
        }
        return size_;
    }

    void reset() { size_ = vec2u{0, 0}; }
    bool done() { return size_ != vec2u{0, 0}; }

    explicit SizeProvider(Fn fn) :
        fn_(std::move(fn)) {}

  private:
    vec2u size_{};
    mutable Fn fn_;
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

    [[nodiscard]] PaddingPx calc(const UI& ui, vec2u size) const;
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

struct TargetBorder {
    TargetBound round_lt, round_rt, round_lb, round_rb;
    TargetBound t, b, l, r;
    Color color = Color::ZERO;

    TargetBorder() = default;

    void set(const sol::object& obj);

    void setColor(const sol::object& obj);
    void setThickness(const sol::object& obj);
    void setRound(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
    static void applyThickness(const Entt& e, const sol::object& obj);
    static void applyColor(const Entt& e, const sol::object& obj);
    static void applyRound(const Entt& e, const sol::object& obj);
    static void applyT(const Entt& e, const sol::object& obj);
    static void applyB(const Entt& e, const sol::object& obj);
    static void applyL(const Entt& e, const sol::object& obj);
    static void applyR(const Entt& e, const sol::object& obj);
    static void applyX(const Entt& e, const sol::object& obj);
    static void applyY(const Entt& e, const sol::object& obj);
    static void applyRoundLT(const Entt& e, const sol::object& obj);
    static void applyRoundRT(const Entt& e, const sol::object& obj);
    static void applyRoundLB(const Entt& e, const sol::object& obj);
    static void applyRoundRB(const Entt& e, const sol::object& obj);
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

namespace fmt {
template <>
struct formatter<mle::ui::TargetBound::Type> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::TargetBound::Type& v, FormatContext& ctx) const {
        switch (v) {
            case mle::ui::TargetBound::Type::PX:
                return format_to(ctx.out(), "PX(px)");
            case mle::ui::TargetBound::Type::FLEX_SHARE:
                return format_to(ctx.out(), "FLEX_SHARE(flex)");
            case mle::ui::TargetBound::Type::ROOT:
                return format_to(ctx.out(), "ROOT(%r)");
            case mle::ui::TargetBound::Type::FIT:
                return format_to(ctx.out(), "SELF(fit)");
            case mle::ui::TargetBound::Type::RELATIVE:
                return format_to(ctx.out(), "RELATIVE(%)");
            case mle::ui::TargetBound::Type::RELATIVE_W:
                return format_to(ctx.out(), "RELATIVE_W(%w)");
            case mle::ui::TargetBound::Type::RELATIVE_H:
                return format_to(ctx.out(), "RELATIVE_H(%h)");
            case mle::ui::TargetBound::Type::DEFAULT:
                return format_to(ctx.out(), "DEFAULT()");
        }
    }
};

template <>
struct formatter<mle::ui::TargetBound> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::TargetBound& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}]", v.val, v.type);
    }
};

template <>
struct formatter<mle::ui::Dependency> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Dependency& v, FormatContext& ctx) const {
        if (v.e == entt::null) {
            return format_to(ctx.out(), "-");
        }
        return format_to(ctx.out(), "[val: {}, e: {}]", v.dep_tb, v.e);
    }
};

template <>
struct formatter<mle::ui::comp::Bounds> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::Bounds& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos: {}, size: {}", v.parent_px.pos(), v.parent_px.size());
    }
};

template <>
struct formatter<mle::ui::comp::Border> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::Border& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[t: {}, b: {}, l: {}, r: {}, color: {}, rounds: lt:{}, rt:{}, lb:{}, rb:{}]", v.t, v.b, v.l, v.r, v.color, v.round_lt,
                         v.round_rt, v.round_lb, v.round_rb);
    }
};

template <>
struct formatter<mle::ui::comp::TargetSize> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetSize& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[x: {}, y: {}, xdep: {}, ydep: {}]", v.x, v.y, v.xdep, v.ydep);
    }
};

template <>
struct formatter<mle::ui::comp::TargetPosition> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetPosition& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[x: {}, y: {}, xdep: {}, ydep: {}]", v.x, v.y, v.xdep, v.ydep);
    }
};

template <>
struct formatter<mle::ui::comp::TargetPadding> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetPadding& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[t: {}, b: {}, l: {}, r: {}]", v.t, v.b, v.l, v.r);
    }
};

template <>
struct formatter<mle::ui::comp::TargetMargin> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetMargin& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[t: {}, b: {}, l: {}, r: {}]", v.t, v.b, v.l, v.r);
    }
};

template <>
struct formatter<mle::ui::comp::TargetBorder> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetBorder& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[t: {}, b: {}, l: {}, r: {}, color: {}, rounds: lt:{}, rt:{}, lb:{}, rb:{}]", v.t, v.b, v.l, v.r, v.color, v.round_lt,
                         v.round_rt, v.round_lb, v.round_rb);
    }
};

template <>
struct formatter<mle::ui::comp::TargetOrigin> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetOrigin& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[o: {}]", v.o);
    }
};

template <>
struct formatter<mle::ui::comp::TargetAspectRatio> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::comp::TargetAspectRatio& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[o: {}]", v.o);
    }
};

}  // namespace fmt
