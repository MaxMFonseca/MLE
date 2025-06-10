#pragma once

#include "../Types.h"
#include "mle/common/Utils.h"
#include "mle/lua/Types.h"
#include "mle/renderer/RenderingThread.h"

namespace mle::ui::element {
struct Layout {
    MLE_NO_COPY_MOVE(Layout)

    Layout() = default;
    virtual ~Layout() = default;

    virtual void updateChildrenBounds(entt::entity self);
};

class Container {
  public:
    MLE_NO_COPY_MOVE(Container)

    Container();
    ~Container();

    void render(renderer::RenderingThread& thread) const;

    void setLayout(Layout* layout) { layout_ = layout; }

    [[nodiscard]] bool isArray() const { return count_ <= 6; }
    void addChild(const sol::table& table);
    void addChildren(const sol::table& table);
    [[nodiscard]] entt::entity getChild(usize idx) const;
    [[nodiscard]] entt::entity getChild(std::string name) const;
    void getChildIdx(entt::entity) const;
    [[nodiscard]] std::span<const entt::entity> getChildren() const;
    [[nodiscard]] usize size() const;
    void remove(entt::entity e);

    void notifyChildChangedBounds(entt::entity self) const;

    void updateChildrenBounds(entt::entity self) const;

  private:
    Layout* layout_ = nullptr;

    union Storage {
        std::array<entt::entity, 6> array{};
        std::vector<entt::entity> vec;

        MLE_NO_COPY_MOVE(Storage)

        Storage() {};
        ~Storage() {};
    } children_;
    // This is used to track if array or vector, since we do not demote this can become invalid after promotion
    usize count_ = 0;
};
}  // namespace mle::ui::element
