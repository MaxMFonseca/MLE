#include "ShaderModule.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_reflect.h>

#include <vector>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Exception.h"
#include "mle/common/Result.h"
#include "mle/common/Types.h"
#include "mle/common/Utils.h"
#include "mle/renderer/Renderer.h"

namespace mle::renderer {
namespace {
ShaderModule::PushConstantField::Type typeDescriptionToType(SpvReflectTypeDescription& td) {
    using T = ShaderModule::PushConstantField::Type;
    auto is_float = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT);
    MLE_ASSERT_LOG(is_float, "Only float types are supported in push constant fields.");  // TODO: MAYBE: this
    auto is_vector = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR);
    auto is_matrix = static_cast<bool>(td.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX);
    if (!(is_vector || is_matrix)) {
        return T::FLOAT;
    }
    if (is_vector) {
        auto vec_size = td.traits.numeric.vector.component_count;
        if (vec_size == 2) {
            return T::VEC2;
        }
        if (vec_size == 3) {
            return T::VEC3;
        }
        if (vec_size == 4) {
            return T::VEC4;
        }
    }
    if (is_matrix) {
        auto mat_size = td.traits.numeric.matrix.column_count;
        if (mat_size == 2) {
            return T::MAT2;
        }
        if (mat_size == 4) {
            return T::MAT4;
        }
    }
    MLE_UNREACHABLE_TODO;
}
}  // namespace

vk::ShaderStageFlagBits ShaderModule::getStageOfFile(const fs::path& filepath) {
    auto stage_extension = filepath.extension();
    if (stage_extension == ".frag") {
        return vk::ShaderStageFlagBits::eFragment;
    }
    if (stage_extension == ".vert") {
        return vk::ShaderStageFlagBits::eVertex;
    }
    if (stage_extension == ".comp") {
        return vk::ShaderStageFlagBits::eCompute;
    }
    MLE_UNREACHABLE_LOG("Unsupported shader stage: {}", stage_extension.generic_string());
}

ShaderModule::ShaderModule(const fs::path& path) :
    debug_name_(path.generic_string()),
    stage_(getStageOfFile(path)) {
    MLE_D("Creating {} shader module: {}", vk::to_string(stage_), debug_name_);

    // TODO: if(filepath.extension() == ".spv") { read spv file directly }
    auto spv_data = compile(readShaderSource(path));

    vk::ShaderModuleCreateInfo module_ci;
    module_ci.codeSize = spv_data.size() * sizeof(u32);
    module_ci.pCode = spv_data.data();

    obj_ = getVk().getDevice().createShaderModule(module_ci);

    reflect(spv_data);
}

ShaderModule::~ShaderModule() {
    MLE_D("Destroying {} shader module: {}", vk::to_string(stage_), debug_name_);
    getVk().getDevice().destroy(obj_);
}

std::string ShaderModule::readShaderSource(const fs::path& path) {
    if (!fs::exists(path)) {
        MLE_THROW(RESOURCE_NOT_FOUND, "Shader source file not found: {}", path);
    }

    std::ifstream file(path);
    std::string ret;
    for (std::string line; std::getline(file, line);) {
        if (line.starts_with('#')) {
            if (line.starts_with("#include")) {
                std::string mod = line.substr(10);
                mod.pop_back();
                fs::path include_path;
                if (mod.starts_with("~")) {
                    include_path = mle::addShadersBasePath(mod);
                } else {
                    include_path = path.parent_path() / mod;
                }
                ret += readShaderSource(include_path);
                continue;
            }

            if (line.starts_with("#instance")) {
                first_instance_attribute_location_ = std::stoi(line.substr(10));
                continue;
            }
        }
        ret += line + '\n';
    }
    return ret;
}

std::vector<u32> ShaderModule::compile(std::string_view src) {
    glslang::InitializeProcess();

    EShLanguage env_stage = {};

    switch (stage_) {
        case vk::ShaderStageFlagBits::eVertex:
            env_stage = EShLangVertex;
            break;
        case vk::ShaderStageFlagBits::eFragment:
            env_stage = EShLangFragment;
            break;
        case vk::ShaderStageFlagBits::eCompute:
            env_stage = EShLangCompute;
            break;
        default:
            MLE_UNREACHABLE_LOG("Unsupported shader stage: {}", vk::to_string(stage_));
    }

    MLE_T("Compiling {} shader module: {}\n{}", vk::to_string(stage_), debug_name_, src);

    glslang::TShader shader{env_stage};
    const char* const src_char = src.data();
    shader.setStrings(&src_char, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, env_stage, glslang::EShClientVulkan, 450);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    shader.setEntryPoint("main");
    shader.setSourceEntryPoint("main");

    MLE_ASSERT_LOG(shader.parse(GetDefaultResources(), 450, false, EShMsgDefault), "Shader compilation error:\n{}", shader.getInfoLog());

    glslang::TProgram program;
    program.addShader(&shader);

    MLE_ASSERT(program.link(EShMsgDefault));

    std::vector<u32> spv_data;
    spv::SpvBuildLogger spv_logger;
    glslang::SpvOptions spv_options;
    spv_options.disableOptimizer = false;
    spv_options.optimizeSize = true;

    GlslangToSpv(*program.getIntermediate(env_stage), spv_data, &spv_logger, &spv_options);

    bool compilation_error = false;
    std::string spv_logger_msg;
    spv_logger.error(spv_logger_msg);
    while (!spv_logger_msg.empty()) {
        compilation_error = true;
        MLE_E("Spirv log error:\n{}", spv_logger_msg);
        spv_logger_msg.clear();
        spv_logger.error(spv_logger_msg);
    }
    while (!spv_logger_msg.empty()) {
        MLE_W("Spirv log warn:\n{}", spv_logger_msg);
        spv_logger_msg.clear();
        spv_logger.warning(spv_logger_msg);
    }

    MLE_ASSERT(!compilation_error);

    glslang::FinalizeProcess();

    return spv_data;
}

vk::PipelineShaderStageCreateInfo ShaderModule::makePipelineShaderStageCi() const {
    vk::PipelineShaderStageCreateInfo ret;
    ret.stage = stage_;
    ret.module = obj_;
    ret.pName = "main";
    return ret;
}

void ShaderModule::reflect(const std::vector<u32>& spv_data) {
    spv_reflect::ShaderModule reflection(spv_data.size() * sizeof(u32), spv_data.data());
    MLE_ASSERT(reflection.GetResult() == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectInterfaceVariable*> input_vars;

    u32 count = 0;

    if (stage_ == vk::ShaderStageFlagBits::eVertex) {
        if (reflection.EnumerateInputVariables(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
            MLE_THROW(VK_ERROR, "Failed to enumerate input variables");
        }
        input_vars.resize(count);
        if (reflection.EnumerateInputVariables(&count, input_vars.data()) != SPV_REFLECT_RESULT_SUCCESS) {
            MLE_THROW(VK_ERROR, "Failed to enumerate input variables");
        }
        std::ranges::sort(input_vars, [](const SpvReflectInterfaceVariable* a, const SpvReflectInterfaceVariable* b) { return a->location < b->location; });

        auto last_location = max<usize>();
        usize offset = 0;
        for (auto& input_var : input_vars) {
            const SpvReflectInterfaceVariable& refl_var = *input_var;
            if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
                continue;
            }

            auto& attribute = attributes_.emplace_back();
            attribute.format = static_cast<vk::Format>(refl_var.format);

            attribute.offset = offset;
            offset += typeSize(attribute.format);

            attribute.location = refl_var.location;
            MLE_T("Attribute {}: location: {}, format: {}", refl_var.name, attribute.location, vk::to_string(attribute.format));

            MLE_ASSERT_LOG(!(last_location == max<usize>() && attribute.location != 0), "First attribute location must be 0");

            last_location = attribute.location;
        }
    }

    std::vector<SpvReflectDescriptorSet*> sets;
    count = 0;
    if (reflection.EnumerateDescriptorSets(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
        MLE_THROW(VK_ERROR, "Failed to enumerate descriptor sets");
    }
    sets.resize(count);
    if (reflection.EnumerateDescriptorSets(&count, sets.data()) != SPV_REFLECT_RESULT_SUCCESS) {
        MLE_THROW(VK_ERROR, "Failed to enumerate descriptor sets");
    }

    for (auto& set : sets) {
        MLE_T("Descriptor set: {}, binding_count: {}", set->set, set->binding_count);
        MLE_ASSERT_LOG(set->set == 0, "Only one descriptor set is supported (push descriptors)");
        for (u32 i = 0; i < set->binding_count; ++i) {
            auto& binding = *set->bindings[i];  // NOLINT
            auto& descriptor = descriptors_.emplace_back();
            descriptor.binding = binding.binding;
            descriptor.descriptorType = static_cast<vk::DescriptorType>(binding.descriptor_type);
            descriptor.descriptorCount = binding.count;
            descriptor.stageFlags = stage_;
            descriptor.pImmutableSamplers = nullptr;
        }
    }
    std::ranges::sort(descriptors_, [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) { return a.binding < b.binding; });
    for (const auto& descriptor : descriptors_) {
        MLE_T("  Binding: {}, Descriptor type: {}, Descriptor count: {}, Stage flags: {}", descriptor.binding, vk::to_string(descriptor.descriptorType),
              descriptor.descriptorCount, vk::to_string(descriptor.stageFlags));
    }

    std::vector<SpvReflectBlockVariable*> push_constants;
    count = 0;
    if (reflection.EnumeratePushConstantBlocks(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
        MLE_THROW(VK_ERROR, "Failed to enumerate push constant blocks");
    }
    push_constants.resize(count);
    if (reflection.EnumeratePushConstantBlocks(&count, push_constants.data()) != SPV_REFLECT_RESULT_SUCCESS) {
        MLE_THROW(VK_ERROR, "Failed to enumerate push constant blocks");
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
            field.type = typeDescriptionToType(*m.type_description);

            MLE_T("Push constant field {}: name: {}, offset: {}, size: {}, type: {}", i, field.name, field.offset, field.size, field.type);
        }
    }
}

[[nodiscard]] ShaderModule::PipelineVertexInputState ShaderModule::makePipelineVertexInputStateCreateInfo() const {
    if (attributes_.empty()) {
        return {};
    }

    PipelineVertexInputState ret;
    ret.attribute_descriptions = attributes_;

    bool has_vertex_attributes = first_instance_attribute_location_ != 0;
    bool has_instance_attributes = first_instance_attribute_location_ != max<int>();
    usize last_vertex_attribute = has_instance_attributes ? first_instance_attribute_location_ : attributes_.size();

    if (has_vertex_attributes) {
        auto& vertex_binding = ret.binding_descriptions.emplace_back();
        vertex_binding.binding = 0;
        vertex_binding.inputRate = vk::VertexInputRate::eVertex;
        vertex_binding.stride = 0;
        for (usize i = 0; i < last_vertex_attribute; ++i) {
            MLE_T("Vertex attribute {}: location: {}, format: {}", i, attributes_[i].location, vk::to_string(attributes_[i].format));
            vertex_binding.stride += typeSize(ret.attribute_descriptions[i].format);
            ret.attribute_descriptions[i].binding = vertex_binding.binding;
        }
    }

    if (has_instance_attributes) {
        auto& instance_binding = ret.binding_descriptions.emplace_back();
        instance_binding.binding = has_vertex_attributes ? 1 : 0;
        instance_binding.inputRate = vk::VertexInputRate::eInstance;
        instance_binding.stride = 0;
        for (usize i = first_instance_attribute_location_; i < attributes_.size(); ++i) {
            MLE_T("Instance attribute {}: location: {}, format: {}", i, attributes_[i].location, vk::to_string(attributes_[i].format));
            instance_binding.stride += typeSize(ret.attribute_descriptions[i].format);
            ret.attribute_descriptions[i].binding = instance_binding.binding;
        }
    }

    ret.ci.setVertexAttributeDescriptions(ret.attribute_descriptions);
    ret.ci.setVertexBindingDescriptions(ret.binding_descriptions);

    return ret;
}

std::vector<vk::DescriptorSetLayoutBinding> ShaderModule::mergeDescriptors(const std::vector<vk::DescriptorSetLayoutBinding>& a,
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

ShaderModuleRef ShaderModule::get(const fs::path& path) {
    static bool first = true;
    static std::unordered_map<fs::path, ShaderModuleHnd> shader_modules;
    if (first) {
        addToShutdownDeleteQueue([&]() { shader_modules.clear(); });
        first = false;
    }

    auto it = shader_modules.find(path);
    if (it != shader_modules.end()) {
        return it->second.get();
    }
    auto emplace_result = shader_modules.emplace(path, ShaderModuleHnd(new ShaderModule(path)));
    MLE_ASSERT(emplace_result.second);
    return emplace_result.first->second.get();
}

}  // namespace mle::renderer
