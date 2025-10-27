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
    entt::entity getFirstHoveredChild(entt::entity e, vec2i pos);

  private:
    UI& ui_;
};
}  // namespace mle::ui::system
