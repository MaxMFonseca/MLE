#pragma once

#include "Types.h"
#include "mle/common/LiveCounter.h"

namespace mle::renderer {
class Shader : public LiveCounter<Shader> {
  public:
    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&&) = delete;

    ~Shader();

    [[nodiscard]] vk::ShaderModule getModule() const { return shader_module_; }
    [[nodiscard]] vk::ShaderStageFlagBits getStage() const { return stage_; }
    [[nodiscard]] u32 getFirstInstanceAttributeLocation() const { return first_instance_attribute_location_; }
    [[nodiscard]] const std::vector<vk::VertexInputAttributeDescription>& getVertexAttributes() const { return vertex_attributes_; }
    [[nodiscard]] const std::vector<vk::DescriptorSetLayoutBinding>& getDescriptors() const { return descriptors_; }
    [[nodiscard]] vk::PushConstantRange getPushConstantRange() const { return pc_range_; }
    [[nodiscard]] const std::vector<PushConstantField>& getPushConstantFields() const { return pc_fields_; }

    [[nodiscard]] vk::PipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo() const;
    [[nodiscard]] PipelineVertexInputState makePipelineVertexInputStateCreateInfo() const;

    [[nodiscard]] static std::vector<vk::DescriptorSetLayoutBinding> mergeDescriptors(const std::vector<vk::DescriptorSetLayoutBinding>& a,
                                                                                      const std::vector<vk::DescriptorSetLayoutBinding>& b);

    void reflect(const Bytes& spv_data);

  private:
    friend class detail::ShaderCache;
    Shader() = default;

    void setStage(vk::ShaderStageFlagBits stage) { stage_ = stage; }
    void setShaderModule(vk::ShaderModule shader_module) { shader_module_ = shader_module; }

    vk::ShaderModule shader_module_;
    vk::ShaderStageFlagBits stage_{};
    u32 first_instance_attribute_location_ = max<u32>();
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes_;
    std::vector<vk::DescriptorSetLayoutBinding> descriptors_;
    vk::PushConstantRange pc_range_;
    std::vector<PushConstantField> pc_fields_;
};
}  // namespace mle::renderer
