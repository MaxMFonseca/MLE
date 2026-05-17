#include "AnimationCache.h"

#include <unordered_set>

#include "mle/core/Assert.h"

namespace mle {
entt::id_type AnimationCache::makeAnimationId(entt::id_type source_id, std::string_view animation_name) {
    const std::string key = std::to_string(source_id) + "." + std::string{animation_name};
    return entt::hashed_string::value(key.c_str());
}

std::vector<AnimationClipRef> AnimationCache::addAnimations(entt::id_type source_id, const GLTF& gltf) {
    validateAnimationNames(source_id, gltf);

    std::vector<AnimationClipRef> refs;
    refs.reserve(gltf.model().animations.size());

    for (usize i = 0; i < gltf.model().animations.size(); ++i) {
        const auto& animation = gltf.model().animations[i];
        const auto id = makeAnimationId(source_id, animation.name);

        if (auto found = animations_.find(id); found != animations_.end()) {
            refs.push_back(found->second.get());
            continue;
        }

        auto clip = std::make_unique<AnimationClip>();
        clip->loadFromGLTF(gltf, i);

        auto [it, inserted] = animations_.emplace(id, std::move(clip));
        refs.push_back(it->second.get());
        if (inserted) {
            MLE_D("Added animation '{}' id:{} to AnimationCache from source id:{}", animation.name, id, source_id);
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

    return addAnimations(entt::hashed_string::value(animation_file.c_str()), gltf);
}

AnimationClipRef AnimationCache::addAnimation(entt::id_type source_id, const GLTF& gltf, std::string_view animation_name) {
    validateAnimationNames(source_id, gltf);

    for (usize i = 0; i < gltf.model().animations.size(); ++i) {
        const auto& animation = gltf.model().animations[i];
        if (animation.name != animation_name) {
            continue;
        }

        const auto id = makeAnimationId(source_id, animation.name);
        if (auto found = animations_.find(id); found != animations_.end()) {
            return found->second.get();
        }

        auto clip = std::make_unique<AnimationClip>();
        clip->loadFromGLTF(gltf, i);

        auto [it, inserted] = animations_.emplace(id, std::move(clip));
        if (inserted) {
            MLE_D("Added animation '{}' id:{} to AnimationCache from source id:{}", animation.name, id, source_id);
        }
        return it->second.get();
    }

    MLE_E("Animation '{}' was not found in source id:{}", animation_name, source_id);
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
}

void AnimationCache::validateAnimationNames(entt::id_type source_id, const GLTF& gltf) {
    std::unordered_set<std::string> names;
    names.reserve(gltf.model().animations.size());

    for (usize i = 0; i < gltf.model().animations.size(); ++i) {
        const auto& name = gltf.model().animations[i].name;
        MLE_ASSERT_LOG(!name.empty(), "GLTF animation at source id:{} index:{} has an empty name. Animation clips must use non-empty unique names.",
                       source_id, i);
        const auto [_, inserted] = names.emplace(name);
        MLE_ASSERT_LOG(inserted, "GLTF animation source id:{} contains duplicate animation name '{}'. Animation clip names must be unique per file.",
                       source_id, name);
    }
}
}  // namespace mle
