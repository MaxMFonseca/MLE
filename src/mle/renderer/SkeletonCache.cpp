#include "SkeletonCache.h"

namespace mle {
SkeletonRef SkeletonCache::addSkeleton(const std::string& skeleton_name) {
    Path path = ResPath::RES;
    path /= ResPath::MODELS;
    path /= skeleton_name;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load skeleton: {}", path.string());
        return nullptr;
    }

    auto skeleton_i = skeletons_.emplace(skeleton_name, std::make_unique<Skeleton>());
    auto& skeleton = skeleton_i.first->second;
    skeleton->loadFromGLTF(gltf);
    MLE_D("Added skeleton '{}' to SkeletonCache from file: {}", skeleton_name, path.string());
    return skeleton.get();
}

SkeletonRef SkeletonCache::getSkeleton(const std::string& skeleton_name) {
    auto it = skeletons_.find(skeleton_name);
    if (it != skeletons_.end()) {
        return it->second.get();
    }
    return nullptr;
}
}  // namespace mle
