#pragma once

#include "Types.h"
#include "mle/utils/File.h"
#include "vulkan/vulkan.hpp"

namespace mle {
class Shader final {
  public:
    enum class DataType : u8 { FLOAT, INT, UINT, VEC2, VEC3, VEC4, VEC4U, MAT2, MAT4, SAMPLER2D, COUNT, UNKNOWN };

    struct DescriptorSet {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        u32 set;
    };

    struct PushConstantField {
        std::string name;
        u32 offset;
        u32 size;
        DataType type;
    };

  public:
    MLE_NO_COPY_MOVE(Shader);
    ~Shader();

    [[nodiscard]] auto get() const { return o_; }
    [[nodiscard]] auto getStage() const { return stage_; }
    [[nodiscard]] auto getFirstInstanceAttributeLocation() const { return first_instance_attribute_location_; }
    [[nodiscard]] const auto& getVertexAttributes() const { return vertex_attributes_; }
    [[nodiscard]] const auto& getVertexBindings() const { return vertex_bindings_; }
    [[nodiscard]] auto getPushConstantRange() const { return pc_range_; }
    [[nodiscard]] const auto& getPushConstantFields() const { return pc_fields_; }
    [[nodiscard]] const auto& getDescriptorSets() const { return descriptor_sets_; }

    [[nodiscard]] vk::PipelineShaderStageCreateInfo makePipelineShaderStageCreateInfo() const;
    [[nodiscard]] vk::PipelineVertexInputStateCreateInfo makePipelineVertexInputStateCreateInfo() const;

    [[nodiscard]] std::vector<DescriptorSet> mergeDescriptorSets(const Shader& other) const;

    static vk::ShaderStageFlagBits stageFromExtension(const Path& file);
    static Bytes readSPV(const Path& path);
    static u32 typeSize(DataType type);
    static DataType vkFormatToDataType(vk::Format format);

  private:
    friend class ShaderCache;
    Shader() = default;
    void init(const Bytes& spv, vk::ShaderStageFlagBits stage);

    void reflect(const Bytes& spv_data);

    static void mergeSetBindings(DescriptorSet& a, const DescriptorSet& b);

  private:
    vk::ShaderModule o_;
    vk::ShaderStageFlagBits stage_{};
    u32 first_instance_attribute_location_ = max<u32>();
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes_;
    std::vector<vk::VertexInputBindingDescription> vertex_bindings_;
    vk::PushConstantRange pc_range_;
    std::vector<PushConstantField> pc_fields_;
    std::vector<DescriptorSet> descriptor_sets_;
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::Shader::DataType> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::Shader::DataType type, FormatContext& ctx) const {
        switch (type) {
            case mle::Shader::DataType::FLOAT:
                return format_to(ctx.out(), "FLOAT");
            case mle::Shader::DataType::INT:
                return format_to(ctx.out(), "INT");
            case mle::Shader::DataType::UINT:
                return format_to(ctx.out(), "UINT");
            case mle::Shader::DataType::VEC2:
                return format_to(ctx.out(), "VEC2");
            case mle::Shader::DataType::VEC3:
                return format_to(ctx.out(), "VEC3");
            case mle::Shader::DataType::VEC4:
                return format_to(ctx.out(), "VEC4");
            case mle::Shader::DataType::VEC4U:
                return format_to(ctx.out(), "VEC4U");
            case mle::Shader::DataType::MAT2:
                return format_to(ctx.out(), "MAT2");
            case mle::Shader::DataType::MAT4:
                return format_to(ctx.out(), "MAT4");
            case mle::Shader::DataType::SAMPLER2D:
                return format_to(ctx.out(), "SAMPLER2D");
            default:
                MLE_TODO;
        }
    }
};
}  // namespace fmt
