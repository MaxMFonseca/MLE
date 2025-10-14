#include "Pipeline.h"

#include "Renderer.h"

namespace mle {
Pipeline::~Pipeline() {
    if (o_) {
        Renderer::i().destroy(o_);
        Renderer::i().destroy(pipeline_layout_);
    }
};

void Pipeline::init(const CI& ci) {
    MLE_D("Building pipeline");

    MLE_ASSERT(ci.vertex_shader || ci.fragment_shader || ci.compute_shader);

    createPipelineLayout(ci);

    if (ci.compute_shader) {
        createComputePipeline(ci);
    } else {
        createGraphicsPipeline(ci);
    }
}

void Pipeline::createPipelineLayout(const CI& ci) {
    MLE_T("Creating pipeline layout");

    vk::PipelineLayoutCreateInfo pipeline_layout_ci;

    std::vector<std::pair<u8, vk::DescriptorSetLayout>> set_layouts;

    auto owning_sets = ci.compute_shader ? ci.compute_shader->getDescriptorSets() : ci.vertex_shader->mergeDescriptorSets(*ci.fragment_shader);
    for (auto& i : owning_sets) {
        if (i.bindings.empty()) {
            continue;
        }
        if (std::ranges::find_if(ci.external_descriptor_set_layouts, [&](const auto& p) { return p.first == i.set; }) !=
            ci.external_descriptor_set_layouts.end()) {
            continue;
        }

        vk::DescriptorSetLayoutCreateInfo dsl_ci;
        dsl_ci.setBindings(i.bindings);
        if (i.set == 0) {
            dsl_ci.setFlags(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR);
        }
        owned_dsls_.emplace_back(unwrap(Renderer::i().vkDevice().createDescriptorSetLayout(dsl_ci)));
        set_layouts.emplace_back(i.set, owned_dsls_.back());
    }

    std::vector<vk::DescriptorSetLayout> sorted_set_layouts;
    usize external_idx = 0;
    usize owned_idx = 0;
    for (usize i = 0; i < ci.external_descriptor_set_layouts.size() + set_layouts.size(); i++) {
        if (ci.external_descriptor_set_layouts.size() > external_idx && ci.external_descriptor_set_layouts.at(external_idx).first == i) {
            sorted_set_layouts.emplace_back(ci.external_descriptor_set_layouts.at(external_idx).second);
            external_idx++;
        } else if (set_layouts.size() > owned_idx && set_layouts.at(owned_idx).first == i) {
            sorted_set_layouts.emplace_back(set_layouts.at(owned_idx).second);
            owned_idx++;
        } else {
            MLE_UNREACHABLE_LOG("Descriptor set layout missing for set {}", i);
        }
    }

    pipeline_layout_ci.setSetLayouts(sorted_set_layouts);

    pc_fields_.clear();
    pc_frag_offset_ = max<u8>();
    int pc_count = 0;
    std::array<vk::PushConstantRange, 2> pc_ranges{};
    if ((ci.compute_shader != nullptr) && ci.compute_shader->getPushConstantRange().size != 0U) {
        pc_ranges.at(0) = ci.compute_shader->getPushConstantRange();
        pc_fields_ = ci.compute_shader->getPushConstantFields();
        MLE_ASSERT_LOG(pc_ranges.at(0).offset == 0,
                       "Compute shader push constant offset must be 0, add layout(offset = 0) to the first line of your compute shader pc block.");
        pc_count++;
    }
    if ((ci.vertex_shader != nullptr) && ci.vertex_shader->getPushConstantRange().size != 0U) {
        pc_ranges.at(0) = ci.vertex_shader->getPushConstantRange();
        pc_fields_ = ci.vertex_shader->getPushConstantFields();
        pc_count++;
    }
    if ((ci.fragment_shader != nullptr) && ci.fragment_shader->getPushConstantRange().size != 0U) {
        pc_frag_offset_ = as<int>(pc_ranges.at(0).size);
        pc_ranges.at(pc_count) = ci.fragment_shader->getPushConstantRange();
        pc_fields_.insert(pc_fields_.end(), ci.fragment_shader->getPushConstantFields().begin(), ci.fragment_shader->getPushConstantFields().end());
        MLE_ASSERT_LOG(pc_frag_offset_ == (int)pc_ranges.at(pc_count).offset,
                       "Push constant offset mismatch, add layout(offset = {}) to the first line of your frag shader pc block.", pc_frag_offset_);
        pc_count++;
    }
    pc_size_ = as<int>(pc_ranges.at(0).size + pc_ranges.at(1).size);
    MLE_ASSERT_LOG(pc_size_ <= 128, "Push constant size too large: {}", pc_size_);

    pipeline_layout_ci.setPushConstantRanges(pc_ranges);
    pipeline_layout_ci.setPushConstantRangeCount(pc_count);

    pipeline_layout_ = unwrap(Renderer::i().vkDevice().createPipelineLayout(pipeline_layout_ci));
}

void Pipeline::createComputePipeline(const CI& ci) {
    MLE_ASSERT_LOG(ci.compute_shader, "Compute shader must be set");

    compute_ = true;

    vk::ComputePipelineCreateInfo pipeline_ci{};

    MLE_T("Shader stages");
    pipeline_ci.setStage(ci.compute_shader->makePipelineShaderStageCreateInfo());

    pipeline_ci.layout = pipeline_layout_;
    pipeline_ci.basePipelineHandle = nullptr;
    pipeline_ci.basePipelineIndex = 0;

    o_ = unwrap(Renderer::i().vkDevice().createComputePipeline(nullptr, pipeline_ci));

    MLE_T("Compute pipeline created successfully");
}

void Pipeline::createGraphicsPipeline(const CI& ci) {
    MLE_ASSERT_LOG(ci.vertex_shader, "Vertex shader must be set");
    MLE_ASSERT_LOG(ci.blend_attachments.size() == ci.color_attachment_formats.size(),
                   "Number of blend attachments must match the number of color attachment formats");

    first_instance_binding_ = ci.vertex_shader->getFirstInstanceAttributeLocation();

    vk::GraphicsPipelineCreateInfo pipeline_ci{};

    MLE_T("Shader stages");
    std::vector stages = {ci.vertex_shader->makePipelineShaderStageCreateInfo()};
    if (ci.fragment_shader) {
        stages.push_back(ci.fragment_shader->makePipelineShaderStageCreateInfo());
    }
    pipeline_ci.setStages(stages);

    MLE_T("Vertex input state");
    auto vertex_input_state = ci.vertex_shader->makePipelineVertexInputStateCreateInfo();

    for (const auto& vi : ci.vertex_shader->getVertexAttributes()) {
        MLE_T("Vertex input attribute: location = {}, binding = {}, format = {}, offset = {}", vi.location, vi.binding, vk::to_string(vi.format), vi.offset);
    }
    for (const auto& binding : ci.vertex_shader->getVertexBindings()) {
        MLE_T("Vertex input binding: binding = {}, stride = {}, input rate = {}", binding.binding, binding.stride, vk::to_string(binding.inputRate));
    }

    pipeline_ci.setPVertexInputState(&vertex_input_state);

    MLE_T("Input assembly state");
    MLE_T("Topology: {}", vk::to_string(ci.topology));
    MLE_T("Primitive restart enable: {}", false);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_ci;
    input_assembly_state_ci.topology = ci.topology;
    input_assembly_state_ci.primitiveRestartEnable = vk::False;
    pipeline_ci.setPInputAssemblyState(&input_assembly_state_ci);

    MLE_T("Tessellation state");
    MLE_T("Not used");

    pipeline_ci.setPTessellationState(nullptr);

    MLE_T("Viewport state");
    MLE_T("Dynamic");

    vk::PipelineViewportStateCreateInfo viewport_state_ci;
    viewport_state_ci.viewportCount = 1;
    viewport_state_ci.scissorCount = 1;
    pipeline_ci.setPViewportState(&viewport_state_ci);

    MLE_T("Rasterization state");
    MLE_T("Polygon mode: {}", vk::to_string(ci.polygon_mode));
    MLE_T("Cull mode: {}", vk::to_string(ci.cull_mode));
    MLE_T("Front face: {}", vk::to_string(ci.front_face));

    vk::PipelineRasterizationStateCreateInfo rasterization_state_ci;
    rasterization_state_ci.polygonMode = ci.polygon_mode;
    rasterization_state_ci.cullMode = ci.cull_mode;
    rasterization_state_ci.frontFace = ci.front_face;
    rasterization_state_ci.lineWidth = 1.0F;
    rasterization_state_ci.depthClampEnable = vk::False;
    rasterization_state_ci.rasterizerDiscardEnable = vk::False;
    rasterization_state_ci.depthBiasEnable = ci.depth_bias ? vk::True : vk::False;
    rasterization_state_ci.depthBiasConstantFactor = 1.25F;
    rasterization_state_ci.depthBiasSlopeFactor = 1.75F;
    rasterization_state_ci.depthBiasClamp = 0.0F;
    pipeline_ci.setPRasterizationState(&rasterization_state_ci);

    MLE_T("Multisample state");
    MLE_T("Samples: {}", 1);
    MLE_T("Sample shading enable: {}", false);

    vk::PipelineMultisampleStateCreateInfo multisample_state_ci;
    multisample_state_ci.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisample_state_ci.sampleShadingEnable = vk::False;
    pipeline_ci.setPMultisampleState(&multisample_state_ci);

    MLE_T("Depth stencil state");

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_ci;
    depth_stencil_state_ci.depthTestEnable = ci.depth ? vk::True : vk::False;
    depth_stencil_state_ci.depthWriteEnable = ci.depth_write ? vk::True : vk::False;
    depth_stencil_state_ci.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depth_stencil_state_ci.depthBoundsTestEnable = vk::False;
    depth_stencil_state_ci.stencilTestEnable = vk::False;
    depth_stencil_state_ci.minDepthBounds = 0.0F;
    depth_stencil_state_ci.maxDepthBounds = 1.0F;

    MLE_T("Depth test enable: {}", depth_stencil_state_ci.depthTestEnable);
    MLE_T("Depth write enable: {}", depth_stencil_state_ci.depthWriteEnable);
    MLE_T("Depth compare op: {}", vk::to_string(depth_stencil_state_ci.depthCompareOp));
    MLE_T("Depth bounds test enable: {}", depth_stencil_state_ci.depthBoundsTestEnable);
    MLE_T("Stencil test enable: {}", depth_stencil_state_ci.stencilTestEnable);
    MLE_T("Min depth bounds: {}", depth_stencil_state_ci.minDepthBounds);
    MLE_T("Max depth bounds: {}", depth_stencil_state_ci.maxDepthBounds);

    pipeline_ci.setPDepthStencilState(&depth_stencil_state_ci);

    MLE_T("Color blend state");
    for (int i = 0; const auto& state : ci.blend_attachments) {
        MLE_T("Color blend attachment state {}", i);
        MLE_T("Blend enable: {}", state.blendEnable);
        MLE_T("Src color blend factor: {}", vk::to_string(state.srcColorBlendFactor));
        MLE_T("Dst color blend factor: {}", vk::to_string(state.dstColorBlendFactor));
        MLE_T("Color blend op: {}", vk::to_string(state.colorBlendOp));
        MLE_T("Src alpha blend factor: {}", vk::to_string(state.srcAlphaBlendFactor));
        MLE_T("Dst alpha blend factor: {}", vk::to_string(state.dstAlphaBlendFactor));
        MLE_T("Alpha blend op: {}", vk::to_string(state.alphaBlendOp));
        MLE_T("Color blend write mask: {}", vk::to_string(state.colorWriteMask));
        i++;
    }

    vk::PipelineColorBlendStateCreateInfo color_blend_state_ci;
    color_blend_state_ci.logicOpEnable = vk::False;
    color_blend_state_ci.setAttachments(ci.blend_attachments);
    pipeline_ci.setPColorBlendState(&color_blend_state_ci);

    MLE_T("Dynamic state");
    std::vector<vk::DynamicState> dynamic_states;
    dynamic_states.assign(ci.dynamic_states.begin(), ci.dynamic_states.end());
    dynamic_states.push_back(vk::DynamicState::eViewport);
    dynamic_states.push_back(vk::DynamicState::eScissor);
    for (auto state : dynamic_states) {
        MLE_T("{}", vk::to_string(state));
    }

    vk::PipelineDynamicStateCreateInfo dynamic_state_ci;
    dynamic_state_ci.setDynamicStates(dynamic_states);
    pipeline_ci.setPDynamicState(&dynamic_state_ci);

    pipeline_ci.layout = pipeline_layout_;

    pipeline_ci.renderPass = nullptr;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = nullptr;
    pipeline_ci.basePipelineIndex = 0;

    MLE_T("Rendering");

    vk::PipelineRenderingCreateInfo pipeline_rendering_ci;
    if (ci.fragment_shader) {
        MLE_ASSERT(!ci.color_attachment_formats.empty());
        pipeline_rendering_ci.setColorAttachmentFormats(ci.color_attachment_formats);
        for (const auto& format : ci.color_attachment_formats) {
            MLE_T("Color attachment format: {}", vk::to_string(format));
        }
    }
    if (ci.depth) {
        auto depth_format = Renderer::i().vk().getVkImageFormat(ImageFormat::DEPTH);
        pipeline_rendering_ci.setDepthAttachmentFormat(depth_format);
        MLE_T("Depth attachment format: {}", vk::to_string(depth_format));
    }
    pipeline_ci.pNext = &pipeline_rendering_ci;

    o_ = unwrap(Renderer::i().vkDevice().createGraphicsPipeline(nullptr, pipeline_ci));
    MLE_T("Pipeline created successfully");
}

const Shader::PushConstantField& Pipeline::getPushConstantField(std::string_view name) const {
    for (const auto& field : pc_fields_) {
        if (field.name == name) {
            return field;
        }
    }

    MLE_UNREACHABLE_LOG("Push constant field not found: {}", name);
}

PipelineHnd Pipeline::createHnd(const CI& ci) {
    auto ret = std::unique_ptr<Pipeline>(new Pipeline());
    ret->init(ci);
    return ret;
}
}  // namespace mle
