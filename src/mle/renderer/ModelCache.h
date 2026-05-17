#pragma once

#include <string>
#include <unordered_map>

#include "mle/renderer/GLTF.h"
#include "mle/renderer/Model.h"
#include "mle/utils/ECS.h"

namespace mle {
class ModelCache {
  public:
    MLE_NO_COPY_MOVE(ModelCache);

    ~ModelCache() = default;

    ModelRef add(entt::id_type id, const GLTF& gltf);
    ModelRef addModel(const std::string& model_name);
    ModelRef get(entt::id_type id);
    [[nodiscard]] bool contains(entt::id_type id) const;

  private:
    friend class Renderer;
    ModelCache() = default;
    void init() {};
    void shutdown();

  private:
    std::unordered_map<entt::id_type, ModelHnd> models_;
};
}  // namespace mle
