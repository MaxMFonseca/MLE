#include "Shader.h"

#include <spirv_reflect.h>

#include "mle/core/Unwrap.h"
#include "mle/renderer/Renderer.h"
#include "vulkan/vulkan.hpp"

namespace mle {
void Shader::init(const Bytes& spv, vk::ShaderStageFlagBits stage) {
    MLE_D("Creating shader module");

    vk::ShaderModuleCreateInfo module_ci;
    module_ci.codeSize = spv.size();
    module_ci.pCode = rAs<const u32*>(spv.data());
    o_ = unwrap(Renderer::i().vkDevice().createShaderModule(module_ci));
    stage_ = stage;

    reflect(spv);
}

Shader::~Shader() {
    if (o_) {
        Renderer::i().destroy(o_);
    }
}

vk::ShaderStageFlagBits Shader::stageFromExtension(const Path& filename) {
    auto stage_extension = filename.stem().extension();
    if (stage_extension == ".frag") {
        return vk::ShaderStageFlagBits::eFragment;
    }
    if (stage_extension == ".vert") {
        return vk::ShaderStageFlagBits::eVertex;
    }
    if (stage_extension == ".comp") {
        return vk::ShaderStageFlagBits::eCompute;
    }
    MLE_UNREACHABLE_LOG("Unsupported shader stage extension {} on file: {}, filename should be <name>.<stage>.spv", stage_extension, filename);
}

Bytes Shader::readSPV(const Path& path) {
    return unwrap(readFileBytes(path));
}

namespace {
Shader::DataType spvTypeToShaderDataType(SpvReflectTypeDescription& td) {
    using DataType = Shader::DataType;

    bool is_vector = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR);
    bool is_matrix = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX);

    bool is_float = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT);
    if (is_float) {
        if (is_vector) {
            auto vec_size = td.traits.numeric.vector.component_count;
            if (vec_size == 2) {
                return DataType::VEC2;
            }
            if (vec_size == 3) {
                return DataType::VEC3;
            }
            if (vec_size == 4) {
                return DataType::VEC4;
            }
        }
        if (is_matrix) {
            auto mat_size = td.traits.numeric.matrix.column_count;
            if (mat_size == 2) {
                return DataType::MAT2;
            }
            if (mat_size == 4) {
                return DataType::MAT4;
            }
        }
        MLE_ASSERT_LOG(td.type_flags == SPV_REFLECT_TYPE_FLAG_FLOAT, "Unsupported float type: {}", td.type_flags);
        return DataType::FLOAT;
    }

    bool is_int = as<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_INT);
    if (is_int) {
        if (td.traits.numeric.scalar.signedness) {
            return DataType::INT;
        }
        return DataType::UINT;
    }

    MLE_UNREACHABLE_LOG("Unsupported type. Fixme! {}", td.type_flags);
}

void reflectVertexInput(auto& reflection, auto& vertex_attributes_, auto& vertex_bindings_, u32& first_instance_attribute_location_) {
    std::vector<SpvReflectInterfaceVariable*> input_vars;
    u32 count = 0;
    if (reflection.EnumerateInputVariables(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
        Core::i().unrecoverable("Failed to enumerate input variables");
    }
    input_vars.resize(count);
    if (reflection.EnumerateInputVariables(&count, input_vars.data()) != SPV_REFLECT_RESULT_SUCCESS) {
        Core::i().unrecoverable("Failed to enumerate input variables");
    }
    std::ranges::sort(input_vars, [](const SpvReflectInterfaceVariable* a, const SpvReflectInterfaceVariable* b) { return a->location < b->location; });

    u32 last_location = max<u32>();
    u32 offset = 0;
    for (auto& input_var : input_vars) {
        const SpvReflectInterfaceVariable& refl_var = *input_var;
        if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
            continue;
        }

        auto& attribute = vertex_attributes_.emplace_back();
        attribute.format = as<vk::Format>(refl_var.format);
        attribute.offset = offset;
        offset += Shader::typeSize(Shader::vkFormatToDataType(attribute.format));

        attribute.location = refl_var.location;
        MLE_T("Attribute {}: location: {}, format: {}", refl_var.name, attribute.location, vk::to_string(attribute.format));

        MLE_ASSERT_LOG(!(last_location == max<u32>() && attribute.location != 0), "First attribute location must be 0");
        MLE_ASSERT_LOG(last_location == max<u32>() || last_location + 1 == attribute.location, "Attribute locations must be consecutive, last: {}, current: {}",
                       last_location, attribute.location);

        std::string var_name = refl_var.name;
        if (var_name.starts_with("ini_")) {
            if (first_instance_attribute_location_ == max<u32>()) {
                first_instance_attribute_location_ = attribute.location;
            }
        } else {
            if (first_instance_attribute_location_ != max<u32>()) {
                Core::i().unrecoverable("All instance attributes must be defined after vertex attributes, instance at {}, vertex at {}",
                                        first_instance_attribute_location_, attribute.location);
            }
        }

        last_location = attribute.location;
    }

    bool has_instance_attributes = first_instance_attribute_location_ != max<uint>();
    bool has_vertex_attributes = !has_instance_attributes || first_instance_attribute_location_ > 0;
    u32 last_vertex_attribute = has_instance_attributes ? first_instance_attribute_location_ - 1 : vertex_attributes_.size() - 1;
    u32 vert_bind_stride = 0;

    if (has_vertex_attributes) {
        auto& vertex_binding = vertex_bindings_.emplace_back();
        vertex_binding.binding = 0;
        vertex_binding.inputRate = vk::VertexInputRate::eVertex;
        vertex_binding.stride = 0;
        for (u32 i = 0; i <= last_vertex_attribute; ++i) {
            MLE_T("Vertex attribute {}: location: {}, format: {}", i, vertex_attributes_[i].location, vk::to_string(vertex_attributes_[i].format));
            vertex_binding.stride += Shader::typeSize(Shader::vkFormatToDataType(vertex_attributes_[i].format));
            vertex_attributes_[i].binding = vertex_binding.binding;
        }
        vert_bind_stride = vertex_binding.stride;
    }

    if (has_instance_attributes) {
        auto& instance_binding = vertex_bindings_.emplace_back();
        instance_binding.binding = has_vertex_attributes ? 1 : 0;
        instance_binding.inputRate = vk::VertexInputRate::eInstance;
        instance_binding.stride = 0;
        for (u32 i = first_instance_attribute_location_; i < vertex_attributes_.size(); ++i) {
            MLE_T("Instance attribute {}: location: {}, format: {}", i, vertex_attributes_[i].location, vk::to_string(vertex_attributes_[i].format));
            instance_binding.stride += Shader::typeSize(Shader::vkFormatToDataType(vertex_attributes_[i].format));
            vertex_attributes_[i].binding = instance_binding.binding;
            vertex_attributes_[i].offset -= vert_bind_stride;
        }
    }
}
}  // namespace

void Shader::reflect(const Bytes& spv_data) {
    MLE_D("SPIR-V reflection");

    spv_reflect::ShaderModule reflection(spv_data.size(), spv_data.data());
    MLE_ASSERT(reflection.GetResult() == SPV_REFLECT_RESULT_SUCCESS);

    if (stage_ == vk::ShaderStageFlagBits::eVertex) {
        reflectVertexInput(reflection, vertex_attributes_, vertex_bindings_, first_instance_attribute_location_);
    }

    u32 count = 0;
    if (reflection.EnumerateDescriptorSets(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
        Core::i().unrecoverable("Failed to enumerate descriptor sets");
    }
    if (count > 0) {
        std::vector<SpvReflectDescriptorSet*> sets;
        sets.resize(count);
        if (reflection.EnumerateDescriptorSets(&count, sets.data()) != SPV_REFLECT_RESULT_SUCCESS) {
            Core::i().unrecoverable("Failed to enumerate descriptor sets");
        }
        for (auto& set : sets) {
            auto& ds_bindings = descriptor_sets_.emplace_back();
            ds_bindings.set = set->set;

            MLE_T("Descriptor set: {}, binding_count: {}", set->set, set->binding_count);

            for (usize i = 0; i < set->binding_count; ++i) {
                auto& spvbinding = *set->bindings[i];  // NOLINT
                auto& b = ds_bindings.bindings.emplace_back();
                b.binding = spvbinding.binding;
                b.descriptorType = as<vk::DescriptorType>(spvbinding.descriptor_type);
                b.descriptorCount = spvbinding.count;
                b.stageFlags = stage_;
                b.pImmutableSamplers = nullptr;
                MLE_T("Descriptor set: {}, binding: {}, type: {}, count: {}", ds_bindings.set, b.binding, vk::to_string(b.descriptorType), b.descriptorCount);
            }

            std::ranges::sort(ds_bindings.bindings,
                              [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) { return a.binding < b.binding; });
        }

        std::ranges::sort(descriptor_sets_, [](const DescriptorSet& a, const DescriptorSet& b) { return a.set < b.set; });
    }

    std::vector<SpvReflectBlockVariable*> push_constants;
    count = 0;
    if (reflection.EnumeratePushConstantBlocks(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
        Core::i().unrecoverable("Failed to enumerate push constant blocks");
    }
    push_constants.resize(count);
    if (reflection.EnumeratePushConstantBlocks(&count, push_constants.data()) != SPV_REFLECT_RESULT_SUCCESS) {
        Core::i().unrecoverable("Failed to enumerate push constant blocks");
    }

    MLE_ASSERT_LOG(push_constants.size() <= 1, "Only one push constant block is supported");
    if (!push_constants.empty()) {
        auto& push_constant = *push_constants[0];
        pc_range_.stageFlags = stage_;
        pc_range_.offset = push_constant.offset;
        pc_range_.size = push_constant.size - push_constant.offset;
        MLE_T("Push constant block: size: {}", pc_range_.size);
        MLE_T("Push constant block: offset: {}", pc_range_.offset);
        for (u32 i = 0; i < push_constant.member_count; ++i) {
            auto& field = pc_fields_.emplace_back();
            auto& m = push_constant.members[i];  // NOLINT
            field.name = m.name;
            field.size = static_cast<int>(m.size);
            field.offset = static_cast<int>(m.offset);
            field.type = spvTypeToShaderDataType(*m.type_description);

            MLE_T("Push constant field {}: name: {}, offset: {}, size: {}, type: {}", i, field.name, field.offset, field.size, field.type);
        }
    }
}

vk::PipelineShaderStageCreateInfo Shader::makePipelineShaderStageCreateInfo() const {
    vk::PipelineShaderStageCreateInfo ret;
    ret.stage = stage_;
    ret.module = o_;
    ret.pName = "main";
    return ret;
}

vk::PipelineVertexInputStateCreateInfo Shader::makePipelineVertexInputStateCreateInfo() const {
    if (vertex_attributes_.empty()) {
        return {};
    }

    vk::PipelineVertexInputStateCreateInfo ret{};
    ret.setVertexBindingDescriptions(vertex_bindings_);
    ret.setVertexAttributeDescriptions(vertex_attributes_);

    return ret;
}

void Shader::mergeSetBindings(DescriptorSet& a, const DescriptorSet& b) {
    MLE_ASSERT(a.set == b.set);

    for (const auto& b_binding : b.bindings) {
        auto it = std::ranges::find_if(a.bindings, [&](const vk::DescriptorSetLayoutBinding& a_binding) { return a_binding.binding == b_binding.binding; });

        if (it != a.bindings.end()) {
            auto match = it->descriptorType == b_binding.descriptorType && it->descriptorCount == b_binding.descriptorCount;
            if (!match) {
                Core::i().unrecoverable("Descriptor binding mismatch on set {} binding {}, a: (type: {}, count: {}), b: (type: {}, count: {})", a.set,
                                        it->binding, vk::to_string(it->descriptorType), it->descriptorCount, vk::to_string(b_binding.descriptorType),
                                        b_binding.descriptorCount);
            }

            it->stageFlags |= b_binding.stageFlags;
        } else {
            a.bindings.push_back(b_binding);
        }
    }

    std::ranges::sort(a.bindings, [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) { return a.binding < b.binding; });
}

std::vector<Shader::DescriptorSet> Shader::mergeDescriptorSets(const Shader& other) const {
    std::vector<DescriptorSet> ret = descriptor_sets_;

    for (const auto& other_ds : other.descriptor_sets_) {
        auto found = std::ranges::find_if(ret, [&](const DescriptorSet& existing) { return existing.set == other_ds.set; });

        if (found != ret.end()) {
            mergeSetBindings(*found, other_ds);
        } else {
            ret.push_back(other_ds);
        }
    }

    std::ranges::sort(ret, [](const DescriptorSet& a, const DescriptorSet& b) { return a.set < b.set; });

    return ret;
}

u32 Shader::typeSize(DataType type) {
    switch (type) {
        case DataType::FLOAT:
        case DataType::INT:
        case DataType::UINT:
            return 4;
        case DataType::VEC2:
            return 8;
        case DataType::VEC3:
            return 12;
        case DataType::VEC4:
        case DataType::MAT2:
            return 16;
        case DataType::MAT4:
            return 64;
        default:
            MLE_UNREACHABLE_LOG("Unsupported type size query: {}", type);
    }
}

Shader::DataType Shader::vkFormatToDataType(vk::Format format) {
    switch (format) {
        case vk::Format::eR32Sfloat:
            return DataType::FLOAT;
        case vk::Format::eR32Sint:
            return DataType::INT;
        case vk::Format::eR32Uint:
            return DataType::UINT;
        case vk::Format::eR32G32Sfloat:
            return DataType::VEC2;
        case vk::Format::eR32G32B32Sfloat:
            return DataType::VEC3;
        case vk::Format::eR32G32B32A32Sfloat:
            return DataType::VEC4;
        default:
            MLE_UNREACHABLE_LOG("Unsupported format to type conversion: {}", vk::to_string(format));
    }
};
}  // namespace mle
