#pragma once

#include "../Types.h"
#include "mle/ui/components/Container.h"
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
    std::pair<std::vector<entt::entity>, std::vector<entt::entity>> separateFlexFromListChildren(std::span<const EntityStorage::Entry> span);
    std::vector<entt::entity> sortChildrenByDependency(std::span<const entt::entity> span);

  private:
    UI& ui_;

    entt::storage_for_t<entt::reactive>& storage_;
};
}  // namespace mle::ui::system
