#include "AnimationCache.h"

namespace mle {
std::vector<AnimationClipRef> AnimationCache::addAnimations(const std::string& animation_file, const std::string& resource_dir) {
    Path path = ResPath::RES;
    path /= resource_dir;
    path /= animation_file;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load animations: {}", path.string());
        return {};
    }

    std::vector<AnimationClipRef> refs;
    refs.reserve(gltf.model().animations.size());

    for (usize i = 0; i < gltf.model().animations.size(); ++i) {
        auto clip = std::make_unique<AnimationClip>();
        clip->loadFromGLTF(gltf, i);

        const std::string name = makeAnimationName(animation_file, *clip, i);
        auto [it, inserted] = animations_.emplace(name, std::move(clip));
        refs.push_back(it->second.get());
        if (inserted) {
            MLE_D("Added animation '{}' to AnimationCache from file: {}", name, path.string());
        }
    }

    return refs;
}

AnimationClipRef AnimationCache::addAnimation(const std::string& animation_file, usize animation_index, const std::string& resource_dir) {
    Path path = ResPath::RES;
    path /= resource_dir;
    path /= animation_file;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load animation: {}", path.string());
        return nullptr;
    }

    auto clip = std::make_unique<AnimationClip>();
    clip->loadFromGLTF(gltf, animation_index);

    const std::string name = makeAnimationName(animation_file, *clip, animation_index);
    auto [it, inserted] = animations_.emplace(name, std::move(clip));
    if (inserted) {
        MLE_D("Added animation '{}' to AnimationCache from file: {}", name, path.string());
    }
    return it->second.get();
}

AnimationClipRef AnimationCache::getAnimation(const std::string& animation_name) {
    if (animation_name.empty()) {
        return animations_.empty() ? nullptr : animations_.begin()->second.get();
    }

    auto it = animations_.find(animation_name);
    if (it != animations_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::string AnimationCache::makeAnimationName(const std::string& animation_file, const AnimationClip& clip, usize animation_index) {
    if (!clip.getName().empty()) {
        return animation_file + "." + clip.getName();
    }
    return animation_file + "." + std::to_string(animation_index);
}
}  // namespace mle
