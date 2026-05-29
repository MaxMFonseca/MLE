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

void OnCreate::apply(const Entt& e, const sol::object& obj) {
    lua::assertIs<sol::function>(obj);
    e.patchOrEmplace<OnCreate>([&](OnCreate& oc) { oc.fn = obj.as<FuncType>(); });
};

void OnDestroy::apply(const Entt& e, const sol::object& obj) {
    lua::assertIs<sol::function>(obj);
    e.patchOrEmplace<OnDestroy>([&](OnDestroy& od) { od.fn = obj.as<FuncType>(); });
};

void ListenEvents::clear() {
    for (auto& listener : listeners) {
        listener->unlisten();
    }
    listeners.clear();
}

void ListenEvents::addListener(const Entt& e, const std::string& event_name, sol::function fn) {
    if (event_name.empty()) {
        MLE_E("Cannot listen to an empty event name on entity {}.", e.fullName());
        return;
    }
    if (!fn.valid()) {
        MLE_E("Cannot listen to event '{}' on entity {} with invalid callback.", event_name, e.fullName());
        return;
    }

    UI* ui = &e.ui();
    entt::entity entity = e.e();
    auto listener = std::make_shared<system::EventListener>(ui->eventSystem());
    listener->setTargetEvent(event_name)
        .setCallback([ui, entity, fn = std::move(fn)](const sol::object& obj) mutable {
            if (!ui->getRegistry().valid(entity)) {
                return;
            }
            Entt ew{*ui, entity};
            if (ew.has<DisabledFlag>()) {
                return;
            }
            fn(ew, obj);
        })
        .listen();
    listeners.emplace_back(std::move(listener));
}

void ListenEvents::set(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    clear();

    if (!lua::valid<sol::table>(obj)) {
        MLE_E("Invalid listen value for entity {}. Expected table.", e.fullName());
        return;
    }

    auto table = lua::as<sol::table>(obj);
    for (const auto& [key_r, value_r] : table) {
        if (key_r.is<std::string>() && lua::valid<sol::function>(value_r)) {
            addListener(e, lua::as<std::string>(key_r), lua::as<sol::function>(value_r));
            continue;
        }

        if (lua::valid<sol::table>(value_r)) {
            auto listener_table = lua::as<sol::table>(value_r);
            const auto event_r = lua::getFirstKey(listener_table, "event", "name", 1);
            const auto fn_r = lua::getFirstKey(listener_table, "fn", "callback", 2);
            if (lua::valid<std::string>(event_r) && lua::valid<sol::function>(fn_r)) {
                addListener(e, lua::as<std::string>(event_r), lua::as<sol::function>(fn_r));
                continue;
            }
        }

        MLE_E("Invalid listen entry for entity {}. Expected event_name = function or {{ event/name, fn/callback }}.", e.fullName());
    }
}

void ListenEvents::apply(const Entt& e, const sol::object& obj) {
    e.patchOrEmplace<ListenEvents>([&](ListenEvents& listen_events) { listen_events.set(e, obj); });
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

void Tags::apply(const Entt& ew, const sol::object& obj) {
    auto* tags_c = ew.tryGet<Tags>();
    if (!tags_c) {
        tags_c = &ew.emplace<Tags>();
    }

    if (lua::valid<sol::table>(obj)) {
        auto table = lua::as<sol::table>(obj);
        for (const auto& [k, v] : table) {
            if (v.is<std::string>()) {
                tags_c->tags.push_back(v.as<std::string>());
            }
        }
    } else if (lua::valid<std::string>(obj)) {
        tags_c->tags.push_back(lua::as<std::string>(obj));
    } else {
        MLE_E("Invalid obj type passed to Tags apply. {}", obj.get_type());
    }
}

void Tags::applyRemove(const Entt& ew, const sol::object& obj) {
    auto* tags_c = ew.tryGet<Tags>();
    if (!tags_c) {
        return;
    }

    if (lua::valid<sol::table>(obj)) {
        auto table = lua::as<sol::table>(obj);
        for (const auto& [k, v] : table) {
            if (v.is<std::string>()) {
                tags_c->removeTag(v.as<std::string>());
            }
        }
    } else if (lua::valid<std::string>(obj)) {
        tags_c->removeTag(lua::as<std::string>(obj));
    } else {
        MLE_E("Invalid obj type passed to Tags applyRemove. {}", obj.get_type());
    }
}

sol::object Tags::get(const Entt& ew, const sol::object& /*params*/) {
    if (auto* tags = ew.tryGet<Tags>()) {
        sol::table table = Client::i().lua().createTable();
        for (usize i = 0; i < tags->tags.size(); ++i) {
            table[i + 1] = tags->tags[i];
        }
        return table;
    }
    return sol::lua_nil;
}

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
        const auto c = lua::as<Color>(obj).toLinear();
        lt = rt = lb = rb = c;
    } else if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();

        if (const auto lt_r = lua::getFirstKey(table, "lt", 1); lt_r.valid()) {
            lt = lua::as<Color>(lt_r).toLinear();
        }
        if (const auto rt_r = lua::getFirstKey(table, "rt", 2); rt_r.valid()) {
            rt = lua::as<Color>(rt_r).toLinear();
        }
        if (const auto lb_r = lua::getFirstKey(table, "lb", 3); lb_r.valid()) {
            lb = lua::as<Color>(lb_r).toLinear();
        }
        if (const auto rb_r = lua::getFirstKey(table, "rb", 4); rb_r.valid()) {
            rb = lua::as<Color>(rb_r).toLinear();
        }
    } else {
        MLE_E("Invalid obj type passed to Background set. {}", obj.get_type());
    }
};

void OnResized::apply(const Entt& e, const sol::object& obj) {
    lua::assertIs<FuncType>(obj);
    e.patchOrEmplace<OnResized>([&](OnResized& orc) { orc.fn = obj.as<FuncType>(); });
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
