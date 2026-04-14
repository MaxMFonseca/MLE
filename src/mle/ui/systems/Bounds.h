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
    static void updateEntity(const Entt& ew);
    void updateTree(entt::entity root);
    void updateRoot();

  private:
    UI& ui_;

    std::map<entt::entity, std::move_only_function<void()>> bound_providers_;
};
}  // namespace mle::ui::system
