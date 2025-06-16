#pragma once

#include "../Types.h"
#include "mle/common/Utils.h"
#include "mle/lua/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/ui/Utils.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element {
struct Layout {
    MLE_NO_COPY_MOVE(Layout)

    Layout() = default;
    virtual ~Layout() = default;

    virtual void updateChildrenBounds(entt::entity self, Recti context, bool force_update) const = 0;
};

using LayoutHnd = std::unique_ptr<Layout>;
using LayoutRef = std::shared_ptr<Layout>;

namespace comp {
class Container : public RenderableInterface {
  public:
    MLE_NO_COPY_MOVE(Container)

    explicit Container(LayoutHnd&& layout);
    ~Container() override = default;

    void updateChildrenBounds(entt::entity self, Recti context = {}, bool verify_all = false, bool force_update = false) const;

    void update();
    void renderComp(entt::entity self, renderer::RenderingThreadRef thread) const override;

    void addChild(entt::entity self, const sol::table& table, usize pos = max<usize>());
    void addChildren(entt::entity self, const sol::table& table);
    auto getChildren() { return children_.get(); }
    [[nodiscard]] entt::entity getChild(usize idx) const;
    [[nodiscard]] entt::entity getChild(std::string name) const;

    static void notifyChildChangedBounds(entt::entity child);
    static void notifyChildChangedBounds(entt::entity self, entt::entity child);

    static void add(entt::entity self, LayoutHnd&& layout, const sol::table& table);

  private:
    LayoutHnd layout_ = nullptr;
    EntityStorage children_;
    std::set<entt::entity> changed_bounds_children_;
};

struct ChildChangedBounds {};

};  // namespace comp
}  // namespace mle::ui::element
