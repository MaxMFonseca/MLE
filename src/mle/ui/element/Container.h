#pragma once

#include "../Types.h"
#include "mle/common/Utils.h"
#include "mle/lua/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/ui/Utils.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Renderable.h"
#include "mle/ui/element/Utils.h"

namespace mle::ui::element {
namespace comp {
class Container : public RenderableInterface {
  public:
    MLE_NO_COPY_MOVE(Container)

    Container() = default;
    ~Container() override = default;

    void update(entt::entity self, const sol::object& obj);

    // Verify all forces full tree traversal in layer order
    void updateChildrenBounds(entt::entity self, Recti context = {}, bool verify_all = false, bool force_update = false);
    void updateChildrenBoundsCol(entt::entity self, Recti content_rect);
    void updateChildrenBoundsRow(entt::entity self, Recti context);
    void updateChildrenBoundsColR(entt::entity self, Recti context);
    void updateChildrenBoundsRowR(entt::entity self, Recti context);

    void update();
    void renderComp(const RenderContext& ctx) const override;

    void addChild(entt::entity self, const sol::table& table, usize pos = max<usize>());
    void addChildren(entt::entity self, const sol::table& table);
    [[nodiscard]] auto getChildren() const { return children_.get(); }
    [[nodiscard]] entt::entity getChild(usize idx) const;
    [[nodiscard]] entt::entity getChild(const std::string& name) const;
    [[nodiscard]] usize getChildIdx(entt::entity) const;

    static void notifyChildChangedBounds(entt::entity child);
    static void notifyChildChangedBounds(entt::entity self, entt::entity child);

    static void lkh(entt::entity self, const sol::object& obj);

  private:
    EntityStorage children_;
    std::set<entt::entity> changed_bounds_children_;

    enum class Direction : u8 { ROW, ROW_R, COL, COL_R };
    enum class Justify : u8 { START, END, CENTER, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };
    enum class AlignCross : u8 { START, END, CENTER, STRETCH, BASELINE };

    Direction direction_ = Direction::COL;
    Justify justify_ = Justify::START;
    AlignCross align_cross_ = AlignCross::START;

    int gap_ = 0;
    int row_gap_ = 0;
    int col_gap_ = 0;
};

struct ChildChangedBounds {};

};  // namespace comp
}  // namespace mle::ui::element
