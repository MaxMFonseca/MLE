/**
 * @file
 * @brief Vulkan graphics pipeline abstraction.
 */

#pragma once

#include "Types.h"
#include "glm/fwd.hpp"
#include "mle/common/LiveCounter.h"
#include "mle/common/math/Types.h"

namespace mle::renderer {
/**
 * @brief Represents a Vulkan graphics pipeline.
 *
 * This class wraps a `vk::Pipeline` object and its associated layout,
 * managing dynamic states, push constants, blending, and other pipeline settings.
 */
class Pipeline final : public LiveCounter<Pipeline> {
  public:
    /// Configuration for creating a graphics pipeline.
    /// TODO: I hate the fact that I use vectors here...
    struct CreateInfo {
        ShaderRef vertex_shader = nullptr;    ///< Vertex shader module.
        ShaderRef fragment_shader = nullptr;  ///< Fragment shader module.
        ShaderRef compute_shader = nullptr;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;                                     ///< Primitive topology.
        vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;                                                     ///< Polygon rasterization mode.
        vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;                                                 ///< Face culling mode.
        vk::FrontFace front_face = vk::FrontFace::eClockwise;                                                      ///< Vertex winding order.
        bool line_width_dynamic = false;                                                                           ///< Whether line width is dynamic.
        std::vector<vk::Format> color_attachment_formats;                                                          ///< Color attachment formats.
        std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments;                                      ///< Blend states per attachment.
        std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};  ///< Enabled dynamic states.
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;  ///< Descriptor set layouts used by the pipeline.
        bool depth = false;                                           ///< Whether depth testing is enabled.
        bool depth_write = true;                                      ///< Whether depth writes are enabled.
        bool depth_bias = false;
    };

    using CI = CreateInfo;  ///< Alias for CreateInfo.

  public:
    Pipeline(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    /// Constructs an empty Pipeline instance. Use `init` to initialize it.
    Pipeline() = default;
    /// Destroys the pipeline and associated Vulkan objects.
    ~Pipeline();

    /**
     * @brief Creates and returns a new pipeline handle.
     *
     * @param ci Configuration for the pipeline.
     * @return A owning hnd to the created Pipeline instance.
     */
    static PipelineHnd createHnd(const CI& ci);

    /**
     * @brief Initializes the pipeline with the given configuration.
     *
     * @param ci Configuration for the pipeline.
     */
    void init(const CI& ci);

    /// Resets the pipeline to an uninitialized state.
    void reset();

    /// Returns the Vulkan pipeline object.
    [[nodiscard]] auto get() const { return o_; }
    /// Returns the Vulkan pipeline object.
    [[nodiscard]] auto getVkHnd() const { return get(); }

    /// Returns true if the pipeline uses push constants.
    [[nodiscard]] bool hasPushConstants() const { return pc_size_ > 0; }
    /// Returns the size of push constants in bytes.
    [[nodiscard]] auto getPushConstantSize() const { return pc_size_; }
    /// Returns the offset to fragment-stage push constants, or `max<u8>()` if not applicable.
    [[nodiscard]] auto getPushConstantFragOffset() const { return pc_frag_offset_; }
    /// Returns the pipeline layout used by this pipeline.
    [[nodiscard]] auto getPipelineLayout() const { return pipeline_layout_; }

    [[nodiscard]] bool hasInstance() const { return first_instance_binding_ != max<u8>(); }  ///< Returns true if the pipeline has instance attributes.
    [[nodiscard]] u8 getFirstInstanceBinding() const { return first_instance_binding_; }     ///< Returns the first instance binding index.

    [[nodiscard]] bool isCompute() const { return compute_; }  ///< Returns true if this is a compute pipeline.

    /**
     * @brief Finds a push constant field by name.
     * @param name The name of the field.
     * @return The push constant field or an empty/default-initialized one if not found.
     */
    [[nodiscard]] PushConstantField getPushConstantField(const std::string& name) const;

  private:
    /// Creates the internal pipeline layout based on the configuration.
    void createPipelineLayout(const CI& ci);

    void initGraphicsPipeline(const CI& ci);
    void initComputePipeline(const CI& ci);

  private:
    vk::Pipeline o_;                            ///< Vulkan pipeline object.
    vk::PipelineLayout pipeline_layout_;        ///< Pipeline layout object.
    std::vector<PushConstantField> pc_fields_;  ///< Push constant fields.
    u8 pc_size_ = 0;                            ///< Total size of push constants in bytes.
    u8 pc_frag_offset_ = max<u8>();             ///< Offset to fragment-stage constants (if any).
    u8 first_instance_binding_ = max<u8>();     ///< First instance binding index.
    bool compute_ = false;                      ///< True if this is a compute pipeline.
};
}  // namespace mle::renderer
