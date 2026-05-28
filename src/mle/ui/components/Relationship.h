#pragma once

#include "../Types.h"

namespace mle::ui::comp {
class Relationship {
  public:
    struct NewChild {
        entt::entity child_e = entt::null;
        sol::table comp_table;
    };

  public:
    Relationship() = default;

    [[nodiscard]] bool isChildOf(entt::entity potential_parent) const { return parent_ == potential_parent; }
    [[nodiscard]] entt::entity getParent() const { return parent_; }
    [[nodiscard]] usize getChildCount() const { return child_count_; }
    [[nodiscard]] bool hasChildren() const { return child_count_ > 0; }
    [[nodiscard]] std::vector<entt::entity> getChildren(const Entt& e) const;
    [[nodiscard]] std::vector<entt::entity> getChildrenSortedByLayer(const Entt& e) const;

    [[nodiscard]] entt::entity getChildAt(const Entt& e, usize idx) const;
    [[nodiscard]] Expected<usize> getChildIdx(const Entt& e, std::string_view name) const;
    [[nodiscard]] entt::entity getChildByName(const Entt& e, std::string_view name) const;

    [[nodiscard]] NewChild createChildBase(const Entt& e, const sol::table& table);
    [[nodiscard]] NewChild createChildBaseNamed(const Entt& e, const sol::table& table, const std::string& name);
    [[nodiscard]] std::vector<NewChild> createChildrenBase(const Entt& e, const sol::table& table);
    void destroyChild(const Entt& e, entt::entity child_e);
    void destroyAllChildren(const Entt& e);

    entt::entity createChild(const Entt& e, const sol::table& table);
    void createChildren(const Entt& e, const sol::table& table);

    void applyOnChildren(const Entt& ew, const sol::table& table) const;
    void enableAll(const Entt& e) const;
    void disableAll(const Entt& e) const;
    void disableAllBut(const Entt& e, std::string_view child_name) const;

    static void callOnDestroy(const Entt& child);

    static void applyAddChildren(const Entt& e, const sol::object& obj);
    static void applyAddChild(const Entt& e, const sol::object& obj);

  private:
    [[nodiscard]] entt::entity createChildHndAt(const Entt& e, usize idx = max<usize>());
    [[nodiscard]] std::pair<Entt, Relationship&> createChildHnd(const Entt& e);

    static void applyBaseOnChild(const Entt& e, const sol::table& table, entt::entity child_e);

    static void callOnCreate(const Entt& child);

  private:
    entt::entity parent_ = entt::null;
    entt::entity left_ = entt::null;
    entt::entity right_ = entt::null;
    entt::entity first_child_ = entt::null;
    usize child_count_ = 0;
};

struct ChildrenBase {
    sol::table o;
};

}  // namespace mle::ui::comp
