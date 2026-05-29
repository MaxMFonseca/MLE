#include "Entt.h"

#include "UI.h"
#include "mle/client/Client.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Hoverable.h"
#include "mle/utils/ECS.h"

namespace mle::ui {
void Entt::setName(const std::string& name) const {
    if (name.empty()) {
        erase<comp::Name>();
        return;
    }

    if (name.contains('.')) {
        MLE_W("Invalid character '.' in element name '{}'", name);
        return;
    }
    patchOrEmplace<comp::Name>([&](auto& n) { n.o = name; });
}

bool Entt::hasFitSize() const {
    const auto* target_size = tryGet<comp::TargetSize>();
    if (!target_size) {
        return true;
    }

    if (target_size->x.type == TargetBound::Type::FIT || target_size->y.type == TargetBound::Type::FIT) {
        return true;
    }
    if (((target_size->x.type == TargetBound::Type::DEFAULT && target_size->x.val == 0) ||
         (target_size->y.type == TargetBound::Type::DEFAULT && target_size->y.val == 0)) &&
        has<comp::SizeProvider>()) {
        return true;
    }
    return true;
}

std::string Entt::name() const {
    if (const auto* name_comp = tryGet<comp::Name>(); name_comp) {
        return name_comp->o;
    }
    return getRelationship().getParent() == entt::null ? "<root>" : "<unnamed>";
};

// NOLINTNEXTLINE(misc-no-recursion) No problem
std::string Entt::fullName() const {
    auto parent = getParent();
    if (parent == entt::null) {
        return "<root>";
    }
    return Entt{ui_, parent}.fullName() + "." + name();
}

void Entt::destroy() const {
    addFlag<comp::DestroyFlag>();
}

bool Entt::hasTag(std::string_view tag) const {
    if (auto* tags_comp = tryGet<comp::Tags>()) {
        return std::ranges::find(tags_comp->tags, tag) != tags_comp->tags.end();
    }
    return false;
}

// NOLINTNEXTLINE(misc-no-recursion) No problem
void Entt::getChildrenWithTagRecursiveHelper(std::string_view tag, std::vector<entt::entity>& out) const {
    auto& relationship = getRelationship();
    for (usize i = 0; i < relationship.getChildCount(); ++i) {
        auto child_e = relationship.getChildAt(*this, i);
        Entt child_ew{ui_, child_e};
        if (child_ew.hasTag(tag)) {
            out.push_back(child_e);
        }
        child_ew.getChildrenWithTagRecursiveHelper(tag, out);
    }
}

[[nodiscard]] std::vector<Entt> Entt::getChildrenWithTagRecursive(std::string_view tag) const {
    std::vector<entt::entity> result;
    getChildrenWithTagRecursiveHelper(tag, result);
    std::vector<Entt> result2;
    result2.reserve(result.size());
    for (entt::entity e : result) {
        result2.emplace_back(ui_, e);
    }
    return result2;
}

// NOLINTNEXTLINE(misc-no-recursion) No problem
void Entt::getChildrenNamedRecursiveHelper(std::string_view name, std::vector<entt::entity>& out) const {
    auto& relationship = getRelationship();
    for (usize i = 0; i < relationship.getChildCount(); ++i) {
        auto child_e = relationship.getChildAt(*this, i);
        Entt child_ew{ui_, child_e};
        if (child_ew.name() == name) {
            out.push_back(child_e);
        }
        child_ew.getChildrenNamedRecursiveHelper(name, out);
    }
}

[[nodiscard]] std::vector<Entt> Entt::getChildrenNamedRecursive(std::string_view name) const {
    std::vector<entt::entity> result;
    getChildrenNamedRecursiveHelper(name, result);
    std::vector<Entt> result2;
    result2.reserve(result.size());
    for (entt::entity e : result) {
        result2.emplace_back(ui_, e);
    }
    return result2;
};

Expected<Entt> Entt::getChild(std::span<const std::string_view> tree) const {
    MLE_ASSERT(!tree.empty());

    entt::entity cur = e();
    for (const std::string_view name : tree) {
        Entt ew = derive(cur);

        auto& relationship = ew.getRelationship();
        auto next_e = relationship.getChildByName(ew, name);
        if (next_e == entt::null) {
            return std::unexpected(Result::NOT_FOUND);
        }
        cur = next_e;
    }
    return derive(cur);
}

// NOLINTNEXTLINE(misc-no-recursion) No problem
void Entt::requestInternalBoundsUpdate() const {
    addFlag<comp::RequestInternalBoundsUpdateFlag>();
    if (hasFitSize()) {
        requestExternalBoundsUpdate();
    }
}

// NOLINTNEXTLINE(misc-no-recursion) No problem
void Entt::requestExternalBoundsUpdate() const {
    auto& relationship = getRelationship();
    if (relationship.getParent() == entt::null) {
        addFlag<comp::RequestExternalBoundsUpdateFlag>();
        return;
    }
    Entt parent_ew = derive(relationship.getParent());
    parent_ew.requestInternalBoundsUpdate();
}

bool Entt::isRoot() const {
    return e_ == ui_.getRoot();
};

// NOLINTNEXTLINE(misc-no-recursion) No problem
[[nodiscard]] bool Entt::isRecursivelyDisabled() const {
    if (has<comp::DisabledFlag>()) {
        return true;
    }
    auto parent = getRelationship().getParent();
    if (parent == entt::null) {
        return false;
    }
    return derive(parent).isRecursivelyDisabled();
}

void Entt::dispatch(const std::string& event_name, const sol::object& obj) const {
    ui_.eventSystem().enqueueEvent(event_name, obj);
};

[[nodiscard]] Recti Entt::getBoundsOnRoot() const {
    return get<comp::Bounds>().onRoot(*this);
};

[[nodiscard]] Rectf Entt::getBoundsOnRootNormalized() const {
    return get<comp::Bounds>().onRootNormalized(*this);
};

// FIXME: every time I create a popup, I trigger a full layout recalculation. Optimize this.

void Entt::createPopup(const sol::table& comp) const {
    const entt::entity parent_popup = ui_.findNearestPopupRoot(e_);
    ui_.trimPopupStackTo(parent_popup);
    const u32 stack_index = ui_.getNextPopupStackIndex(parent_popup);

    auto pop_up = Client::i().lua().createTable();

    pop_up["layer"] = 1000 + as<int>(stack_index);
    pop_up["table"] = Client::i().lua().createTable();
    pop_up["table"]["owner"] = *this;

    pop_up["comp"] = comp;

    Entt root_ew{ui_, ui_.getRoot()};
    auto pop_up_e = root_ew.getRelationship().createChild(root_ew, pop_up);
    Entt pop_up_ew{ui_, pop_up_e};

    pop_up_ew.emplace<mle::ui::comp::PopupRoot>(mle::ui::comp::PopupRoot{
        .owner = e_,
        .parent_popup = parent_popup,
        .stack_index = stack_index,
    });
};

}  // namespace mle::ui
