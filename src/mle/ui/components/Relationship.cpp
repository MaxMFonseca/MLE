#include "Relationship.h"

#include "../Entt.h"
#include "mle/client/Client.h"
#include "mle/lua/Utils.h"
#include "mle/ui/components/Base.h"

namespace mle::ui::comp {
entt::entity Relationship::getChildAt(const Entt& e, usize idx) const {
    if (first_child_ == entt::null) {
        return entt::null;
    }

    if (idx >= child_count_) {
        MLE_E("Tried to get child at invalid idx {} in Container at entity {} with {} children.", idx, e.fullName(), child_count_);
        return entt::null;
    }

    auto ichild = first_child_;
    for (usize i = 0; i < idx; ++i) {
        auto& child_relationship = Entt{e.ui(), ichild}.getRelationship();
        ichild = child_relationship.right_;
    }
    return ichild;
}

Expected<usize> Relationship::getChildIdx(const Entt& e, std::string_view name) const {
    entt::entity ichild = first_child_;

    for (usize i = 0; i < child_count_; ++i) {
        Entt child{e.ui(), ichild};
        if (child.name() == name) {
            return i;
        }
        ichild = child.getRelationship().right_;
    }
    MLE_E("Tried to get index of child named '{}' in Container at entity {} but no such child exists.", name, e.fullName());
    return std::unexpected(Result::NOT_FOUND);
}

entt::entity Relationship::getChildByName(const Entt& e, std::string_view name) const {
    auto idx_r = getChildIdx(e, name);
    if (!idx_r) {
        MLE_E("Tried to get child named '{}' in Container at entity {} but no such child exists.", name, e.fullName());
        return entt::null;
    }
    return getChildAt(e, *idx_r);
}

void Relationship::destroyChild(const Entt& e, entt::entity child_e) {
    auto& self_relationship = e.getRelationship();

    Entt removing_child{e.ui(), child_e};
    auto& removing_child_relationship = removing_child.getRelationship();

    if (removing_child_relationship.parent_ != e.e()) {
        MLE_E("Tried to destroy child entity {}({}) from Container at entity {} but it is not a child of it.", as<u32>(child_e), removing_child.fullName(),
              e.fullName());
        return;
    }

    if (removing_child_relationship.right_ == child_e) {
        self_relationship.first_child_ = entt::null;
    } else {
        Entt left_sibling{e.ui(), removing_child_relationship.left_};
        auto& left_relationship = left_sibling.getRelationship();
        Entt right_sibling{e.ui(), removing_child_relationship.right_};
        auto& right_relationship = right_sibling.getRelationship();

        left_relationship.right_ = removing_child_relationship.right_;
        right_relationship.left_ = removing_child_relationship.left_;

        if (self_relationship.first_child_ == child_e) {
            self_relationship.first_child_ = removing_child_relationship.right_;
        }
    }

    self_relationship.child_count_ -= 1;

    if (removing_child_relationship.child_count_ > 0) {
        destroyAllChildren(removing_child);
    }

    callOnDestroy(removing_child);

    e.ui().getRegistry().destroy(child_e);
}

// NOLINTNEXTLINE(misc-no-recursion) Cool recursion
void Relationship::destroyAllChildren(const Entt& e) {
    auto& self_relationship = e.getRelationship();
    if (self_relationship.first_child_ == entt::null) {
        return;
    }

    auto child_e = self_relationship.first_child_;
    for (usize i = 0; i < self_relationship.child_count_; ++i) {
        auto current_child_e = child_e;
        Entt current_child{e.ui(), current_child_e};
        auto& current_child_relationship = current_child.getRelationship();
        child_e = current_child_relationship.right_;

        if (current_child_relationship.child_count_ > 0) {
            destroyAllChildren(current_child);
        }

        callOnDestroy(current_child);

        e.ui().getRegistry().destroy(current_child_e);
    }

    self_relationship.first_child_ = entt::null;
    self_relationship.child_count_ = 0;
}

std::vector<entt::entity> Relationship::getChildren(const Entt& e) const {
    std::vector<entt::entity> children;
    children.reserve(e.getRelationship().getChildCount());
    entt::entity ichild = first_child_;
    for (usize i = 0; i < child_count_; ++i) {
        children.push_back(ichild);
        Entt child{e.ui(), ichild};
        ichild = child.getRelationship().right_;
    }
    return children;
}

std::vector<entt::entity> Relationship::getChildrenSortedByLayer(const Entt& e) const {
    auto children = getChildren(e);
    std::ranges::sort(children, [&e](entt::entity a, entt::entity b) {
        auto layer_a = 0;
        auto layer_b = 0;
        Entt ew_a{e.ui(), a};
        Entt ew_b{e.ui(), b};
        if (ew_a.has<comp::Layer>()) {
            layer_a = ew_a.get<comp::Layer>().layer;
        }
        if (ew_b.has<comp::Layer>()) {
            layer_b = ew_b.get<comp::Layer>().layer;
        }
        return layer_a > layer_b;
    });
    return children;
}

std::vector<Relationship::NewChild> Relationship::createChildrenBase(const Entt& e, const sol::table& table) {
    std::vector<NewChild> new_children;

    for (auto& [name_r, child_table_r] : table) {
        MLE_ASSERT(lua::valid<sol::table>(child_table_r));
        const auto child_table = lua::as<sol::table>(child_table_r);

        std::string name;
        if (lua::valid<std::string>(name_r)) {
            name = lua::as<std::string>(name_r);
        }

        if (name.empty()) {
            new_children.emplace_back(createChildBase(e, child_table));
        } else {
            new_children.emplace_back(createChildBaseNamed(e, child_table, name));
        }
    }

    return new_children;
};

Relationship::NewChild Relationship::createChildBase(const Entt& e, const sol::table& table) {
    std::string name;
    lua::tryGetFirstKeyAs(table, name, "name", 1);
    return createChildBaseNamed(e, table, name);
};

Relationship::NewChild Relationship::createChildBaseNamed(const Entt& e, const sol::table& table, const std::string& name) {
    auto pos = max<usize>();
    lua::tryGetFirstKeyAs(table, pos, "idx", 2);

    entt::entity child_e = createChildHndAt(e, pos);
    if (child_e == entt::null) {
        MLE_E("Failed to create child for Container at entity {}.", e.fullName());
        return {};
    }

    Entt child{e.ui(), child_e};
    if (!name.empty()) {
        child.setName(name);
    }

    return {.child_e = child_e, .comp_table = table};
}

std::pair<Entt, Relationship&> Relationship::createChildHnd(const Entt& e) {
    auto child_e = e.ui().getRegistry().create();
    Entt child{e.ui(), child_e};
    auto& child_relationship = child.emplace<Relationship>();
    child.emplace<comp::Bounds>();
    child_relationship.parent_ = e.e();
    child_count_ += 1;
    return {child, child_relationship};
};

entt::entity Relationship::createChildHndAt(const Entt& e, usize idx) {
    if (first_child_ == entt::null) {
        auto [new_child, new_child_relationship] = createChildHnd(e);

        first_child_ = new_child.e();
        new_child_relationship.left_ = new_child.e();
        new_child_relationship.right_ = new_child.e();

        return new_child.e();
    }

    if (idx == 0) {
        auto [new_child, new_child_relationship] = createChildHnd(e);

        auto right_sibling = first_child_;
        auto& right_sibling_relationship = Entt{e.ui(), right_sibling}.getRelationship();
        auto left_sibling = right_sibling_relationship.left_;
        auto& left_sibling_relationship = Entt{e.ui(), left_sibling}.getRelationship();

        new_child_relationship.right_ = right_sibling;
        new_child_relationship.left_ = left_sibling;

        left_sibling_relationship.right_ = new_child.e();
        right_sibling_relationship.left_ = new_child.e();

        first_child_ = new_child.e();
        return new_child.e();
    }

    if (idx == max<usize>()) {
        idx = child_count_;
    }

    entt::entity left_sibling = getChildAt(e, idx - 1);
    if (left_sibling == entt::null) {
        MLE_E("Failed to get left sibling at idx {} when creating child for Container at entity {}.", idx - 1, e.fullName());
        return entt::null;
    }

    auto [new_child, new_child_relationship] = createChildHnd(e);

    auto& left_sibling_relationship = Entt{e.ui(), left_sibling}.getRelationship();
    entt::entity right_sibling = left_sibling_relationship.right_;
    auto& right_sibling_relationship = Entt{e.ui(), right_sibling}.getRelationship();

    new_child_relationship.left_ = left_sibling;
    new_child_relationship.right_ = right_sibling;

    left_sibling_relationship.right_ = new_child.e();
    right_sibling_relationship.left_ = new_child.e();

    return new_child.e();
};

entt::entity Relationship::createChild(const Entt& e, const sol::table& table) {
    auto [child_e, comp_table] = createChildBase(e, table);

    applyBaseOnChild(e, comp_table, child_e);

    callOnCreate(e);

    return child_e;
}

void Relationship::callOnCreate(const Entt& child) {
    if (child.has<comp::OnCreate>()) {
        child.get<comp::OnCreate>().fn(child);
        child.eraseChecked<comp::OnCreate>();
    }
}

void Relationship::callOnDestroy(const Entt& child) {
    if (child.has<comp::OnDestroy>()) {
        const auto& on_destroy = child.get<comp::OnDestroy>();
        if (on_destroy.fn) {
            on_destroy.fn(child);
        }
    }
}

void Relationship::createChildren(const Entt& e, const sol::table& table) {
    auto children_data = createChildrenBase(e, table);

    for (const auto& [child_e, comp_table] : children_data) {
        applyBaseOnChild(e, comp_table, child_e);
        callOnCreate(Entt{e.ui(), child_e});
    }
}

void Relationship::applyBaseOnChild(const Entt& e, const sol::table& table, entt::entity child_e) {
    Entt centt{e.ui(), child_e};

    if (e.has<comp::ChildrenBase>()) {
        centt.applyTable(e.get<comp::ChildrenBase>().o);
    }

    centt.applyTable(table);
}

void Relationship::applyAddChildren(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    auto table_r = lua::tryAs<sol::table>(obj);
    if (!table_r) {
        MLE_E("Invalid object to apply add_children at entity {}. Expected table.", e.name());
        return;
    }

    e.getRelationship().createChildren(e, *table_r);
}

void Relationship::applyAddChild(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    auto table_r = lua::tryAs<sol::table>(obj);
    if (!table_r) {
        MLE_E("Invalid object to apply add_child at entity {}. Expected table.", e.name());
        return;
    }

    e.getRelationship().createChild(e, *table_r);
}

void Relationship::applyOnChildren(const Entt& ew, const sol::table& table) const {
    for (const auto& child_e : getChildren(ew)) {
        ew.derive(child_e).applyTable(table);
    }
}

void Relationship::enableAll(const Entt& e) const {
    for (const auto& child_e : getChildren(e)) {
        auto child = e.derive(child_e);
        child.eraseChecked<DisabledFlag>();
        child.requestExternalBoundsUpdate();
    }
}

void Relationship::disableAll(const Entt& e) const {
    for (const auto& child_e : getChildren(e)) {
        auto child = e.derive(child_e);
        child.addFlag<DisabledFlag>();
        child.requestExternalBoundsUpdate();
    }
}

void Relationship::disableAllBut(const Entt& e, std::string_view child_name) const {
    const auto child_to_enable = getChildByName(e, child_name);
    if (child_to_enable == entt::null) {
        return;
    }

    for (const auto& child_e : getChildren(e)) {
        auto child = e.derive(child_e);
        if (child_e == child_to_enable) {
            child.eraseChecked<DisabledFlag>();
        } else {
            child.addFlag<DisabledFlag>();
        }
        child.requestExternalBoundsUpdate();
    }
}

}  // namespace mle::ui::comp
