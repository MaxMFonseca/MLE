#pragma once

#include <string>
#include <unordered_map>

#include "mle/renderer/Skeleton.h"
#include "mle/renderer/Types.h"

namespace mle {
class SkeletonCache {
  public:
    MLE_NO_COPY_MOVE(SkeletonCache);

    ~SkeletonCache() = default;

    SkeletonRef addSkeleton(const std::string& skeleton_name);
    SkeletonRef getSkeleton(const std::string& skeleton_name);

  private:
    friend class Renderer;
    SkeletonCache() = default;
    void init() {};
    void shutdown() {};

  private:
    std::unordered_map<std::string, SkeletonHnd> skeletons_;
};
}  // namespace mle
