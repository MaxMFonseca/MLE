#include "ShaderCache.h"

#include <vulkan/vulkan_handles.hpp>

#include "mle/common/Utils.h"
#include "mle/core/Core.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Shader.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer::detail {
namespace {
// This function validates the shader file name and extracts the stage from the file extension.
vk::ShaderStageFlagBits getFileStage(const fs::path& filepath) {
    MLE_ASSERT_LOG(filepath.extension() == ".spv", "For now only spv files are supported");
    auto stage_extension = filepath.stem().extension();
    if (stage_extension == ".frag") {
        return vk::ShaderStageFlagBits::eFragment;
    }
    if (stage_extension == ".vert") {
        return vk::ShaderStageFlagBits::eVertex;
    }
    if (stage_extension == ".comp") {
        return vk::ShaderStageFlagBits::eCompute;
    }
    MLE_UNREACHABLE_LOG("Unsupported shader stage on file: {}, filename should be <name>.<stage>.spv", filepath.c_str());
}
}  // namespace

void ShaderCache::init() {  // NOLINT TODO: create engine shaders?
    MLE_I("Initializing shader cache");
}

void ShaderCache::reset() {
    MLE_I("Shutting down shader cache");
    shaders_.clear();
}

ShaderRef ShaderCache::addShader(const std::string& name, bool engine) {
    fs::path path = name;
    if (engine) {
        path = res::addMleShaderPath(name);
    } else {
        path = res::addUserShaderPath(name);
    }
    path += ".spv";
    return addShader(path, name);
}

ShaderRef ShaderCache::addShader(const fs::path& path, std::string name) {
    MLE_D("Adding shader from file: {}", path.generic_string());

    auto stage = getFileStage(path);  // This also validates the name

    if (name.empty()) {
        auto no_spv = path;
        no_spv.replace_extension("");
        name = res::removeBasePath(no_spv.string());
        MLE_D("Shader name not provided, using: {}", name);
    } else {
        MLE_D("Shader name provided: {}", name);
    }

    auto file = readFile(path, true);
    if (!file) {
        core::unrecoverable("Failed to read shader file: {}", path.generic_string());
    }

    vk::ShaderModuleCreateInfo module_ci;
    module_ci.codeSize = file.value().size();
    module_ci.pCode = rAs<const u32*>(file.value().data());

    vk::ShaderModule shader_module = unwrap(detail::getDevice().createShaderModule(module_ci));

    auto entry = std::unique_ptr<Shader>(new Shader());
    entry->setShaderModule(shader_module);
    entry->setStage(stage);
    entry->reflect(file.value());

    MLE_ASSERT_LOG(!shaders_.contains(name), "Shader with name '{}' already exists in cache", name);
    auto r = shaders_.emplace(name, std::move(entry));

    return r.first->second.get();
}

ShaderRef ShaderCache::get(const std::string& name, bool engine) {
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        return it->second.get();
    }

    MLE_D("Shader '{}' not found in cache, attempting to load", name);
    return addShader(name, engine);
}
}  // namespace mle::renderer::detail
