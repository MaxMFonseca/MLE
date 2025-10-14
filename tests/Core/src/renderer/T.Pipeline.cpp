#include <gtest/gtest.h>

#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/ShaderCache.h"

using namespace mle;  // NOLINT

TEST(Pipeline, CreateGraphicsPipeline) {
    auto& shader_cache = Renderer::i().shaderCache();
    const auto& vert = shader_cache.get("i/test.vert");
    const auto& frag = shader_cache.get("i/test.frag");

    vk::DescriptorSetLayout vdc_descriptor_set_layout;
    {
        vk::DescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eSampledImage;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding.descriptorCount = 500000;
        vk::DescriptorSetLayoutCreateInfo dsl_ci;
        dsl_ci.bindingCount = 1;
        dsl_ci.setBindings(binding);
        dsl_ci.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

        vk::DescriptorBindingFlags flags = vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::eUpdateAfterBind |
                                           vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
        vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_ci;
        binding_flags_ci.bindingCount = 1;
        binding_flags_ci.pBindingFlags = &flags;
        dsl_ci.pNext = &binding_flags_ci;

        vdc_descriptor_set_layout = unwrap(Renderer::i().vkDevice().createDescriptorSetLayout(dsl_ci));
    }

    Pipeline::CreateInfo ci;
    ci.vertex_shader = &vert;
    ci.fragment_shader = &frag;
    std::array color_attachment_formats{vk::Format::eR8G8B8A8Unorm};
    ci.color_attachment_formats = color_attachment_formats;
    std::array blend_attachments{vk::PipelineColorBlendAttachmentState{}};
    ci.blend_attachments = blend_attachments;
    ci.depth = false;
    ci.external_descriptor_set_layouts = {{0, vdc_descriptor_set_layout}};

    auto& pipeline_cache = Renderer::i().pipelineCache();
    const Pipeline& pipeline = pipeline_cache.setPipeline("test_graphics_pipeline", ci);
    EXPECT_FALSE(pipeline.isCompute());
    EXPECT_EQ(pipeline.getPipelineLayout() != nullptr, true);
    EXPECT_EQ(pipeline.hasInstance(), true);

    Renderer::i().destroy(vdc_descriptor_set_layout);
}

TEST(Pipeline, CreateComputePipeline) {
    auto& shader_cache = Renderer::i().shaderCache();
    const auto& comp = shader_cache.get("i/test.comp");

    const auto& pc_fields = comp.getPushConstantFields();
    EXPECT_EQ(pc_fields.size(), 2);
    EXPECT_EQ(pc_fields[0].name, "offset");
    EXPECT_EQ(pc_fields[1].name, "scale");
    EXPECT_EQ(comp.getPushConstantRange().size, 8);

    const auto& sets = comp.getDescriptorSets();
    ASSERT_EQ(sets.size(), 1);
    EXPECT_EQ(sets[0].set, 0);
    ASSERT_GE(sets[0].bindings.size(), 1);
    EXPECT_EQ(sets[0].bindings[0].descriptorType, vk::DescriptorType::eStorageBuffer);

    Pipeline::CreateInfo ci;
    ci.compute_shader = &comp;
    auto& pipeline_cache = Renderer::i().pipelineCache();
    const Pipeline& pipeline = pipeline_cache.setPipeline("test_compute_pipeline", ci);
    EXPECT_TRUE(pipeline.isCompute());
    EXPECT_EQ(pipeline.getPipelineLayout() != nullptr, true);
    EXPECT_TRUE(pipeline.hasPushConstants());
    EXPECT_EQ(pipeline.getPushConstantSize(), 8);
}
