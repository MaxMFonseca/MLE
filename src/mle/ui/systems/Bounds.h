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
    void updateContainerInternalBounds(entt::entity e);
    std::set<entt::entity> fixFitContainersNeedsUpdate(const std::set<entt::entity>& containers_to_update);

  private:
    UI& ui_;

    entt::storage_for_t<entt::reactive>& external_bounds_changed_storage_;
    entt::storage_for_t<entt::reactive>& needs_internal_update_storage_;

    std::map<entt::entity, std::move_only_function<void()>> bound_providers_;
};
}  // namespace mle::ui::system
