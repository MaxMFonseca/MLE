#pragma once

#include "../Types.h"
#include "mle/utils/ECS.h"
#include "mle/utils/Utils.h"

namespace mle::ui::system {
class Hover {
  public:
    MLE_NO_COPY_MOVE(Hover);

    explicit Hover(UI& ui) :
        ui_(ui) {}
    ~Hover() = default;

    void update();

  private:
    void hovered(const Entt& e, vec2f pos_parent, const comp::Bounds& self_bounds, std::vector<entt::entity>& inside);

  private:
    UI& ui_;
};
}  // namespace mle::ui::system
