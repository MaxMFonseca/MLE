#pragma once

#include "Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"
#include "mle/ui/components/Relationship.h"
#include "mle/ui/systems/LuaElementOps.h"
#include "mle/utils/ECS.h"
#include "sol/forward.hpp"

namespace mle::ui {
class Entt {
  public:
    Entt(UI& ui, entt::entity entity) :
        ui_(ui),
        e_(entity) {}

    [[nodiscard]] auto& ui() const { return ui_; }
    [[nodiscard]] auto e() const { return e_; }

    void apply(const std::string& key, const sol::object& obj = {}) const { system::LuaElementOps::applyObj(*this, key, obj); }
    [[nodiscard]] sol::object getKey(const std::string& key, const sol::object& params = {}) const { return system::LuaElementOps::getKey(*this, key, params); }
    void applyTable(const sol::table& table) const { system::LuaElementOps::applyTable(*this, table); }

    template <typename... Args>
    void call(const std::string& fn_name, Args&&... args) const {
        if (!has<comp::Functions>()) {
            MLE_E("Entity '{}' has no Functions component to call '{}'", fullName(), fn_name);
            return;
        }
        auto& functions = get<comp::Functions>();
        auto fn = functions.get(fn_name);
        if (!fn) {
            MLE_E("Function '{}' not found in entity '{}'", fn_name, fullName());
            return;
        }

        fn(*this, std::forward<Args>(args)...);
    }

    void setName(const std::string& name) const;
    void destroy() const;

    [[nodiscard]] auto& getRelationship() const { return get<comp::Relationship>(); }
    [[nodiscard]] entt::entity getParent() const { return getRelationship().getParent(); }
    [[nodiscard]] bool hasFitSize() const;
    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string fullName() const;

    [[nodiscard]] Expected<Entt> getChild(std::span<const std::string_view> tree) const;

    [[nodiscard]] Entt derive(entt::entity e) const { return {ui_, e}; }
    [[nodiscard]] bool isRoot() const;

    void requestInternalBoundsUpdate() const;
    void requestExternalBoundsUpdate() const;

    template <typename T>
    [[nodiscard]] T& get() const {
        return ui_.getRegistry().get<T>(e_);
    }

    template <typename T>
    [[nodiscard]] T* tryGet() const {
        return ui_.getRegistry().try_get<T>(e_);
    }

    template <typename... T>
    [[nodiscard]] bool has() const {
        return ui_.getRegistry().all_of<T...>(e_);
    }

    template <typename... T>
    [[nodiscard]] bool hasAny() const {
        return ui_.getRegistry().any_of<T...>(e_);
    }

    template <typename T>
    T& emplace()
        requires std::default_initializable<T>
    {
        MLE_ASSERT(!has<T>());
        return ui_.getRegistry().emplace<T>(e_);
    }

    template <typename T, typename... Args>
    T& emplace(Args&&... args) const {
        MLE_ASSERT(!has<T>());
        return ui_.getRegistry().emplace<T>(e_, std::forward<Args>(args)...);
    }

    template <typename T>
    void addFlag() const {
        if (!has<T>()) {
            ui_.getRegistry().emplace<T>(e_);
        }
    }

    template <typename T, typename... Args>
    T& emplaceOrReplace(Args&&... args) const {
        return ui_.getRegistry().emplace_or_replace<T>(e_, std::forward<Args>(args)...);
    }

    template <typename T>
    void erase() const {
        MLE_ASSERT(has<T>());
        ui_.getRegistry().remove<T>(e_);
    }

    template <typename T>
    void eraseChecked() const {
        ui_.getRegistry().remove<T>(e_);
    }

    template <typename T, typename... Args>
    void replace(Args&&... args) const {
        MLE_ASSERT(has<T>());
        ui_.getRegistry().replace<T>(e_, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T& getOrEmplace() const
        requires std::default_initializable<T>
    {
        if (has<T>()) {
            return get<T>();
        }
        return emplace<T>();
    }

    template <typename T, typename Fn>
    void patch(Fn&& fn) const
        requires std::is_invocable_v<Fn, T&> && std::is_invocable_r_v<void, Fn, T&> && std::default_initializable<T>
    {
        MLE_ASSERT(has<T>());
        ui_.getRegistry().patch<T>(e_, std::forward<Fn>(fn));
    }

    template <typename T, typename Fn>
    void patchOrEmplace(Fn&& fn) const
        requires std::is_invocable_v<Fn, T&> && std::is_invocable_r_v<void, Fn, T&> && std::default_initializable<T>
    {
        if (has<T>()) {
            patch<T>(std::forward<Fn>(fn));
        } else {
            std::forward<Fn>(fn)(emplace<T>());
        }
    }

  private:
    UI& ui_;
    entt::entity e_;
};
}  // namespace mle::ui

namespace fmt {
template <>
struct formatter<mle::ui::Entt> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Entt& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "Entt({})", v.fullName());
    }
};
}  // namespace fmt
