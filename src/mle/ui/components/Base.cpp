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

sol::object Table::get(const Entt& e) {
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
}  // namespace mle::ui::comp
