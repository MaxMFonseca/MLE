#pragma once

#include "Types.h"
#include "mle/common/Utils.h"
#include "mle/renderer/Camera.h"

namespace mle::renderer {
class CubeMap {
  public:
    MLE_NO_COPY_MOVE(CubeMap)

    CubeMap() = default;
    ~CubeMap();

    void init(const std::string& name);
    void shutdown();

    [[nodiscard]] auto getImage() const { return o_; }
    [[nodiscard]] auto getView() const { return view_; }
    [[nodiscard]] auto ready() const { return ready_; }

    static BufferRef getIndexBuffer();
    static int getIndexCount();
    static vk::DescriptorSetLayout getDescriptorSetLayout();

  private:
    vk::Image o_;
    VmaAllocation allocation_ = {};
    VmaAllocationInfo allocation_info_ = {};
    vk::ImageView view_ = {};
    bool ready_ = false;
};
}  // namespace mle::renderer
