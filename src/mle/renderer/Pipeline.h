#pragma once

#include "Types.h"
#include "mle/common/Types.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/renderer/ShaderModule.h"

// FIXME: set dirty only when needed
namespace mle::renderer {
class Pipeline final {
  public:
    struct ExecInfo {
        Rectf viewport{};
        BufferRef vertex_buffer = nullptr;
        usize vertex_buffer_offset = 0;
        BufferRef instance_buffer = nullptr;
        usize instance_buffer_offset = 0;
        BufferRef index_buffer = nullptr;
        usize index_buffer_offset = 0;
        const void* push_constants = nullptr;
        u32 index_count = 0;
        u32 instance_count = 0;
        std::vector<vk::WriteDescriptorSet> descriptor_sets;
    };

  public:
    Pipeline(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    ~Pipeline();

    void build();

    void exec(const ExecInfo& exec_info);

    void setDirty() { dirty_ = true; }
    [[nodiscard]] bool isDirty() const { return dirty_; }

    Pipeline& setVertexShader(ShaderModuleRef shader) {
        layout_changed_ = true;
        vertex_shader_ = shader;
        dirty_ = true;
        return *this;
    }

    Pipeline& setVertexShader(const fs::path& path) { return setVertexShader(ShaderModule::get(path)); }

    Pipeline& setFragmentShader(ShaderModuleRef shader) {
        layout_changed_ = true;
        fragment_shader_ = shader;
        dirty_ = true;
        return *this;
    }

    Pipeline& setFragmentShader(const fs::path& path) { return setFragmentShader(ShaderModule::get(path)); }

    Pipeline& setTopology(vk::PrimitiveTopology topology) {
        topology_ = topology;
        dirty_ = true;
        return *this;
    }
    Pipeline& setPolygonMode(vk::PolygonMode mode) {
        polygon_mode_ = mode;
        dirty_ = true;
        return *this;
    }
    Pipeline& setColorAttachmentFormats(std::vector<vk::Format> formats) {
        color_attachment_formats_ = std::move(formats);
        dirty_ = true;
        return *this;
    }
    Pipeline& addColorAttachmentFormat(vk::Format format) {
        color_attachment_formats_.push_back(format);
        dirty_ = true;
        return *this;
    }
    Pipeline& setDepth(bool depth) {
        depth_ = depth;
        dirty_ = true;
        return *this;
    }
    Pipeline& setBlendAttachments(std::vector<vk::PipelineColorBlendAttachmentState>&& attachments) {
        blend_attachments_ = std::move(attachments);
        dirty_ = true;
        return *this;
    }
    Pipeline& addDynamicState(vk::DynamicState state) {
        dynamic_states_.push_back(state);
        dirty_ = true;
        return *this;
    }
    Pipeline& setOverrideDescriptorSetLayoutCount(usize set, usize count) {
        layout_changed_ = true;
        if (override_descriptor_binding_count_.size() <= set) {
            override_descriptor_binding_count_.resize(set + 1, 0);
        }
        override_descriptor_binding_count_[set] = count;
        dirty_ = true;
        return *this;
    }

    [[nodiscard]] auto getPipeline() const { return pipeline_; }
    [[nodiscard]] auto get() const { return pipeline_; }

    [[nodiscard]] ShaderModule::PushConstantField getPushConstantField(const std::string& name) const;

    [[nodiscard]] auto getDebugName() const { return debug_name_; }

    [[nodiscard]] static PipelineGetResult get(const std::string& name);

  private:
    explicit Pipeline(std::string&& debug_name);

    void createPipelineLayout();

  private:
    std::string debug_name_;

    ShaderModuleRef vertex_shader_{};
    ShaderModuleRef fragment_shader_{};

    vk::PrimitiveTopology topology_ = vk::PrimitiveTopology::eTriangleList;

    vk::PolygonMode polygon_mode_ = vk::PolygonMode::eFill;
    vk::CullModeFlags cull_mode_ = vk::CullModeFlagBits::eNone;
    vk::FrontFace front_face_ = vk::FrontFace::eClockwise;
    bool line_width_dynamic_ = false;

    std::vector<vk::Format> color_attachment_formats_;
    std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments_;
    std::vector<vk::DynamicState> dynamic_states_ = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    bool depth_ = false;

    std::vector<usize> override_descriptor_binding_count_;

    int pc_size_ = 0;
    int pc_f_offset_ = -1;
    std::vector<ShaderModule::PushConstantField> pc_fields_;

    bool dirty_ = true;
    bool layout_changed_ = true;

    vk::PipelineLayout pipeline_layout_;
    vk::Pipeline pipeline_;
    vk::DescriptorSetLayout descriptor_set_layout_;
};
}  // namespace mle::renderer
