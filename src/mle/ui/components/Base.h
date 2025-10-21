#pragma once

#include "../Types.h"
#include "mle/utils/Color.h"
#include "mle/utils/ECS.h"

namespace mle::ui::comp {
struct Name {
    std::string o;
};

class Relationship {
  public:
    struct NewChild {
        entt::entity child_e = entt::null;
        sol::table comp_table;
    };

  public:
    Relationship() = default;

    [[nodiscard]] bool isChildOf(entt::entity potential_parent) const { return parent_ == potential_parent; }
    [[nodiscard]] entt::entity getParent() { return parent_; }
    [[nodiscard]] usize getChildCount() const { return child_count_; }
    [[nodiscard]] std::vector<entt::entity> getChildren(const Entt& e);

    [[nodiscard]] entt::entity getChildAt(const Entt& e, usize idx);
    [[nodiscard]] Expected<usize> getChildIdx(const Entt& e, std::string_view name);
    [[nodiscard]] entt::entity getChildByName(const Entt& e, std::string_view name);

    [[nodiscard]] NewChild createChildBase(const Entt& e, const sol::table& table);
    [[nodiscard]] NewChild createChildBaseNamed(const Entt& e, const sol::table& table, const std::string& name);
    [[nodiscard]] std::vector<NewChild> createChildrenBase(const Entt& e, const sol::table& table);
    void destroyChild(const Entt& e, entt::entity child_e);
    void destroyAllChildren(const Entt& e);

  private:
    [[nodiscard]] entt::entity createChildHndAt(const Entt& e, usize idx = max<usize>());
    [[nodiscard]] std::pair<Entt, Relationship&> createChildHnd(const Entt& e);

  private:
    entt::entity parent_ = entt::null;
    entt::entity left_ = entt::null;
    entt::entity right_ = entt::null;
    entt::entity first_child_ = entt::null;
    usize child_count_ = 0;
};

struct Background {
    Color color = Color::ZERO;

    void set(const sol::object& obj) { color = Color::fromLua(obj); }

    static void apply(const Entt& e, const sol::object& obj);
};

struct ContainerNeedsInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};

}  // namespace mle::ui::comp
