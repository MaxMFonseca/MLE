#include "Renderer.h"

#include "VkCtx.h"
#include "mle/core/Consts.h"
#include "mle/renderer/GLTF.h"

namespace mle {
void Renderer::init() {
    MLE_I("Initializing Renderer");

    vk_ctx_.init();
    command_manager_.init();
    sync_manager_.init();
    frame_renderer_.init();
    shader_cache_.init();
    pipeline_cache_.init();
    texture_cache_.init();
    font_cache_.init();
    model_cache_.init();
    skeleton_cache_.init();
    animation_cache_.init();

    MLE_I("Renderer initialized successfully");
}

void Renderer::stop() {
    MLE_I("Stopping Renderer...");
    frame_renderer_.stopRun();
    frame_renderer_.waitStoped();
    MLE_I("Renderer stopped.");
}

void Renderer::shutdown() {
    auto device = vk_ctx_.getDevice();

    if (!device) {
        return;
    }

    command_manager_.waitDeviceIdle();

    animation_cache_.shutdown();
    skeleton_cache_.shutdown();
    model_cache_.shutdown();
    font_cache_.shutdown();
    texture_cache_.shutdown();
    pipeline_cache_.shutdown();
    shader_cache_.shutdown();
    sync_manager_.shutdown();
    command_manager_.shutdown();
    frame_renderer_.shutdown();
    vk_ctx_.shutdown();
}

void Renderer::addModelPack(const std::string& name) {
    Path path = ResPath::RES;
    path /= ResPath::MODELS;
    path /= name;
    if (path.extension() == "") {
        path += ".glb";
    }

    GLTF gltf;
    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load model pack: {}", path.string());
        return;
    }

    // 1. Add all animations
    auto clips = animation_cache_.addAnimations(name, gltf);
    if (!clips.empty()) {
        // Alias base filename to the first animation
        const auto pack_id = entt::hashed_string::value(name.c_str());
        const auto target_id = AnimationCache::makeAnimationId(name, clips[0]->getName());
        animation_cache_.addAlias(pack_id, target_id);
    }

    const auto& model = gltf.model();
    const int scene_idx = model.defaultScene > -1 ? model.defaultScene : 0;
    const auto& scene = model.scenes[scene_idx];

    // 2. Add all top-level nodes as models
    bool first_model = true;
    for (int node_idx : scene.nodes) {
        const auto& node = model.nodes[node_idx];
        if (!node.name.empty()) {
            std::string id_str = name + "#" + node.name;
            const auto id = entt::hashed_string::value(id_str.c_str());
            model_cache_.add(id, gltf, static_cast<usize>(node_idx));

            if (first_model) {
                const auto pack_id = entt::hashed_string::value(name.c_str());
                model_cache_.addAlias(pack_id, id);
                first_model = false;
            }
        }
    }
}
}  // namespace mle
