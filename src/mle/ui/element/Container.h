#pragma once

#include "../Types.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/lua/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/ui/Utils.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Renderable.h"
#include "mle/ui/element/Utils.h"

namespace mle::ui::element {
namespace comp {
struct ChildBuildInfo;
class Container {
  public:
    enum class Direction : u8 { ROW, ROW_R, COL, COL_R };
    enum class Justify : u8 { START, END, CENTER, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };
    enum class AlignCross : u8 { START, END, CENTER, STRETCH, BASELINE };

  public:
    MLE_NO_COPY_MOVE(Container)

    Container() = default;
    ~Container() = default;

    void update(entt::entity self, const sol::object& obj);

    // Verify all forces full tree traversal in layer order
    void updateChildrenBounds(entt::entity self, vec2u max_size = {}, bool verify_all = false, bool force_update = false);

    void update();
    void render(const RenderContext& ctx) const;

    void addChild(entt::entity self, const sol::table& table, usize pos = max<usize>());
    void addChildren(entt::entity self, const sol::table& table);
    [[nodiscard]] auto getChildren() const { return children_.get(); }
    [[nodiscard]] entt::entity getChild(usize idx) const;
    [[nodiscard]] entt::entity getChild(const std::string& name) const;
    [[nodiscard]] usize getChildIdx(entt::entity) const;

    static void notifyChildChangedBounds(entt::entity child);
    static void notifyChildChangedBounds(entt::entity self, entt::entity child);

    void apply(entt::entity self, const sol::object& obj);

  private:
    struct FlexUpdateData {
        entt::entity self;
        Recti padded;
        std::vector<ChildBuildInfo>& cinfos;
        std::set<entt::entity>& changed_children;
    };

    void updateChildrenBoundsFlex(FlexUpdateData data);

    [[nodiscard]] auto getChildrenSpan() { return children_span_; }
    void calculateChildrenSpan(const std::vector<ChildBuildInfo>& cinfos, std::array<int, 4> padding);

  private:
    EntityStorage children_;
    std::set<entt::entity> children_req_update_;

    Direction direction_ = Direction::COL;
    Justify justify_ = Justify::START;
    AlignCross align_cross_ = AlignCross::START;

    int gap_ = 0;
    int row_gap_ = 0;
    int col_gap_ = 0;

    Recti children_span_{0, 0, 0, 0};
};

struct ChildChangedBounds {};

};  // namespace comp
}  // namespace mle::ui::element

namespace fmt {

template <>
struct formatter<mle::ui::element::comp::Container::Justify> : formatter<std::string> {
    using Justify = mle::ui::element::comp::Container::Justify;
    template <typename FormatContext>
    constexpr auto format(Justify j, FormatContext& ctx) const {
        switch (j) {
            case Justify::START:
                return format_to(ctx.out(), "START");
            case Justify::END:
                return format_to(ctx.out(), "END");
            case Justify::CENTER:
                return format_to(ctx.out(), "CENTER");
            case Justify::SPACE_BETWEEN:
                return format_to(ctx.out(), "SPACE_BETWEEN");
            case Justify::SPACE_AROUND:
                return format_to(ctx.out(), "SPACE_AROUND");
            case Justify::SPACE_EVENLY:
                return format_to(ctx.out(), "SPACE_EVENLY");
        }
    }
};

template <>
struct formatter<mle::ui::element::comp::Container::Direction> : formatter<std::string> {
    using Direction = mle::ui::element::comp::Container::Direction;
    template <typename FormatContext>
    constexpr auto format(Direction d, FormatContext& ctx) const {
        switch (d) {
            case Direction::ROW:
                return format_to(ctx.out(), "ROW");
            case Direction::ROW_R:
                return format_to(ctx.out(), "ROW_R");
            case Direction::COL:
                return format_to(ctx.out(), "COL");
            case Direction::COL_R:
                return format_to(ctx.out(), "COL_R");
        }
    }
};

template <>
struct formatter<mle::ui::element::comp::Container::AlignCross> : formatter<std::string> {
    using AlignCross = mle::ui::element::comp::Container::AlignCross;
    template <typename FormatContext>
    constexpr auto format(AlignCross ac, FormatContext& ctx) const {
        switch (ac) {
            case AlignCross::START:
                return format_to(ctx.out(), "START");
            case AlignCross::END:
                return format_to(ctx.out(), "END");
            case AlignCross::CENTER:
                return format_to(ctx.out(), "CENTER");
            case AlignCross::STRETCH:
                return format_to(ctx.out(), "STRETCH");
            case AlignCross::BASELINE:
                return format_to(ctx.out(), "BASELINE");
        }
    }
};

}  // namespace fmt
