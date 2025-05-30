#include "Pipeline.h"

#include <utility>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Renderer.h"
#include "mle/common/Exception.h"
#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/renderer/Types.h"

namespace mle::renderer {
Pipeline::Pipeline(std::string&& debug_name) :
    debug_name_(std::move(debug_name)) {
    MLE_I("Creating pipeline {}", debug_name_);
}

Pipeline::~Pipeline() {
    MLE_D("Destroying pipeline {}", debug_name_);
    if (descriptor_set_layout_) {
        getVk().getDevice().destroy(descriptor_set_layout_);
    }
    if (pipeline_layout_) {
        getVk().getDevice().destroy(pipeline_layout_);
    }
    if (pipeline_) {
        getVk().getDevice().destroy(pipeline_);
    }
}

void Pipeline::createPipelineLayout() {
    if (!layout_changed_) {
        return;
    }
    MLE_T("Creating pipeline layout");

    if (pipeline_layout_) {
        addToFrameDeleteQueue([pipeline_layout = pipeline_layout_]() { getVk().getDevice().destroy(pipeline_layout); });
    }
    if (descriptor_set_layout_) {
        addToFrameDeleteQueue([descriptor_set_layout = descriptor_set_layout_]() { getVk().getDevice().destroy(descriptor_set_layout); });
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_ci;

    auto bindings = ShaderModule::mergeDescriptors(vertex_shader_->getDescriptors(), fragment_shader_->getDescriptors());
    if (!bindings.empty()) {
        MLE_T("  Descriptor set layout");
        for (usize i = 0; i < override_descriptor_binding_count_.size(); ++i) {
            if (override_descriptor_binding_count_[i] != 0) {
                bindings[i].descriptorCount = override_descriptor_binding_count_[i];
            }
        }

        for (auto& binding : bindings) {
            MLE_T("  Binding: {}, Descriptor type: {}, Descriptor count: {}, Stage flags: {}", binding.binding, vk::to_string(binding.descriptorType),
                  binding.descriptorCount, vk::to_string(binding.stageFlags));
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_ci;
        descriptor_set_layout_ci.setBindings(bindings);
        descriptor_set_layout_ci.flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;

        descriptor_set_layout_ = getVk().getDevice().createDescriptorSetLayout(descriptor_set_layout_ci);

        pipeline_layout_ci.setSetLayouts(descriptor_set_layout_);
    }

    pc_fields_.clear();
    pc_f_offset_ = 0;
    int pc_count = 0;
    std::array<vk::PushConstantRange, 2> pc_ranges{};
    if (vertex_shader_->getPushConstantRange().size != 0U) {
        pc_ranges.at(0) = vertex_shader_->getPushConstantRange();
        pc_fields_ = vertex_shader_->getPushConstantFields();
        pc_count++;
    }
    if (fragment_shader_->getPushConstantRange().size != 0U) {
        pc_f_offset_ = static_cast<int>(pc_ranges.at(0).size);
        pc_ranges.at(pc_count) = fragment_shader_->getPushConstantRange();
        pc_fields_.insert(pc_fields_.end(), fragment_shader_->getPushConstantFields().begin(), fragment_shader_->getPushConstantFields().end());
        MLE_ASSERT_LOG(pc_f_offset_ == (int)pc_ranges.at(pc_count).offset,
                       "Push constant offset mismatch, add layout(offset = {}) to the first line of your frag shader pc block.", pc_f_offset_);
        pc_count++;
    }
    pc_size_ = static_cast<int>(pc_ranges.at(0).size + pc_ranges.at(1).size);
    MLE_ASSERT_LOG(pc_size_ <= 128, "Push constant size too large: {}", pc_size_);

    pipeline_layout_ci.setPushConstantRanges(pc_ranges);
    pipeline_layout_ci.setPushConstantRangeCount(pc_count);

    pipeline_layout_ = getVk().getDevice().createPipelineLayout(pipeline_layout_ci);
}

void Pipeline::build() {
    if (!dirty_) {
        return;
    }

    MLE_D("Building pipeline {}", debug_name_);
    vk::GraphicsPipelineCreateInfo pipeline_ci;
    if (pipeline_) {
        pipeline_ci.basePipelineHandle = pipeline_;
        pipeline_ci.basePipelineIndex = 0;
    }

    MLE_T("Shader stages");
    MLE_T("Vertex shader: {}", vertex_shader_->getDebugName());
    MLE_T("Fragment shader: {}", fragment_shader_->getDebugName());

    std::array stages = {vertex_shader_->makePipelineShaderStageCi(), fragment_shader_->makePipelineShaderStageCi()};
    pipeline_ci.setStages(stages);

    MLE_T("Vertex input state");

    auto vertex_input_state = vertex_shader_->makePipelineVertexInputStateCreateInfo();
    pipeline_ci.setPVertexInputState(&vertex_input_state.ci);

    MLE_T("Input assembly state");
    MLE_T("Topology: {}", vk::to_string(topology_));
    MLE_T("Primitive restart enable: {}", false);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_ci;
    input_assembly_state_ci.topology = topology_;
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
    MLE_T("Polygon mode: {}", vk::to_string(polygon_mode_));
    MLE_T("Cull mode: {}", vk::to_string(cull_mode_));
    MLE_T("Front face: {}", vk::to_string(front_face_));

    vk::PipelineRasterizationStateCreateInfo rasterization_state_ci;
    rasterization_state_ci.polygonMode = polygon_mode_;
    rasterization_state_ci.cullMode = cull_mode_;
    rasterization_state_ci.frontFace = front_face_;
    rasterization_state_ci.lineWidth = 1.0F;
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
    depth_stencil_state_ci.depthTestEnable = depth_ ? vk::True : vk::False;
    depth_stencil_state_ci.depthWriteEnable = vk::True;
    depth_stencil_state_ci.depthCompareOp = vk::CompareOp::eLess;
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
    for (int i = 0; auto& state : blend_attachments_) {
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
    color_blend_state_ci.setAttachments(blend_attachments_);
    pipeline_ci.setPColorBlendState(&color_blend_state_ci);

    MLE_T("Dynamic state");
    for (auto state : dynamic_states_) {
        MLE_T("{}", vk::to_string(state));
    }

    vk::PipelineDynamicStateCreateInfo dynamic_state_ci;
    dynamic_state_ci.setDynamicStates(dynamic_states_);
    pipeline_ci.setPDynamicState(&dynamic_state_ci);

    MLE_T("Pipeline layout");
    createPipelineLayout();
    pipeline_ci.layout = pipeline_layout_;

    pipeline_ci.renderPass = nullptr;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = nullptr;
    pipeline_ci.basePipelineIndex = 0;

    MLE_T("Rendering");
    vk::PipelineRenderingCreateInfo pipeline_rendering_ci;
    MLE_ASSERT(!color_attachment_formats_.empty());
    pipeline_rendering_ci.setColorAttachmentFormats(color_attachment_formats_);
    for (auto& format : color_attachment_formats_) {
        MLE_T("Color attachment format: {}", vk::to_string(format));
    }
    if (depth_) {
        pipeline_rendering_ci.setDepthAttachmentFormat(getVk().getDepthFormat());
        MLE_T("Depth attachment format: {}", vk::to_string(getVk().getDepthFormat()));
    }
    pipeline_ci.pNext = &pipeline_rendering_ci;

    auto pipeline_rv = getVk().getDevice().createGraphicsPipeline(nullptr, pipeline_ci);
    if (pipeline_rv.result != vk::Result::eSuccess) {
        MLE_THROW(VK_ERROR, "Failed to create pipeline {}", debug_name_);
    }

    if (pipeline_) {
        addToFrameDeleteQueue([pipeline = pipeline_]() { getVk().getDevice().destroy(pipeline); });
    }
    pipeline_ = pipeline_rv.value;

    dirty_ = false;
}

void Pipeline::exec(const ExecInfo& info) {
    if (!pipeline_) {
        MLE_THROW(PIPELINE_NOT_BUILT, "Pipeline {} not built", debug_name_);
    }
    auto cmd = getFrameMainCommandBuffer();

    vk::Viewport viewport{};
    viewport.x = info.viewport.pos.x;
    viewport.y = info.viewport.pos.y;
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;

    if (info.viewport.size.x == 0.0F) {
        viewport.width = static_cast<f32>(getCurrentRenderArea().size.x) - info.viewport.pos.x;
    } else {
        viewport.width = info.viewport.size.x;
    }

    if (info.viewport.size.y == 0.0F) {
        viewport.height = static_cast<f32>(getCurrentRenderArea().size.y) - info.viewport.pos.y;
    } else {
        viewport.height = info.viewport.size.y;
    }

    cmd.setViewport(0, viewport);

    vk::Rect2D scissor{};
    scissor.offset.x = static_cast<i32>(viewport.x);
    scissor.offset.y = static_cast<i32>(viewport.y);
    scissor.extent.width = static_cast<u32>(viewport.width);
    scissor.extent.height = static_cast<u32>(viewport.height);
    cmd.setScissor(0, scissor);

    if (info.vertex_buffer) {
        cmd.bindVertexBuffers(0, info.vertex_buffer->get(), info.vertex_buffer_offset);
    }
    if (info.instance_buffer) {
        cmd.bindVertexBuffers(info.vertex_buffer ? 1 : 0, info.instance_buffer->get(), info.instance_buffer_offset);
    }
    if (info.index_buffer) {
        cmd.bindIndexBuffer(info.index_buffer->get(), info.index_buffer_offset, vk::IndexType::eUint32);
    }
    if (!info.descriptor_sets.empty()) {
        cmd.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, pipeline_layout_, 0, info.descriptor_sets);
    }
    if (pc_size_) {
        MLE_ASSERT_LOG(info.push_constants, "This pipeline requires push constants.");
        if (pc_f_offset_ == -1) {
            cmd.pushConstants(pipeline_layout_, vk::ShaderStageFlagBits::eVertex, 0, pc_size_, info.push_constants);
        } else if (pc_f_offset_ == 0) {
            cmd.pushConstants(pipeline_layout_, vk::ShaderStageFlagBits::eFragment, 0, pc_size_, info.push_constants);
        } else {
            cmd.pushConstants(pipeline_layout_, vk::ShaderStageFlagBits::eVertex, 0, pc_f_offset_, info.push_constants);
            cmd.pushConstants(pipeline_layout_, vk::ShaderStageFlagBits::eFragment, pc_f_offset_, pc_size_ - pc_f_offset_,
                              static_cast<const byte*>(info.push_constants) + pc_f_offset_);  // NOLINT
        }
    }

    renderer::bindGraphicsPipeline(this);

    cmd.draw(info.index_count, info.instance_count, 0, 0);
}

PipelineGetResult Pipeline::get(const std::string& name) {
    static bool first = true;
    static std::unordered_map<fs::path, PipelineHnd> pipelines;
    if (first) {
        addToShutdownDeleteQueue([&]() { pipelines.clear(); });
        first = false;
    }

    auto it = pipelines.find(name);
    if (it != pipelines.end()) {
        return {.pipeline = it->second.get(), .is_new = false};
    }

    auto emplace_result = pipelines.emplace(name, PipelineHnd(new Pipeline(std::string(name))));
    MLE_ASSERT(emplace_result.second);
    return {.pipeline = emplace_result.first->second.get(), .is_new = true};
}

ShaderModule::PushConstantField Pipeline::getPushConstantField(const std::string& name) const {
    for (const auto& field : pc_fields_) {
        if (field.name == name) {
            return field;
        }
    }
    MLE_THROW(NOT_FOUND, "Push constant field {} not found in pipeline {}", name, debug_name_);
}
}  // namespace mle::renderer
