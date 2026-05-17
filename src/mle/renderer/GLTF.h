#pragma once

#include <tiny_gltf.h>

#include "mle/renderer/Image.h"
#include "mle/utils/File.h"

namespace mle {
class GLTF {
  public:
    Result load(const Path& path);

    [[nodiscard]] const tinygltf::Accessor& getAccessor(int idx) const;
    [[nodiscard]] const tinygltf::BufferView& getBufferView(const tinygltf::Accessor& acc) const;
    [[nodiscard]] const tinygltf::Buffer& getBuffer(const tinygltf::BufferView& view) const;

    [[nodiscard]] std::vector<vec3f> readAccessorVec3f(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<vec2f> readAccessorVec2f(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<vec4f> readAccessorVec4f(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<vec3f> readAccessorColor3f(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<quat> readAccessorQuat(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<mat4f> readAccessorMat4f(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<f32> readAccessorFloat(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] Image::RawData readImageRawData(usize image_index, bool srgb) const;

    [[nodiscard]] std::vector<vec4u> readAccessorJoints4u(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<vec4f> readAccessorWeights4f(const tinygltf::Accessor& accessor) const;
    [[nodiscard]] std::vector<u32> readIndicesU32(const tinygltf::Accessor& accessor, u32 baseVertex) const;

    [[nodiscard]] const auto& model() const { return model_; }

  private:
    tinygltf::Model model_;
};

}  // namespace mle
