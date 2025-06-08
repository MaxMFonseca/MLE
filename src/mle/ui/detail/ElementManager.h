#pragma once

#include <entt/entt.hpp>

#include "mle/common/Utils.h"
#include "mle/lua/Types.h"

namespace mle::ui::detail {
class ElementManager {
  public:
    MLE_NO_COPY_MOVE(ElementManager)

    ElementManager() = default;
    ~ElementManager() = default;

    void reset(sol::table table = {});

    void update();
    void render();

  private:
    entt::registry registry_;
    entt::entity root_{};
};
}  // namespace mle::ui::detail
