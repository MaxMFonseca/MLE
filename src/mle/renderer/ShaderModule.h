#pragma once

#include <map>
#include <unordered_map>

#include "Types.h"
#include "mle/common/Types.h"
#include "mle/common/Utils.h"

namespace mle::renderer {

// vertex attributes locations. VertexRate ones must be 0..n and InstanceRate ones must be n..m
// TODO: add .spv
class ShaderModule final {
  public:
    struct PushConstantField {
        std::string name;
        int offset;
        int size;
        enum class Type : i8 { FLOAT, VEC2, VEC3, VEC4, MAT2, MAT4, UNKNOWN } type;
    };

  public:
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule& operator=(ShaderModule&&) = delete;

    ~ShaderModule();

    [[nodiscard]] auto getVkHnd() const { return obj_; }
    [[nodiscard]] auto get() const { return getVkHnd(); }
    [[nodiscard]] auto getDebugName() const { return debug_name_; }
    [[nodiscard]] vk::ShaderStageFlagBits getStage() const { return stage_; }
    [[nodiscard]] const auto& getAttributes() const { return attributes_; }
    [[nodiscard]] const auto& getDescriptors() const { return descriptors_; }
    [[nodiscard]] auto getFirstInstanceAttributeLocation() const { return first_instance_attribute_location_; }
    [[nodiscard]] auto getPushConstantRange() const { return pc_range_; }
    [[nodiscard]] const auto& getPushConstantFields() const { return pc_fields_; }

    [[nodiscard]] vk::PipelineShaderStageCreateInfo makePipelineShaderStageCi() const;
    struct PipelineVertexInputState {
        std::vector<vk::VertexInputBindingDescription> binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
        vk::PipelineVertexInputStateCreateInfo ci;
    };
    [[nodiscard]] PipelineVertexInputState makePipelineVertexInputStateCreateInfo() const;

    static vk::ShaderStageFlagBits getStageOfFile(const fs::path& filepath);
    static std::vector<vk::DescriptorSetLayoutBinding> mergeDescriptors(const std::vector<vk::DescriptorSetLayoutBinding>& a,
                                                                        const std::vector<vk::DescriptorSetLayoutBinding>& b);

    static ShaderModuleRef get(const fs::path& path);
    static constexpr usize sizeOf(PushConstantField::Type t);

  private:
    explicit ShaderModule(const fs::path& filepath);

    std::string readShaderSource(const fs::path& path);
    std::vector<u32> compile(std::string_view src);
    void reflect(const std::vector<u32>& spv_data);

  private:
    std::string debug_name_;

    vk::ShaderModule obj_;
    vk::ShaderStageFlagBits stage_ = vk::ShaderStageFlagBits::eAll;
    std::vector<vk::VertexInputAttributeDescription> attributes_;
    std::vector<vk::DescriptorSetLayoutBinding> descriptors_;
    int first_instance_attribute_location_ = max<int>();
    vk::PushConstantRange pc_range_;
    std::vector<PushConstantField> pc_fields_;
};

constexpr usize ShaderModule::sizeOf(PushConstantField::Type t) {
    using T = PushConstantField::Type;
    switch (t) {
        case T::FLOAT:
            return sizeof(float);
        case T::VEC2:
            return sizeof(vec2f);
        case T::VEC3:
            return sizeof(vec3f);
        case T::VEC4:
            return sizeof(vec4f);
        case T::MAT2:
            return sizeof(mat2f);
        case T::MAT4:
            return sizeof(mat4f);
        case T::UNKNOWN: {
            MLE_UNREACHABLE_LOG("Unknown push constant field type");
        } break;
    }
    MLE_UNREACHABLE;
}
}  // namespace mle::renderer

namespace fmt {
template <>
struct formatter<mle::renderer::ShaderModule::PushConstantField::Type> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::renderer::ShaderModule::PushConstantField::Type t, FormatContext& ctx) {
        using T = mle::renderer::ShaderModule::PushConstantField::Type;
        switch (t) {
            case T::FLOAT: {
                return format_to(ctx.out(), "float");
            } break;
            case T::VEC2: {
                return format_to(ctx.out(), "vec2");
            } break;
            case T::VEC3: {
                return format_to(ctx.out(), "vec3");
            } break;
            case T::VEC4: {
                return format_to(ctx.out(), "vec4");
            } break;
            case T::MAT2: {
                return format_to(ctx.out(), "mat2");
            } break;
            case T::MAT4: {
                return format_to(ctx.out(), "mat4");
            } break;
            case T::UNKNOWN: {
                MLE_UNREACHABLE_LOG("Unknown push constant field type");
            } break;
        }
        MLE_UNREACHABLE;
    }
};
}  // namespace fmt
