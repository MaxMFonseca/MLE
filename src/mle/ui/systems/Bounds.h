#pragma once

#include "../Types.h"
#include "mle/utils/ECS.h"

namespace mle::ui::system {
class Bounds final {
  public:
    MLE_NO_COPY_MOVE(Bounds);

    explicit Bounds(UI& ui);
    ~Bounds() = default;

    void update();

  private:
    void checkContainerNeedsUpdate(entt::entity e, const std::set<entt::entity>& containers_to_update);
    void updateContainer(entt::entity e);

  private:
    UI& ui_;

    entt::storage_for_t<entt::reactive>& storage_;
};
}  // namespace mle::ui::system
