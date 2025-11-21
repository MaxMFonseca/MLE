#include "Bounds.h"

#include "../Entt.h"
#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/lua/Utils.h"
#include "mle/ui/UI.h"
#include "mle/utils/String.h"

namespace mle::ui {
namespace {
TargetBound::Type stringToType(std::string_view str) {
    using Type = TargetBound::Type;
    if (str == "px") {
        return Type::PX;
    }
    if (str == "f" || str == "flex") {
        return Type::FLEX_SHARE;
    }
    if (str == "fit") {
        return Type::FIT;
    }
    if (str == "%") {
        return Type::RELATIVE;
    }
    if (str == "%r") {
        return Type::ROOT;
    }
    if (str == "%w") {
        return Type::RELATIVE_W;
    }
    if (str == "%h") {
        return Type::RELATIVE_H;
    }
    return Type::DEFAULT;
}

f32 charToSize(char c) {
    switch (c) {
        case 'l':
        case 't':
            return 0.0F;
        case 'r':
        case 'b':
            return 1.0F;
        case 'x':
        case 'c':
            return 0.5F;
        default:
            MLE_E("Unexpected char in origin string: '{}', returning 0", c);
            return 0;
    }
}

vec2f strToSize(std::string_view s) {
    MLE_ASSERT_LOG(!s.empty() && s.size() <= 2, "Cannot convert string to size: '{}'", s);
    vec2f ret{};
    ret.x = charToSize(s[0]);
    ret.y = (s.size() > 1) ? charToSize(s[1]) : ret.x;
    return ret;
}
}  // namespace

void TargetBound::set(f32 v, Type t) {
    val = v;
    type = t;

    bool type_is_percent = (t == Type::RELATIVE || t == Type::RELATIVE_W || t == Type::RELATIVE_H || t == Type::ROOT);
    if (type_is_percent) {
        val /= 100.0F;
    }
}

void TargetBound::set(std::string_view str) {
    if (str.empty()) {
        val = 0.0F;
        type = Type::DEFAULT;
        return;
    }
    if (str.size() == 1) {
        if (std::isdigit(str[0])) {
            val = static_cast<f32>(str[0] - '0');
            type = Type::DEFAULT;
            return;
        }
        val = charToSize(str[0]);
        type = Type::RELATIVE;
        return;
    }

    auto [num, suffix] = splitNumSuffix(str);

    if (suffix.empty()) {
        set(num, Type::DEFAULT);
        return;
    }

    bool is_percent_type = suffix[0] == '%';

    if (num == 0 && suffix[0] != '0') {
        num = is_percent_type ? 100 : 1;
        return;
    }

    set(num, stringToType(suffix));
}

void TargetBound::set(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        val = obj.as<f32>();
        type = Type::DEFAULT;
        return;
    }
    if (obj.is<std::string>()) {
        set(obj.as<std::string>());
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        f32 num = lua::getIdx<f32>(table, 1);
        auto type_r = table[2];
        auto type = TargetBound::Type::DEFAULT;
        if (type_r.valid()) {
            type = stringToType(lua::as<std::string>(type_r));
        }
        set(num, type);
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetBound: {}", obj.get_type());
}

void Dependency::set(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    this->e = entt::null;
    dep_tb = {};

    Entt parent{e.ui(), e.getParent()};

    if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        auto splited = split(str, ':');
        std::string dep_name{splited[0]};
        auto& parent_rel = parent.getRelationship();
        this->e = parent_rel.getChildByName(parent, dep_name);
        if (this->e == entt::null) {
            MLE_E("Dependency target '{}' not found in parent container of entity {}", dep_name, e.e());
            return;
        }
        if (splited.size() < 2) {
            dep_tb.set(1);
        } else {
            dep_tb.set(splited[1]);
        }
    } else if (obj.is<sol::table>()) {
        auto table = lua::as<sol::table>(obj);
        auto name_r = lua::tryAs<std::string>(table[1]);
        sol::object val_r = table[2];
        if (!name_r) {
            MLE_E("Invalid dependency table for entt {}. Expected format is {{dep_name<, val>}}.", e.name());
            return;
        }
        auto dep_name = *name_r;
        auto& parent_rel = parent.getRelationship();
        this->e = parent_rel.getChildByName(parent, dep_name);
        if (this->e == entt::null) {
            MLE_E("Dependency target '{}' not found in parent container of entity {}", dep_name, e.e());
            return;
        }
        if (val_r.valid()) {
            dep_tb.set(val_r);
        } else {
            dep_tb.set(1);
        }
    } else {
        MLE_UNREACHABLE_LOG("Unexpected obj type for Dependency: {}", obj.get_type());
    }
}

namespace comp {
TargetSize::TargetSize(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        x.val = y.val = obj.as<f32>();
    } else if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        auto c0 = str[0];
        if (std::isdigit(c0) || c0 == '.' || c0 == '-' || c0 == '+') {
            x.set(str);
            y = x;
        } else {
            MLE_ASSERT_LOG(str.size() <= 2, "Invalid target position string: '{}'. Expected format is 'x(size_val)' or 'xy(size_val)'.", str);
            auto str_pos = strToSize(str);
            x.val = str_pos.x;
            y.val = str_pos.y;
            y.type = x.type = TargetBound::Type::RELATIVE;
        }
    } else {
        auto table = lua::as<sol::table>(obj);
        auto x_r = lua::getFirstKey(table, "x", 1);
        if (x_r.valid()) {
            x.set(x_r);
        }
        auto y_r = lua::getFirstKey(table, "y", 2);
        if (y_r.valid()) {
            y.set(y_r);
        }
        auto dep_x_r = lua::getFirstKey(table, "xrel", "xdep", 3);
        if (dep_x_r.valid()) {
            xdep.set(e, dep_x_r);
        }
        auto dep_y_r = lua::getFirstKey(table, "yrel", "ydep", 4);
        if (dep_y_r.valid()) {
            ydep.set(e, dep_y_r);
        }
    }
}

void TargetSize::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetSize>(e, obj);
}

void TargetSize::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetSize>([&](TargetSize& ts) { ts.x.set(obj); });
}

void TargetSize::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetSize>([&](TargetSize& ts) { ts.y.set(obj); });
}

void TargetSize::applyXDep(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetSize>([&](TargetSize& ts) { ts.xdep.set(e, obj); });
}

void TargetSize::applyYDep(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetSize>([&](TargetSize& ts) { ts.ydep.set(e, obj); });
}

TargetPosition::TargetPosition(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        x.set(obj);
        y = x;
    } else if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        auto c0 = str[0];
        if (std::isdigit(c0) || c0 == '.' || c0 == '-' || c0 == '+') {
            x.set(str);
            y = x;
        } else {
            MLE_ASSERT_LOG(str.size() <= 2, "Invalid target position string: '{}'. Expected format is 'x(pos_val)' or 'xy(pos_val)'.", str);
            auto str_pos = strToSize(str);
            x.val = str_pos.x;
            y.val = str_pos.y;
            y.type = x.type = TargetBound::Type::RELATIVE;
        }
    } else {
        auto table = lua::as<sol::table>(obj);
        auto x_r = lua::getFirstKey(table, "x", 1);
        if (x_r.valid()) {
            x.set(x_r);
        }
        auto y_r = lua::getFirstKey(table, "y", 2);
        if (y_r.valid()) {
            y.set(y_r);
        }
        auto dep_x_r = lua::getFirstKey(table, "xrel", "xdep", 3);
        if (dep_x_r.valid()) {
            xdep.set(e, dep_x_r);
        }
        auto dep_y_r = lua::getFirstKey(table, "yrel", "ydep", 4);
        if (dep_y_r.valid()) {
            ydep.set(e, dep_y_r);
        }
    }
}

void TargetPosition::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetPosition>(e, obj);
}

void TargetPosition::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPosition>([&](TargetPosition& tp) { tp.x.set(obj); });
}

void TargetPosition::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPosition>([&](TargetPosition& tp) { tp.y.set(obj); });
}

void TargetPosition::applyXDep(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPosition>([&](TargetPosition& tp) { tp.xdep.set(e, obj); });
}

void TargetPosition::applyYDep(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPosition>([&](TargetPosition& tp) { tp.ydep.set(e, obj); });
}

TargetPadding::TargetPadding(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        t.val = b.val = l.val = r.val = obj.as<f32>();
        return;
    }
    if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        t.set(str);
        b = l = r = t;
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        auto t_r = lua::getFirstKey(table, "t", 1);
        if (t_r.valid()) {
            t.set(t_r);
        }
        auto b_r = lua::getFirstKey(table, "b", 2);
        if (b_r.valid()) {
            b.set(b_r);
        }
        auto l_r = lua::getFirstKey(table, "l", 3);
        if (l_r.valid()) {
            l.set(l_r);
        }
        auto r_r = lua::getFirstKey(table, "r", 4);
        if (r_r.valid()) {
            r.set(r_r);
        }
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetPadding: {}", obj.get_type());
}

void TargetPadding::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetPadding>(obj);
}

void TargetPadding::applyT(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPadding>([&](TargetPadding& tp) { tp.t.set(obj); });
}

void TargetPadding::applyB(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPadding>([&](TargetPadding& tp) { tp.b.set(obj); });
}

void TargetPadding::applyL(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPadding>([&](TargetPadding& tp) { tp.l.set(obj); });
}

void TargetPadding::applyR(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPadding>([&](TargetPadding& tp) { tp.r.set(obj); });
}

void TargetPadding::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPadding>([&](TargetPadding& tp) {
        tp.l.set(obj);
        tp.r.set(obj);
    });
}

void TargetPadding::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPadding>([&](TargetPadding& tp) {
        tp.t.set(obj);
        tp.b.set(obj);
    });
}

PaddingPx TargetPadding::calc(const UI& ui, vec2u size) const {
    PaddingPx ret;

    auto root_size_f = vec2f{ui.getRootMaxSize()};
    auto f_size = vec2f{size};

    auto calc_fn = [&](const TargetBound& p, bool is_x) {
        switch (p.type) {
            case TargetBound::Type::PX: {
                return p.val;
            } break;
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::RELATIVE: {
                if (is_x) {
                    return f_size.x * p.val;
                }
                return f_size.y * p.val;
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return f_size.x * p.val;
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return f_size.y * p.val;
            } break;
            case TargetBound::Type::ROOT: {
                if (is_x) {
                    return root_size_f.x * p.val;
                }
                return root_size_f.y * p.val;
            } break;
            default: {
                // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                MLE_W("Invalid TargetBound type for padding, treating as 0 px");
                return 0.0F;
            } break;
        }
    };

    ret.t = as<int>(calc_fn(t, false));
    ret.b = as<int>(calc_fn(b, false));
    ret.l = as<int>(calc_fn(l, true));
    ret.r = as<int>(calc_fn(r, true));

    return ret;
}

TargetMargin::TargetMargin(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        t.val = b.val = l.val = r.val = obj.as<f32>();
        return;
    }
    if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        t.set(str);
        b = l = r = t;
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        auto t_r = lua::getFirstKey(table, "t", 1);
        if (t_r.valid()) {
            t.set(t_r);
        }
        auto b_r = lua::getFirstKey(table, "b", 2);
        if (b_r.valid()) {
            b.set(b_r);
        }
        auto l_r = lua::getFirstKey(table, "l", 3);
        if (l_r.valid()) {
            l.set(l_r);
        }
        auto r_r = lua::getFirstKey(table, "r", 4);
        if (r_r.valid()) {
            r.set(r_r);
        }

        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetMargin: {}", obj.get_type());
}

void TargetMargin::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetMargin>(obj);
}

void TargetMargin::applyT(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetMargin>([&](TargetMargin& tm) { tm.t.set(obj); });
}

void TargetMargin::applyB(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetMargin>([&](TargetMargin& tm) { tm.b.set(obj); });
}

void TargetMargin::applyL(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetMargin>([&](TargetMargin& tm) { tm.l.set(obj); });
}

void TargetMargin::applyR(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetMargin>([&](TargetMargin& tm) { tm.r.set(obj); });
}

void TargetMargin::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetMargin>([&](TargetMargin& tm) {
        tm.l.set(obj);
        tm.r.set(obj);
    });
}

void TargetMargin::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetMargin>([&](TargetMargin& tm) {
        tm.t.set(obj);
        tm.b.set(obj);
    });
}

TargetOrigin::TargetOrigin(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        o.x = o.y = obj.as<f32>();
        return;
    }
    if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        MLE_ASSERT(!str.empty());
        o = strToSize(str);
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        auto x_r = lua::getFirstKey(table, "x", 1);
        if (x_r.valid()) {
            o.x = lua::as<f32>(x_r);
        }
        auto y_r = lua::getFirstKey(table, "y", 2);
        if (y_r.valid()) {
            o.y = lua::as<f32>(y_r);
        }
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetOrigin: {}", obj.get_type());
}

void TargetOrigin::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetOrigin>(obj);
}

TargetAspectRatio::TargetAspectRatio(const sol::object& obj) :
    o(lua::as<f32>(obj)) {
}

void TargetAspectRatio::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetAspectRatio>(obj);
}

void TargetBorder::set(const Entt& ew, const sol::object& obj) {
    auto table = lua::as<sol::table>(obj);

    if (const auto thickness_r = table["thickness"]; thickness_r.valid()) {
        setThickness(thickness_r);
    }
    if (const auto color_r = table["color"]; color_r.valid()) {
        setColor(ew, color_r);
    }
    if (const auto round_r = table["roundness"]; round_r.valid()) {
        setRound(round_r);
    }
}

void TargetBorder::setThickness(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>() || obj.is<std::string>()) {
        r.set(obj);
        t = b = l = r;
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto t_r = lua::getFirstKey(table, "t", 1); t_r.valid()) {
            t.set(t_r);
        }
        if (const auto b_r = lua::getFirstKey(table, "b", 2); b_r.valid()) {
            b.set(b_r);
        }
        if (const auto l_r = lua::getFirstKey(table, "l", 3); l_r.valid()) {
            l.set(l_r);
        }
        if (const auto r_r = lua::getFirstKey(table, "r", 4); r_r.valid()) {
            r.set(r_r);
        }
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetBorder thickness: {}", obj.get_type());
}

void TargetBorder::setRound(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>() || obj.is<std::string>()) {
        round_rb.set(obj);
        round_lt = round_rt = round_lb = round_rb;
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto lt_r = lua::getFirstKey(table, "lt", 1); lt_r.valid()) {
            round_lt.set(lt_r);
        }
        if (const auto rt_r = lua::getFirstKey(table, "rt", 2); rt_r.valid()) {
            round_rt.set(rt_r);
        }
        if (const auto lb_r = lua::getFirstKey(table, "lb", 3); lb_r.valid()) {
            round_lb.set(lb_r);
        }
        if (const auto rb_r = lua::getFirstKey(table, "rb", 4); rb_r.valid()) {
            round_rb.set(rb_r);
        }
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetBorder round: {}", obj.get_type());
}

void TargetBorder::setColor(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    ew.patchOrEmplace<Border>([&](Border& b) { b.color = Color{obj}.toLinear(); });
}

void TargetBorder::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.set(e, obj); });
}

void TargetBorder::applyThickness(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.setThickness(obj); });
}

void TargetBorder::applyColor(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    setColor(e, obj);
}

void TargetBorder::applyRound(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.setRound(obj); });
}

void TargetBorder::applyT(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.t.set(obj); });
}

void TargetBorder::applyB(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.b.set(obj); });
}

void TargetBorder::applyL(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.l.set(obj); });
}

void TargetBorder::applyR(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.r.set(obj); });
}

void TargetBorder::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) {
        tb.l.set(obj);
        tb.r.set(obj);
    });
}

void TargetBorder::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) {
        tb.t.set(obj);
        tb.b.set(obj);
    });
}

void TargetBorder::applyRoundLT(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.round_lt.set(obj); });
}

void TargetBorder::applyRoundRT(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.round_rt.set(obj); });
}

void TargetBorder::applyRoundLB(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.round_lb.set(obj); });
}

void TargetBorder::applyRoundRB(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetBorder>([&](TargetBorder& tb) { tb.round_rb.set(obj); });
}

Recti Bounds::onRoot(const Entt& ew) const {
    if (ew.isRoot()) {
        return parent_px;
    }

    vec2i offset = {0, 0};

    Entt parent = ew.parentEw();
    while (!parent.isRoot()) {
        auto& parent_bounds = parent.get<Bounds>();
        offset += parent_bounds.parent_px.pos();
        parent.setE(parent.getParent());
    }

    return Recti{parent_px.pos() + offset, parent_px.size()};
};

[[nodiscard]] Rectf Bounds::onRootNormalized(const Entt& ew) const {
    auto on_root_px = onRoot(ew);
    auto parent_size = ew.derive(ew.ui().getRoot()).get<comp::Bounds>().parent_px.size();

    return Rectf{
        vec2f{on_root_px.pos()} / vec2f{parent_size},
        vec2f{on_root_px.size()} / vec2f{parent_size},
    };
};
}  // namespace comp
}  // namespace mle::ui
