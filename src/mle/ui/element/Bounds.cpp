#include "Bounds.h"

#include "mle/lua/Utils.h"
#include "mle/ui/element/Container.h"

namespace mle::ui::element::comp {
namespace {
TargetBound::Type stringToType(const std::string& str) {
    using Type = TargetBound::Type;
    if (str == "px") {
        return Type::PX;
    }
    if (str == "fit") {
        return Type::FIT;
    }
    if (str == "%") {
        return Type::PARENT;
    }
    if (str == "s") {
        return Type::SELF;
    }
    if (str == "sw") {
        return Type::SELF_W;
    }
    if (str == "sh") {
        return Type::SELF_H;
    }
    if (str == "r%") {
        return Type::ROOT;
    }
    if (str == "f" || str == "flex") {
        return Type::FLEX_SHARE;
    }
    if (str == "pw") {
        return Type::PARENT_W;
    }
    if (str == "ph") {
        return Type::PARENT_H;
    }
    return Type::DEFAULT;
}

f32 charToOrigin(char c) {
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
            MLE_W("Unexpected char in origin string: '{}'", c);
            return 0.0F;
    }
}

vec2f stringToOrigin(const std::string& s) {
    vec2f ret{};
    ret.x = charToOrigin(s[0]);
    if (s.size() > 1) {
        ret.y = charToOrigin(s[1]);
    } else {
        ret.y = ret.x;
    }
    return ret;
}
}  // namespace

void TargetBound::applyStr(const std::string& str) {
    auto split = splitNumberAndSuffix<f32>(str);
    MLE_ASSERT_LOG(split.has_value(), "Invalid target bound format: {}", str);
    val = split.value().first;
    if (val == 0.0F && str[0] != '0') {
        val = 1;
    }
    const auto& suffix = split.value().second;
    type = stringToType(suffix);
    if (typeIsPercentage(type)) {
        val /= 100.0F;
    }
}

void TargetBound::apply(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        val = obj.as<f32>();
        return;
    }
    if (obj.is<std::string>()) {
        applyStr(obj.as<std::string>());
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        auto val_r = table[1];
        MLE_ASSERT(lua::valid<f32>(val_r));
        val = val_r.get<f32>();
        auto type_r = table[2];
        if (type_r.valid() && type_r.is<std::string>()) {
            type = stringToType(type_r.get<std::string>());
        }
        if (typeIsPercentage(type)) {
            val /= 100.0F;
        }
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetBound: {}", obj.get_type());
}

void TargetPosition::apply(entt::entity self, const sol::object& obj) {
    if (obj.is<f32>()) {
        x.val = y.val = obj.as<f32>();
    } else if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        auto c0 = str[0];
        if (std::isdigit(c0) || c0 == '.') {
            x.applyStr(str);
            y.applyStr(str);
        } else {
            MLE_ASSERT_LOG(str.size() <= 2, "Invalid target position string: '{}'. Expected format is 'x(origin_val)' or 'xy(origin_val)'.", str);
            auto str_pos = stringToOrigin(str);
            x.val = str_pos.x;
            y.val = str_pos.y;
            y.type = x.type = TargetBound::Type::PARENT;
        }
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();

        auto x_r = table[1];
        if (x_r.valid()) {
            x.apply(x_r);
        } else {
            x_r = table["x"];
            if (x_r.valid()) {
                x.apply(x_r);
            }
        }

        auto y_r = table[2];
        if (x_r.valid()) {
            y.apply(y_r);
        } else {
            y_r = table["y"];
            if (y_r.valid()) {
                y.apply(y_r);
            }
        }
    }

    comp::Container::notifyChildChangedBounds(self);
}

void TargetPosition::applyX(entt::entity self, const sol::object& obj) {
    x.apply(obj);
    comp::Container::notifyChildChangedBounds(self);
}

void TargetPosition::applyY(entt::entity self, const sol::object& obj) {
    y.apply(obj);
    comp::Container::notifyChildChangedBounds(self);
}

void TargetSize::apply(entt::entity self, const sol::object& obj) {
    if (obj.is<f32>()) {
        x.val = y.val = obj.as<f32>();
    } else if (obj.is<std::string>()) {
        auto str = obj.as<std::string>();
        x.applyStr(str);
        y.applyStr(str);
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();

        auto x_r = table[1];
        if (x_r.valid()) {
            x.apply(x_r);
        } else {
            x_r = table["x"];
            if (x_r.valid()) {
                x.apply(x_r);
            }
        }

        auto y_r = table[2];
        if (x_r.valid()) {
            y.apply(y_r);
        } else {
            y_r = table["y"];
            if (y_r.valid()) {
                y.apply(y_r);
            }
        }
    }

    comp::Container::notifyChildChangedBounds(self);
}

void TargetSize::applyX(entt::entity self, const sol::object& obj) {
    x.apply(obj);
    comp::Container::notifyChildChangedBounds(self);
}

void TargetSize::applyY(entt::entity self, const sol::object& obj) {
    y.apply(obj);
    comp::Container::notifyChildChangedBounds(self);
}

void Padding::apply(entt::entity /*self*/, const sol::object& obj) {
    if (obj.is<f32>()) {
        f32 val = obj.as<f32>();
        t.val = b.val = l.val = r.val = val;
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();

        auto t_r = lua::getKeyOrIdx(table, "t", 1);
        auto b_r = lua::getKeyOrIdx(table, "b", 2);
        auto l_r = lua::getKeyOrIdx(table, "l", 3);
        auto r_r = lua::getKeyOrIdx(table, "r", 4);

        if (t_r) {
            t.apply(*t_r);
        }
        if (b_r) {
            b.apply(*b_r);
        }
        if (l_r) {
            l.apply(*l_r);
        }
        if (r_r) {
            r.apply(*r_r);
        }
    }

    // FIXME: Must notify somehow that padding has changed
}

std::array<int, 4> Padding::calcOnRect(Recti rect) const {
    auto width = as<f32>(rect.size.x);
    auto height = as<f32>(rect.size.y);

    std::array<int, 4> ret = {0, 0, 0, 0};

    if (l.val != 0.0F) {
        int val = 0;
        switch (l.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(l.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(l.val * width);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", l.type);
        }
        ret[as<usize>(BoxFace::L)] = val;
    }
    if (r.val != 0.0F) {
        int val = 0;
        switch (r.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(r.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(r.val * width);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", r.type);
        }
        ret[as<usize>(BoxFace::R)] = val;
    }
    if (t.val != 0.0F) {
        int val = 0;
        switch (t.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(t.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(t.val * height);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", t.type);
        }
        ret[as<usize>(BoxFace::T)] = val;
    }
    if (b.val != 0.0F) {
        int val = 0;
        switch (b.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(b.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(b.val * height);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", b.type);
        }
        ret[as<usize>(BoxFace::B)] = val;
    }
    return ret;
}

void TargetMargin::apply(entt::entity self, const sol::object& obj) {
    if (obj.is<f32>()) {
        f32 val = obj.as<f32>();
        t.val = b.val = l.val = r.val = val;
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();

        auto t_r = lua::getKeyOrIdx(table, "t", 1);
        auto b_r = lua::getKeyOrIdx(table, "b", 2);
        auto l_r = lua::getKeyOrIdx(table, "l", 3);
        auto r_r = lua::getKeyOrIdx(table, "r", 4);

        if (t_r) {
            t.apply(*t_r);
        }
        if (b_r) {
            b.apply(*b_r);
        }
        if (l_r) {
            l.apply(*l_r);
        }
        if (r_r) {
            r.apply(*r_r);
        }
    }

    comp::Container::notifyChildChangedBounds(self);
}

void TargetOrigin::apply(entt::entity self, const sol::object& obj) {
    if (obj.is<std::string>()) {
        v = stringToOrigin(obj.as<std::string>());
    } else {
        MLE_ASSERT(obj.is<sol::table>());

        if (const auto x_r = lua::getKeyOrIdx(obj.as<sol::table>(), "x", 1); x_r) {
            if (x_r->is<std::string>()) {
                v.x = charToOrigin(x_r->as<std::string>()[0]);
            } else if (x_r->is<f32>()) {
                v.x = x_r->as<f32>();
            } else {
                MLE_UNREACHABLE_LOG("Unexpected x type for Origin: {}", x_r->get_type());
            }
        }

        if (const auto y_r = lua::getKeyOrIdx(obj.as<sol::table>(), "y", 2); y_r) {
            if (y_r->is<std::string>()) {
                v.y = charToOrigin(y_r->as<std::string>()[0]);
            } else if (y_r->is<f32>()) {
                v.y = y_r->as<f32>();
            } else {
                MLE_UNREACHABLE_LOG("Unexpected y type for Origin: {}", y_r->get_type());
            }
        }
    }

    comp::Container::notifyChildChangedBounds(self);
}

void TargetRelations::apply(entt::entity self, const sol::object& obj) {
    auto& parent = comp::Parent::container(self);

    auto table = lua::as<sol::table>(obj);
    for (const auto& [_, v] : table) {
        auto str = lua::as<std::string>(v);
        auto ss = split(str, ':');

        if (ss.size() != 3) {
            MLE_W("Invalid relation format: '{}'. Expected format is 'type:target:offset'.", str);
            continue;
        }

        comp::TargetRelations::Dep dep;
        comp::TargetRelations::Dep dep2;
        bool duo_dep = false;

        if (ss[0] == "pos_x") {
            dep.type = comp::TargetRelations::Dep::Type::POS_X;
        } else if (ss[0] == "pos_y") {
            dep.type = comp::TargetRelations::Dep::Type::POS_Y;
        } else if (ss[0] == "size_x") {
            dep.type = comp::TargetRelations::Dep::Type::SIZE_X;
        } else if (ss[0] == "size_y") {
            dep.type = comp::TargetRelations::Dep::Type::SIZE_Y;
        } else if (ss[0] == "pos") {
            dep.type = comp::TargetRelations::Dep::Type::POS_X;
            dep2.type = comp::TargetRelations::Dep::Type::POS_Y;
            duo_dep = true;
        } else if (ss[0] == "size") {
            dep.type = comp::TargetRelations::Dep::Type::SIZE_X;
            dep2.type = comp::TargetRelations::Dep::Type::SIZE_Y;
            duo_dep = true;
        }

        dep2.e = dep.e = parent.getChild(ss[1]);
        if (dep.e == entt::null) {
            MLE_W("Invalid relation. Target {} not found on parent.", ss[1]);
            continue;
        }

        if (std::isdigit(ss[2][0]) || ss[2][0] == '.') {
            dep2.val = dep.val = std::stof(ss[2]);
        } else {
            auto v = stringToOrigin(ss[2]);
            dep.val = v.x;
            dep2.val = v.y;
        }

        deps.emplace_back(dep);
        if (duo_dep) {
            deps.emplace_back(dep2);
        }
    }

    Container::notifyChildChangedBounds(self);
}

void TargetAspectRatio::apply(entt::entity self, const sol::object& obj) {
    if (obj.is<f32>()) {
        v = obj.as<f32>();
    } else {
        MLE_UNREACHABLE_LOG("Unexpected obj type for TargetAspectRatio: {}", obj.get_type());
    }

    comp::Container::notifyChildChangedBounds(self);
}

}  // namespace mle::ui::element::comp
