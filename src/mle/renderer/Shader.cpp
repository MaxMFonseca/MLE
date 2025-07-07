#include "Shader.h"

#include <spirv_reflect.h>

#include "Renderer.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
namespace {
DataType spvTypeToType(SpvReflectTypeDescription& td) {
    bool is_vector = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR);
    bool is_matrix = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX);

    bool is_float = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT);
    if (is_float) {
        if (is_vector) {
            auto vec_size = td.traits.numeric.vector.component_count;
            if (vec_size == 2) {
                return DataType::VEC2F;
            }
            if (vec_size == 3) {
                return DataType::VEC3F;
            }
            if (vec_size == 4) {
                return DataType::VEC4F;
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
        return DataType::F32;
    }

    bool is_int = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_INT);
    if (is_int) {
        MLE_ASSERT_LOG(td.type_flags == SPV_REFLECT_TYPE_FLAG_INT, "Unsupported integer type: {}", td.type_flags);
        return DataType::F32;
    }

    MLE_UNREACHABLE_LOG("Unsupported type. Fixme! {}", td.type_flags);
}
}  // namespace

Shader::~Shader() {
    if (shader_module_) {
        MLE_D("Destroying shader module: {}");
        detail::getDevice().destroy(shader_module_);
    }
}

void Shader::reflect(const Bytes& spv_data) {
    MLE_D("SPIR-V reflection");

    spv_reflect::ShaderModule reflection(spv_data.size(), spv_data.data());
    MLE_ASSERT(reflection.GetResult() == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectInterfaceVariable*> input_vars;

    u32 count = 0;

    if (stage_ == vk::ShaderStageFlagBits::eVertex) {
        if (reflection.EnumerateInputVariables(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
            core::unrecoverable("Failed to enumerate input variables");
        }
        input_vars.resize(count);
        if (reflection.EnumerateInputVariables(&count, input_vars.data()) != SPV_REFLECT_RESULT_SUCCESS) {
            core::unrecoverable("Failed to enumerate input variables");
        }
        std::ranges::sort(input_vars, [](const SpvReflectInterfaceVariable* a, const SpvReflectInterfaceVariable* b) { return a->location < b->location; });

        auto last_location = max<u32>();
        u32 offset = 0;
        for (auto& input_var : input_vars) {
            const SpvReflectInterfaceVariable& refl_var = *input_var;
            if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
                continue;
            }

            auto& attribute = vertex_attributes_.emplace_back();
            attribute.format = static_cast<vk::Format>(refl_var.format);

            attribute.offset = offset;
            offset += typeSize(attribute.format);

            attribute.location = refl_var.location;
            MLE_T("Attribute {}: location: {}, format: {}", refl_var.name, attribute.location, vk::to_string(attribute.format));

            MLE_ASSERT_LOG(!(last_location == max<u32>() && attribute.location != 0), "First attribute location must be 0");
            MLE_ASSERT_LOG(last_location == max<u32>() || last_location + 1 == attribute.location,
                           "Attribute locations must be consecutive, last: {}, current: {}", last_location, attribute.location);

            std::string var_name = refl_var.name;
            if (var_name.starts_with("ini_")) {
                if (first_instance_attribute_location_ == max<u32>()) {
                    first_instance_attribute_location_ = attribute.location;
                }
            } else {
                if (first_instance_attribute_location_ != max<u32>()) {
                    core::unrecoverable("All instance attributes must be defined after vertex attributes, instance at {}, vertex at {}",
                                        first_instance_attribute_location_, attribute.location);
                }
            }

            last_location = attribute.location;
        }
    }

    // TODO: make this work
    // std::vector<SpvReflectDescriptorSet*> sets;
    // count = 0;
    // if (reflection.EnumerateDescriptorSets(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
    //     core::unrecoverable("Failed to enumerate descriptor sets");
    // }
    // sets.resize(count);
    // if (reflection.EnumerateDescriptorSets(&count, sets.data()) != SPV_REFLECT_RESULT_SUCCESS) {
    //     core::unrecoverable("Failed to enumerate descriptor sets");
    // }
    //
    // for (auto& set : sets) {
    //     MLE_T("Descriptor set: {}, binding_count: {}", set->set, set->binding_count);
    //     MLE_ASSERT_LOG(set->set == 0, "Only one descriptor set is supported (push descriptors)");
    //     for (u32 i = 0; i < set->binding_count; ++i) {
    //         auto& binding = *set->bindings[i];  // NOLINT
    //         auto& descriptor = descriptors_.emplace_back();
    //         descriptor.binding = binding.binding;
    //         descriptor.descriptorType = static_cast<vk::DescriptorType>(binding.descriptor_type);
    //         descriptor.descriptorCount = binding.count;
    //         descriptor.stageFlags = stage_;
    //         descriptor.pImmutableSamplers = nullptr;
    //     }
    // }
    // std::ranges::sort(descriptors_, [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) { return a.binding < b.binding; });
    // for (const auto& descriptor : descriptors_) {
    //     MLE_T("  Binding: {}, Descriptor type: {}, Descriptor count: {}, Stage flags: {}", descriptor.binding, vk::to_string(descriptor.descriptorType),
    //           descriptor.descriptorCount, vk::to_string(descriptor.stageFlags));
    // }

    std::vector<SpvReflectBlockVariable*> push_constants;
    count = 0;
    if (reflection.EnumeratePushConstantBlocks(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
        core::unrecoverable("Failed to enumerate push constant blocks");
    }
    push_constants.resize(count);
    if (reflection.EnumeratePushConstantBlocks(&count, push_constants.data()) != SPV_REFLECT_RESULT_SUCCESS) {
        core::unrecoverable("Failed to enumerate push constant blocks");
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
            field.type = spvTypeToType(*m.type_description);

            MLE_T("Push constant field {}: name: {}, offset: {}, size: {}, type: {}", i, field.name, field.offset, field.size, field.type);
        }
    }
}
vk::PipelineShaderStageCreateInfo Shader::getPipelineShaderStageCreateInfo() const {
    vk::PipelineShaderStageCreateInfo ret;
    ret.stage = stage_;
    ret.module = shader_module_;
    ret.pName = "main";
    return ret;
}

[[nodiscard]] PipelineVertexInputState Shader::makePipelineVertexInputStateCreateInfo() const {
    if (vertex_attributes_.empty()) {
        return {};
    }

    PipelineVertexInputState ret;
    ret.attribute_descriptions = vertex_attributes_;

    bool has_instance_attributes = first_instance_attribute_location_ != max<uint>();
    bool has_vertex_attributes = !has_instance_attributes || first_instance_attribute_location_ > 0;
    u32 last_vertex_attribute = has_instance_attributes ? first_instance_attribute_location_ - 1 : vertex_attributes_.size() - 1;

    if (has_vertex_attributes) {
        auto& vertex_binding = ret.binding_descriptions.emplace_back();
        vertex_binding.binding = 0;
        vertex_binding.inputRate = vk::VertexInputRate::eVertex;
        vertex_binding.stride = 0;
        for (u32 i = 0; i <= last_vertex_attribute; ++i) {
            MLE_T("Vertex attribute {}: location: {}, format: {}", i, vertex_attributes_[i].location, vk::to_string(vertex_attributes_[i].format));
            vertex_binding.stride += typeSize(ret.attribute_descriptions[i].format);
            ret.attribute_descriptions[i].binding = vertex_binding.binding;
        }
    }

    if (has_instance_attributes) {
        auto& instance_binding = ret.binding_descriptions.emplace_back();
        instance_binding.binding = has_vertex_attributes ? 1 : 0;
        instance_binding.inputRate = vk::VertexInputRate::eInstance;
        instance_binding.stride = 0;
        for (uint i = first_instance_attribute_location_; i < vertex_attributes_.size(); ++i) {
            MLE_T("Instance attribute {}: location: {}, format: {}", i, vertex_attributes_[i].location, vk::to_string(vertex_attributes_[i].format));
            instance_binding.stride += typeSize(ret.attribute_descriptions[i].format);
            ret.attribute_descriptions[i].binding = instance_binding.binding;
        }
    }

    ret.ci.setVertexAttributeDescriptions(ret.attribute_descriptions);
    ret.ci.setVertexBindingDescriptions(ret.binding_descriptions);

    return ret;
}

std::vector<vk::DescriptorSetLayoutBinding> Shader::mergeDescriptors(const std::vector<vk::DescriptorSetLayoutBinding>& a,
                                                                     const std::vector<vk::DescriptorSetLayoutBinding>& b) {
    if (a.empty()) {
        return b;
    }
    if (b.empty()) {
        return a;
    }

    std::vector<vk::DescriptorSetLayoutBinding> ret = a;

    for (const auto& b_binding : b) {
        auto it = std::ranges::find_if(ret, [&b_binding](const vk::DescriptorSetLayoutBinding& a_binding) { return a_binding.binding == b_binding.binding; });
        if (it != ret.end()) {
            MLE_ASSERT_LOG(it->descriptorType == b_binding.descriptorType, "Descriptor type mismatch");
            MLE_ASSERT_LOG(it->descriptorCount == b_binding.descriptorCount, "Descriptor count mismatch");
            it->stageFlags |= b_binding.stageFlags;
        } else {
            ret.push_back(b_binding);
        }
    }
    std::ranges::sort(ret, [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) { return a.binding < b.binding; });

    return ret;
}

}  // namespace mle::renderer

// TODO: maybe reimplement glslang shader compilation here and allow the engine user to opt-in to it.
// Or even better.. use SLANG
// std::string ShaderModule::readShaderSource(const fs::path& path) {
//     if (!fs::exists(path)) {
//         MLE_THROW(RESOURCE_NOT_FOUND, "Shader source file not found: {}", path);
//     }
//
//     std::ifstream file(path);
//     std::string ret;
//     for (std::string line; std::getline(file, line);) {
//         if (line.starts_with('#')) {
//             if (line.starts_with("#include")) {
//                 std::string mod = line.substr(10);
//                 mod.pop_back();
//                 fs::path include_path;
//                 if (mod.starts_with("~")) {
//                     include_path = mle::addShadersBasePath(mod);
//                 } else {
//                     include_path = path.parent_path() / mod;
//                 }
//                 ret += readShaderSource(include_path);
//                 continue;
//             }
//
//             if (line.starts_with("#instance")) {
//                 first_instance_attribute_location_ = std::stoi(line.substr(10));
//                 continue;
//             }
//         }
//         ret += line + '\n';
//     }
//     return ret;
// }
//
// std::vector<u32> ShaderModule::compile(std::string_view src) {
//     glslang::InitializeProcess();
//
//     EShLanguage env_stage = {};
//
//     switch (stage_) {
//         case vk::ShaderStageFlagBits::eVertex:
//             env_stage = EShLangVertex;
//             break;
//         case vk::ShaderStageFlagBits::eFragment:
//             env_stage = EShLangFragment;
//             break;
//         case vk::ShaderStageFlagBits::eCompute:
//             env_stage = EShLangCompute;
//             break;
//         default:
//             MLE_UNREACHABLE_LOG("Unsupported shader stage: {}", vk::to_string(stage_));
//     }
//
//     MLE_T("Compiling {} shader module: {}\n{}", vk::to_string(stage_), debug_name_, src);
//
//     glslang::TShader shader{env_stage};
//     const char* const src_char = src.data();
//     shader.setStrings(&src_char, 1);
//     shader.setEnvInput(glslang::EShSourceGlsl, env_stage, glslang::EShClientVulkan, 450);
//     shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
//     shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
//     shader.setEntryPoint("main");
//     shader.setSourceEntryPoint("main");
//
//     MLE_ASSERT_LOG(shader.parse(GetDefaultResources(), 450, false, EShMsgDefault), "Shader compilation error:\n{}", shader.getInfoLog());
//
//     glslang::TProgram program;
//     program.addShader(&shader);
//
//     MLE_ASSERT(program.link(EShMsgDefault));
//
//     std::vector<u32> spv_data;
//     spv::SpvBuildLogger spv_logger;
//     glslang::SpvOptions spv_options;
//     spv_options.disableOptimizer = false;
//     spv_options.optimizeSize = true;
//
//     GlslangToSpv(*program.getIntermediate(env_stage), spv_data, &spv_logger, &spv_options);
//
//     bool compilation_error = false;
//     std::string spv_logger_msg;
//     spv_logger.error(spv_logger_msg);
//     while (!spv_logger_msg.empty()) {
//         compilation_error = true;
//         MLE_E("Spirv log error:\n{}", spv_logger_msg);
//         spv_logger_msg.clear();
//         spv_logger.error(spv_logger_msg);
//     }
//     while (!spv_logger_msg.empty()) {
//         MLE_W("Spirv log warn:\n{}", spv_logger_msg);
//         spv_logger_msg.clear();
//         spv_logger.warning(spv_logger_msg);
//     }
//
//     MLE_ASSERT(!compilation_error);
//
//     glslang::FinalizeProcess();
//
//     return spv_data;
// }
