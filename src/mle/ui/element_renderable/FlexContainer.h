#pragma once

#include "../Element.h"

namespace mle::ui::element_renderable {
class FlexContainer : public Element::Container {
  public:
    MLE_FLAGS_BEGIN(BoundComplete, u16)
    MLE_FLAG(BoundComplete, CONTENT_X)
    MLE_FLAG(BoundComplete, PAD_L)
    MLE_FLAG(BoundComplete, PAD_R)
    MLE_FLAG(BoundComplete, MARG_L)
    MLE_FLAG(BoundComplete, MARG_R)
    MLE_FLAG(BoundComplete, REM_X)
    MLE_FLAG(BoundComplete, CONTENT_Y)
    MLE_FLAG(BoundComplete, PAD_T)
    MLE_FLAG(BoundComplete, PAD_B)
    MLE_FLAG(BoundComplete, MARG_T)
    MLE_FLAG(BoundComplete, MARG_B)
    MLE_FLAG(BoundComplete, REM_Y)
    MLE_FLAGS_END(BoundComplete);

  public:
    FlexContainer(FlexContainer&&) = delete;
    FlexContainer& operator=(FlexContainer&&) = delete;
    FlexContainer(const FlexContainer&) = delete;
    FlexContainer& operator=(const FlexContainer&) = delete;

    explicit FlexContainer(ElementRef element);
    ~FlexContainer() override = default;

    void set(const sol::table& obj) override;

    void updateChildrenBounds() override;

    static void registerLuaTypes();

  private:
    void updateChildBounds(std::vector<Element::NewBounds>& new_bounds_vec, ElementRef child);

    void sortChildrenByDependencies();

    struct RemainginSizeHelper {
        f32 content = 0.0F;
        f32 padding_a = 0.0F;
        f32 padding_b = 0.0F;
        f32 margin_a = 0.0F;
        f32 margin_b = 0.0F;
    };

    struct TryComputeChildBoundData {
        std::vector<Element::NewBounds>& new_bounds_vec;
        const Element::TargetBounds& target;
        f32 target_ar;
        RemainginSizeHelper rem_x, rem_y;
        BoundCompleteFlags complete_flags;
    };
    bool tryComputeChildBoundContentSizeX(TryComputeChildBoundData& data);
    bool tryComputeChildBoundContentSizeY(TryComputeChildBoundData& data);
    bool tryComputeChildBoundExtra(TryComputeChildBoundData& data, BoundCompleteFlagBits flag_bit, Element::Bound target_bound, f32& new_bound, f32& greedy,
                                   bool is_x, f32* set_offset = nullptr);
    static bool tryResolveRemainingX(TryComputeChildBoundData& data);
    static bool tryResolveRemainingY(TryComputeChildBoundData& data);

  private:
};
}  // namespace mle::ui::element_renderable

#define TEMP_BOUND_COMPLETE mle::ui::element_renderable::FlexContainer::BoundComplete
MLE_FLAGS_FMT_BEGIN(TEMP_BOUND_COMPLETE)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, CONTENT_X)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, PAD_L)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, PAD_R)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, MARG_L)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, MARG_R)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, REM_X)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, CONTENT_Y)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, PAD_T)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, PAD_B)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, MARG_T)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, MARG_B)
MLE_FLAGS_FMT_CASE(TEMP_BOUND_COMPLETE, REM_Y)
MLE_FLAGS_FMT_END(TEMP_BOUND_COMPLETE)
#undef TEMP_BOUND_COMPLETE
