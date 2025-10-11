#include "ShaderCache.h"

#include "Shader.h"
#include "mle/core/Unwrap.h"
#include "mle/utils/File.h"

namespace mle {
ShaderRef ShaderCache::get(const std::string& name) {
    auto found = shaders_.find(name);
    if (found != shaders_.end()) {
        return found->second.get();
    }

    Path path = ResPath::RES;
    path /= ResPath::SHADERS;
    path /= name;
    path += ".spv";

    auto bytes = unwrap(readFileBytes(path));
    ShaderHnd shader{new Shader()};

    shader->init(bytes, Shader::stageFromExtension(path));

    auto emplace_r = shaders_.emplace(name, std::move(shader));

    return emplace_r.first->second.get();
}

void ShaderCache::shutdown() {
    MLE_I("Shutting down shader cache");
    shaders_.clear();
}
}  // namespace mle
