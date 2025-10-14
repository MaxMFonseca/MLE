#pragma once

#include "Shader.h"
#include "Types.h"

namespace mle {
class Pipeline final {
  public:
    struct CreateInfo {
        Shader const* vertex_shader = nullptr;
        Shader const* fragment_shader = nullptr;
        Shader const* compute_shader = nullptr;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
        vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
        vk::FrontFace front_face = vk::FrontFace::eCounterClockwise;
        bool line_width_dynamic = false;
        std::span<const vk::Format> color_attachment_formats;
        std::span<const vk::PipelineColorBlendAttachmentState> blend_attachments;
        std::span<const vk::DynamicState> dynamic_states;
        bool depth = false;
        bool depth_write = true;
        bool depth_bias = false;

        std::vector<std::pair<u8, vk::DescriptorSetLayout>> external_descriptor_set_layouts;
    };

    using CI = CreateInfo;  ///< Alias for CreateInfo.
  public:
    MLE_NO_COPY_MOVE(Pipeline)

    ~Pipeline();

    void init(const CI& ci);

    [[nodiscard]] auto get() const { return o_; }
    [[nodiscard]] auto hasPushConstants() const { return pc_size_ > 0; }
    [[nodiscard]] auto getPushConstantSize() const { return pc_size_; }
    [[nodiscard]] auto getPushConstantFragOffset() const { return pc_frag_offset_; }
    [[nodiscard]] auto getPipelineLayout() const { return pipeline_layout_; }
    [[nodiscard]] auto hasInstance() const { return first_instance_binding_ != max<u8>(); }
    [[nodiscard]] auto getFirstInstanceBinding() const { return first_instance_binding_; }
    [[nodiscard]] auto isCompute() const { return compute_; }
    [[nodiscard]] const Shader::PushConstantField& getPushConstantField(std::string_view name) const;

  private:
    friend PipelineCache;
    Pipeline() = default;
    PipelineHnd static createHnd(const CI& ci);

    void createGraphicsPipeline(const CI& ci);
    void createComputePipeline(const CI& ci);

    void createPipelineLayout(const CI& ci);

  private:
    vk::Pipeline o_ = nullptr;
    vk::PipelineLayout pipeline_layout_ = nullptr;

    std::vector<Shader::PushConstantField> pc_fields_;  ///< Push constant fields.
    u8 pc_size_ = 0;                                    ///< Size of the push constant block.
    u8 pc_frag_offset_ = max<u8>();                     ///< Offset of fragment shader push constants.
    u8 first_instance_binding_ = max<u8>();             ///< First instance attribute binding location.
    bool compute_ = false;                              ///< Whether this is a compute pipeline.

    std::vector<vk::DescriptorSetLayout> owned_dsls_;  ///< Descriptor set layouts owned by this pipeline.
};
}  // namespace mle
