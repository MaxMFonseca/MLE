#include "AnimationCache.h"

#include <unordered_set>

#include "mle/core/Assert.h"
#include "mle/renderer/Model.h"

namespace mle {
entt::id_type AnimationCache::makeAnimationId(std::string_view source_name, std::string_view animation_name) {
    const std::string key = std::string{source_name} + "." + std::string{animation_name};
    return entt::hashed_string::value(key.c_str());
}

std::vector<AnimationClipRef> AnimationCache::addAnimations(std::string_view source_name, const GLTF& gltf) {
    validateAnimationNames(source_name, gltf);

    std::vector<AnimationClipRef> refs;
    refs.reserve(gltf.model().animations.size());

    for (usize i = 0; i < gltf.model().animations.size(); ++i) {
        const auto& animation = gltf.model().animations[i];
        const auto id = makeAnimationId(source_name, animation.name);

        if (auto found = animations_.find(id); found != animations_.end()) {
            refs.push_back(found->second.get());
            continue;
        }

        auto clip = std::make_unique<AnimationClip>();
        clip->loadFromGLTF(gltf, i);

        auto [it, inserted] = animations_.emplace(id, std::move(clip));
        refs.push_back(it->second.get());
        if (inserted) {
            MLE_D("Added animation '{}' id:{} to AnimationCache from source:{}", animation.name, id, source_name);
        }
    }

    return refs;
}

std::vector<AnimationClipRef> AnimationCache::addAnimations(const std::string& animation_file, const std::string& resource_dir) {
    Path path = ResPath::RES;
    path /= resource_dir;
    path /= animation_file;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load animations: {}", path.string());
        return {};
    }

    return addAnimations(animation_file, gltf);
}

AnimationClipRef AnimationCache::addAnimation(std::string_view source_name, const GLTF& gltf, std::string_view animation_name) {
    validateAnimationNames(source_name, gltf);

    for (usize i = 0; i < gltf.model().animations.size(); ++i) {
        const auto& animation = gltf.model().animations[i];
        if (animation.name != animation_name) {
            continue;
        }

        const auto id = makeAnimationId(source_name, animation.name);
        if (auto found = animations_.find(id); found != animations_.end()) {
            return found->second.get();
        }

        auto clip = std::make_unique<AnimationClip>();
        clip->loadFromGLTF(gltf, i);

        auto [it, inserted] = animations_.emplace(id, std::move(clip));
        if (inserted) {
            MLE_D("Added animation '{}' id:{} to AnimationCache from source:{}", animation.name, id, source_name);
        }
        return it->second.get();
    }

    MLE_E("Animation '{}' was not found in source:{}", animation_name, source_name);
    return nullptr;
}

AnimationClipRef AnimationCache::get(entt::id_type id) {
    auto it = animations_.find(id);
    if (it != animations_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool AnimationCache::contains(entt::id_type id) const {
    return animations_.contains(id);
}

void AnimationCache::shutdown() {
    animations_.clear();
    bindings_.clear();
}

const AnimationBinding& AnimationCache::getBinding(ModelRef model, AnimationClipRef clip) {
    const BindingKey key{.model = model, .clip = clip};
    if (auto it = bindings_.find(key); it != bindings_.end()) {
        return it->second;
    }

    AnimationBinding binding;
    const auto& anim_nodes = clip->getNodes();
    binding.channel_to_node_map.reserve(anim_nodes.size());

    for (const auto& node_anim : anim_nodes) {
        binding.channel_to_node_map.push_back(model->getNodeIdxByName(node_anim.target_name));
    }

    auto [it, inserted] = bindings_.emplace(key, std::move(binding));
    return it->second;
}

void AnimationCache::validateAnimationNames([[maybe_unused]] std::string_view source_name, const GLTF& gltf) {
    std::unordered_set<std::string> names;
    names.reserve(gltf.model().animations.size());

    for (u32 i = 0; i < gltf.model().animations.size(); ++i) { const auto& animation = gltf.model().animations[i];
        const auto& name = animation.name;
        MLE_ASSERT_LOG(!name.empty(), "GLTF animation at source:{} index:{} has an empty name. Animation clips must use non-empty unique names.", source_name,
                       i);
        const auto [_, inserted] = names.emplace(name);
        MLE_ASSERT_LOG(inserted, "GLTF animation source:{} contains duplicate animation name '{}'. Animation clip names must be unique per file.", source_name,
                       name);
    }
}
}  // namespace mle
