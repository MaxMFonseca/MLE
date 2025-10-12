#pragma once

#include "Shader.h"
#include "mle/renderer/Types.h"

namespace mle {
class ShaderCache {
  public:
    MLE_NO_COPY_MOVE(ShaderCache)
    ~ShaderCache() = default;

    const Shader& get(const std::string& name);

  private:
    friend Renderer;
    ShaderCache() = default;
    void init() {};
    void shutdown();

  private:
    std::unordered_map<std::string, ShaderHnd> shaders_;
};
}  // namespace mle
