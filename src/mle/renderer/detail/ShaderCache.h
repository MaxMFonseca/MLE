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
     * @brief Adds a shader module to the cache.
     *
     * @param name This should be <relative>/<name>.<stage> the full path will be added with addUserShaderPath or addMleShaderPath if `engine` is true.
     * relative path is relative to the user or engine shader path.
     * @param engine If true, the shader is added to the engine's shader path, otherwise to the user shader path.
     */
    ShaderRef addShader(const std::string& name, bool engine = false);

    /**
     * @brief Adds a shader module to the cache.
     *
     * @param path The full path to the shader file.
     * @param name The shader name. If empty, this will be the path striped of base and .spv.
     * ie. if default res values "res/mle/shaders/hello_world.vert.spv" will be "hello_world.vert".
     */
    ShaderRef addShader(const fs::path& path, std::string name = "");

    /**
     * @brief Retrieves a shader by name. Or tries to load it if not found.
     *
     * @param name The name of the shader to retrieve.
     * @param engine If true, the shader is retrieved from the engine's shader path, otherwise from the user shader path.
     * @return A reference to the shader.
     */
    ShaderRef get(const std::string& name, bool engine = false);

  private:
    std::unordered_map<std::string, ShaderHnd> shaders_;
};
}  // namespace mle::renderer::detail
