#pragma once

#include "Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"
#include "mle/ui/components/Relationship.h"
#include "mle/utils/ECS.h"

namespace mle::ui {
class Entt {
  public:
    Entt(UI& ui, entt::entity entity) :
        ui_(ui),
        e_(entity) {}

    [[nodiscard]] auto& ui() const { return ui_; }
    [[nodiscard]] auto e() const { return e_; }
    void setE(entt::entity e) { e_ = e; }

    void apply(const std::string& key, const sol::object& obj) const { ui_.getLuaElementOps().applyObj(e_, key, obj); }
    void applyTable(const sol::table& table) const { ui_.getLuaElementOps().applyTable(e_, table); }
    void setName(const std::string& name) const;
    void destroy() const;

    [[nodiscard]] auto& getRelationship() const { return get<comp::Relationship>(); }
    [[nodiscard]] entt::entity getParent() const { return getRelationship().getParent(); }
    [[nodiscard]] bool hasFitSize() const;
    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string fullName() const;

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
    void eraseCheck() const {
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
