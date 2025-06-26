#pragma once

#include "../Types.h"
#include "mle/common/Utils.h"

namespace mle::renderer::detail {
class ShaderCache {
  public:
    MLE_NO_COPY_MOVE(ShaderCache)

    ShaderCache() = default;
    ~ShaderCache() = default;

    void init();
    void reset();

    /**
     * @brief Retrieves a shader by name. Or tries to load it if not found.
     *
     * @param name The name of the shader to retrieve.
     * @return A reference to the shader.
     */
    ShaderRef get(const std::string& name);

  private:
    /**
     * @brief Adds a shader module to the cache.
     *
     * @param path The full path to the shader file.
     * @param name The shader name. If empty, this will be the path striped of base and .spv.
     * ie. if default res values "res/mle/shaders/hello_world.vert.spv" will be "hello_world.vert".
     */
    ShaderRef addShader(std::string name);

  private:
    std::unordered_map<std::string, ShaderHnd> shaders_;
};
}  // namespace mle::renderer::detail
