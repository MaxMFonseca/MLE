#include "Base.h"

#include "mle/client/Client.h"
#include "mle/core/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "sol/forward.hpp"

namespace mle::ui::comp {
void Background::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<Background>([&](Background& bg) { bg.set(obj); });
}

void Table::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (!e.has<Table>()) {
        auto& nt = e.emplace<Table>();
        if (obj.is<sol::table>()) {
            nt.o = Client::i().lua().createTable(obj.as<sol::table>());
        } else if (obj.is<bool>() && obj.as<bool>()) {
            nt.o = Client::i().lua().createTable();
        } else {
            MLE_E("Invalid obj/bool type passed to Table apply. {}", obj.get_type());
        }
    } else {
        if (obj.is<sol::table>()) {
            auto& t = e.get<Table>();
            Client::i().lua().mergeTables(t.o, obj.as<sol::table>());
        } else {
            MLE_E("Invalid obj type passed to Table apply. {}", obj.get_type());
        }
    }
}

sol::object Table::get(const Entt& e, const sol::object& /*params*/) {
    if (!e.has<Table>()) {
        auto& nt = e.emplace<Table>();
        nt.o = Client::i().lua().createTable();
    }
    return e.get<Table>().o;
};

void OnUpdate::apply(const Entt& e, const sol::object& obj) {
    lua::assertIs<sol::function>(obj);
    e.patchOrEmplace<OnUpdate>([&](OnUpdate& ou) { ou.fn = obj.as<FuncType>(); });
};

void RenderScale::apply(const Entt& e, const sol::object& obj) {
    lua::assertIs<f32>(obj);
    e.patchOrEmplace<RenderScale>([&](RenderScale& rs) {
        rs.scale = obj.as<f32>();
        rs.scale = glm::max(rs.scale, 0.01F);
    });
};

void ID::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(lua::valid<std::string>(obj));
    e.patchOrEmplace<ID>([&](ID& id) { id.o = lua::as<std::string>(obj); });
};

void Layer::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(lua::valid<int>(obj));
    e.patchOrEmplace<Layer>([&](Layer& layer) { layer.layer = obj.as<int>(); });
};

void Functions::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    lua::assertIs<sol::table>(obj);

    e.patchOrEmplace<Functions>([&](Functions& funcs) {
        if (obj.is<sol::table>()) {
            auto table = obj.as<sol::table>();
            for (const auto& [key_r, value_r] : table) {
                lua::assertIs<std::string>(key_r);
                lua::assertIs<sol::function>(value_r);

                auto key = lua::as<std::string>(key_r);
                auto fn = value_r.as<sol::function>();
                funcs.fns[key] = fn;
            }
        }
    });
};

sol::object Functions::get(const Entt& e, const sol::object& params) {
    if (!e.has<Functions>()) {
        MLE_E("Tried to get Functions from entity {} which has no Functions component.", e.fullName());
        return {};
    }

    if (!lua::valid<std::string>(params)) {
        MLE_E("Tried to get Functions from entity {} without specifying valid function name.", e.fullName());
        return {};
    }

    const auto fn_name = lua::as<std::string>(params);
    const auto& c = e.get<Functions>();
    return c.get(fn_name);
}

[[nodiscard]] sol::function Functions::get(const std::string& name) const {
    auto it = fns.find(name);
    if (it != fns.end()) {
        return it->second;
    }
    MLE_W("Tried to get Function '{}' which does not exist.", name);
    return {};
}

void DisabledFlag::applyEnabled(const Entt& e, const sol::object& obj) {
    if (!lua::valid<bool>(obj)) {
        MLE_E("Invalid value passed to DisabledFlag apply enabled. Expected bool, got {}.", obj.get_type());
        return;
    }

    const auto enabled = lua::as<bool>(obj);

    if (enabled) {
        e.eraseChecked<DisabledFlag>();
    } else {
        e.addFlag<DisabledFlag>();
    }
    e.requestExternalBoundsUpdate();
};

void DisabledFlag::applyDisabled(const Entt& e, const sol::object& obj) {
    if (!lua::valid<bool>(obj)) {
        MLE_E("Invalid value passed to DisabledFlag apply disabled. Expected bool, got {}.", obj.get_type());
        return;
    }

    const auto disabled = lua::as<bool>(obj);

    if (disabled) {
        e.addFlag<DisabledFlag>();
    } else {
        e.eraseChecked<DisabledFlag>();
    }
    e.requestExternalBoundsUpdate();
};

void Background::set(const sol::object& obj) {
    if (lua::valid<Color>(obj)) {
        const auto c = lua::as<Color>(obj);
        lt = rt = lb = rb = c;
    } else if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();

        if (const auto lt_r = lua::getFirstKey(table, "lt", 1); lt_r.valid()) {
            lt = lua::as<Color>(lt_r);
        }
        if (const auto rt_r = lua::getFirstKey(table, "rt", 2); rt_r.valid()) {
            rt = lua::as<Color>(rt_r);
        }
        if (const auto lb_r = lua::getFirstKey(table, "lb", 3); lb_r.valid()) {
            lb = lua::as<Color>(lb_r);
        }
        if (const auto rb_r = lua::getFirstKey(table, "rb", 4); rb_r.valid()) {
            rb = lua::as<Color>(rb_r);
        }
    } else {
        MLE_E("Invalid obj type passed to Background set. {}", obj.get_type());
    }
};

void ForceFitFlag::apply(const Entt& e, const sol::object& obj) {
    if (!lua::valid<bool>(obj)) {
        MLE_E("Invalid value passed to ForceFitFlag apply. Expected bool, got {}.", obj.get_type());
        return;
    }

    const auto enabled = lua::as<bool>(obj);

    if (enabled) {
        e.addFlag<ForceFitFlag>();
    } else {
        e.eraseChecked<ForceFitFlag>();
    }
};
}  // namespace mle::ui::comp
