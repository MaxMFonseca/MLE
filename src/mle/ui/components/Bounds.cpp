#include "Bounds.h"

#include "../Entt.h"
#include "Base.h"
#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/lua/Utils.h"
#include "mle/ui/UI.h"
#include "mle/utils/ECS.h"
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
    if (str == "%") {
        return Type::PARENT;
    }
    if (str == "%r") {
        return Type::ROOT;
    }
    if (str == "%pw") {
        return Type::PARENT_W;
    }
    if (str == "%ph") {
        return Type::PARENT_H;
    }
    if (str == "%s") {
        return Type::SELF;
    }
    if (str == "%sw") {
        return Type::SELF_W;
    }
    if (str == "%sh") {
        return Type::SELF_H;
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

    bool type_is_percent =
        (t == Type::PARENT || t == Type::ROOT || t == Type::PARENT_W || t == Type::PARENT_H || t == Type::SELF || t == Type::SELF_W || t == Type::SELF_H);
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
        val = charToSize(str[0]);
        type = Type::PARENT;
        return;
    }

    auto [num, suffix] = splitNumSuffix(str);

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
namespace comp {
TargetSize::TargetSize(const sol::object& obj) {
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
            y.type = x.type = TargetBound::Type::PARENT;
        }
    } else {
        auto table = lua::as<sol::table>(obj);
        auto x_r = lua::tryGetKeyOrIdx(table, "x", 1);
        if (x_r) {
            x.set(*x_r);
        }
        auto y_r = lua::tryGetKeyOrIdx(table, "y", 2);
        if (y_r) {
            y.set(*y_r);
        }
    }
}

void TargetSize::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetSize>(obj);
}

void TargetSize::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetSize>([&](TargetSize& ts) { ts.x.set(obj); });
}

void TargetSize::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetSize>([&](TargetSize& ts) { ts.y.set(obj); });
}

TargetPosition::TargetPosition(const sol::object& obj) {
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
            MLE_ASSERT_LOG(str.size() <= 2, "Invalid target position string: '{}'. Expected format is 'x(pos_val)' or 'xy(pos_val)'.", str);
            auto str_pos = strToSize(str);
            x.val = str_pos.x;
            y.val = str_pos.y;
            y.type = x.type = TargetBound::Type::PARENT;
        }
    } else {
        auto table = lua::as<sol::table>(obj);
        auto x_r = lua::tryGetKeyOrIdx(table, "x", 1);
        if (x_r) {
            x.set(*x_r);
        }
        auto y_r = lua::tryGetKeyOrIdx(table, "y", 2);
        if (y_r) {
            y.set(*y_r);
        }
    }
}

void TargetPosition::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetPosition>(obj);
}

void TargetPosition::applyX(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPosition>([&](TargetPosition& tp) { tp.x.set(obj); });
}

void TargetPosition::applyY(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetPosition>([&](TargetPosition& tp) { tp.y.set(obj); });
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
        auto t_r = lua::tryGetKeyOrIdx(table, "t", 1);
        if (t_r) {
            t.set(*t_r);
        }
        auto b_r = lua::tryGetKeyOrIdx(table, "b", 2);
        if (b_r) {
            b.set(*b_r);
        }
        auto l_r = lua::tryGetKeyOrIdx(table, "l", 3);
        if (l_r) {
            l.set(*l_r);
        }
        auto r_r = lua::tryGetKeyOrIdx(table, "r", 4);
        if (r_r) {
            r.set(*r_r);
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
        auto t_r = lua::tryGetKeyOrIdx(table, "t", 1);
        if (t_r) {
            t.set(*t_r);
        }
        auto b_r = lua::tryGetKeyOrIdx(table, "b", 2);
        if (b_r) {
            b.set(*b_r);
        }
        auto l_r = lua::tryGetKeyOrIdx(table, "l", 3);
        if (l_r) {
            l.set(*l_r);
        }
        auto r_r = lua::tryGetKeyOrIdx(table, "r", 4);
        if (r_r) {
            r.set(*r_r);
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
        auto x_r = lua::tryGetKeyOrIdx(table, "x", 1);
        if (x_r) {
            o.x = lua::as<f32>(*x_r);
        }
        auto y_r = lua::tryGetKeyOrIdx(table, "y", 2);
        if (y_r) {
            o.y = lua::as<f32>(*y_r);
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

TargetRelations::TargetRelations(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    const auto& parent_container = e.getParentContainer();

    if (obj.is<std::string>()) {
        add(obj, parent_container);
        return;
    }
    if (obj.is<sol::table>()) {
        for (const auto& v : obj.as<sol::table>()) {
            add(v.second, parent_container);
        }
        return;
    }

    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetRelations: {}", obj.get_type());
}

void TargetRelations::add(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    add(obj, e.getParentContainer());

    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetRelations entry: {}", obj.get_type());
}

void TargetRelations::add(const sol::object& obj, const comp::Container& parent_container) {
    // obj is str in format name:type:val
    auto str = lua::as<std::string>(obj);
    auto s = split(str, ':');
    if (s.size() != 3) {
        MLE_E("Invalid TargetRelations string: '{}'. Expected format is 'name:type:val'.", str);
        return;
    }
    entt::entity rel_e = parent_container.o.getEFromName(std::string(s.at(0)));
    if (rel_e == entt::null) {
        MLE_E("Invalid TargetRelations string: '{}'. No such sibling entity: '{}'.", str, s.at(0));
        return;
    }

    vec2f val{0.0F, 0.0F};
    if (std::isdigit(s.at(2)[0]) || s.at(2)[0] == '-' || s.at(2)[0] == '+' || s.at(2)[0] == '.') {
        auto val_r = strTo<f32>(s.at(2));
        if (val_r) {
            val.x = *val_r;
        }
    } else {
        if (s.at(2).empty() || s.at(2).size() > 2) {
            MLE_E("Invalid target position string: '{}'. {} Expected format is 'x(pos_val)' or 'xy(pos_val)'.", str, s.at(2));
            return;
        }
        val = strToSize(s.at(2));
    }

    auto type_str = s.at(1);
    if (type_str == "pos_x") {
        o.emplace_back(val.x, rel_e, Dep::Type::POS_X);
    } else if (type_str == "pos_y") {
        o.emplace_back(val.x, rel_e, Dep::Type::POS_Y);
    } else if (type_str == "size_x") {
        o.emplace_back(val.x, rel_e, Dep::Type::SIZE_X);
    } else if (type_str == "size_y") {
        o.emplace_back(val.x, rel_e, Dep::Type::SIZE_Y);
    } else if (type_str == "pos") {
        o.emplace_back(val.x, rel_e, Dep::Type::POS_X);
        o.emplace_back(val.y, rel_e, Dep::Type::POS_Y);
    } else if (type_str == "size") {
        o.emplace_back(val.x, rel_e, Dep::Type::SIZE_X);
        o.emplace_back(val.y, rel_e, Dep::Type::SIZE_Y);
    } else {
        MLE_E("Invalid TargetRelations string: '{}'. Unknown relation type: '{}'.", str, type_str);
        return;
    }
}

void TargetRelations::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.emplaceOrReplace<TargetRelations>(e, obj);
}

void TargetRelations::applyAdd(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<TargetRelations>([&](TargetRelations& tr) { tr.add(e, obj); });
}

}  // namespace comp
}  // namespace mle::ui
