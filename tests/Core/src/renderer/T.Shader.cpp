#include <gtest/gtest.h>

#include "mle/renderer/Renderer.h"
#include "mle/renderer/Shader.h"
#include "mle/renderer/ShaderCache.h"

using namespace mle;  // NOLINT

TEST(Shader, LoadVertexShader) {
    auto& cache = Renderer::i().shaderCache();
    auto* shader = cache.get("i/test.vert");
    ASSERT_NE(shader, nullptr);

    EXPECT_EQ(shader->makePipelineShaderStageCreateInfo().stage, vk::ShaderStageFlagBits::eVertex);

    // Check vertex attributes
    const auto& attrs = shader->getVertexAttributes();
    EXPECT_EQ(attrs.size(),
              8);  // ini_pos, ini_size, ini_color, ini_outline_color, ini_texture_offset, ini_texture_size, ini_texture_index, ini_outline_thickness

    EXPECT_EQ(attrs[0].location, 0);
    EXPECT_EQ(attrs[0].format, vk::Format::eR32G32Sfloat);  // ini_pos: vec2
    EXPECT_EQ(attrs[6].location, 6);
    EXPECT_EQ(attrs[6].format, vk::Format::eR32Uint);  // ini_texture_index: uint
}

TEST(Shader, LoadFragmentShader) {
    auto& cache = Renderer::i().shaderCache();
    auto* shader = cache.get("i/test.frag");
    ASSERT_NE(shader, nullptr);

    EXPECT_EQ(shader->makePipelineShaderStageCreateInfo().stage, vk::ShaderStageFlagBits::eFragment);

    const auto& sets = shader->mergeDescriptorSets(*shader);
    ASSERT_GE(sets.size(), 2);

    // Set 0: texture2D array
    EXPECT_EQ(sets[0].set, 0);
    ASSERT_GE(sets[0].bindings.size(), 1);
    EXPECT_EQ(sets[0].bindings[0].descriptorType, vk::DescriptorType::eSampledImage);

    // Set 1: sampler
    EXPECT_EQ(sets[1].set, 1);
    ASSERT_GE(sets[1].bindings.size(), 1);
    EXPECT_EQ(sets[1].bindings[0].descriptorType, vk::DescriptorType::eSampler);
}

TEST(Shader, VertexAndFragmentMergeDescriptorSets) {
    auto& cache = Renderer::i().shaderCache();
    auto* vert = cache.get("i/test.vert");
    auto* frag = cache.get("i/test.frag");
    ASSERT_NE(vert, nullptr);
    ASSERT_NE(frag, nullptr);

    auto merged = vert->mergeDescriptorSets(*frag);

    EXPECT_EQ(merged.size(), 2);
    EXPECT_EQ(merged[0].set, 0);
    EXPECT_EQ(merged[1].set, 1);
}

TEST(Shader, PushConstantReflection) {
    auto& cache = Renderer::i().shaderCache();
    auto* vert = cache.get("i/test.vert");
    ASSERT_NE(vert, nullptr);

    const auto& pc_fields = vert->getPushConstantFields();
    EXPECT_TRUE(pc_fields.empty());
    EXPECT_EQ(vert->getPushConstantRange().size, 0);
}

TEST(Shader, DescriptorSetBindingMerge) {
    auto& cache = Renderer::i().shaderCache();
    auto* frag = cache.get("i/test.frag");
    auto* frag2 = cache.get("i/test.frag");
    ASSERT_NE(frag, nullptr);
    ASSERT_NE(frag2, nullptr);

    auto merged = frag->mergeDescriptorSets(*frag2);
    EXPECT_EQ(merged.size(), frag->getDescriptorSets().size());
    for (size_t i = 0; i < merged.size(); ++i) {
        EXPECT_EQ(merged[i].set, frag->getDescriptorSets()[i].set);
        EXPECT_EQ(merged[i].bindings.size(), frag->getDescriptorSets()[i].bindings.size());
    }
}

TEST(Shader, StageFromExtension) {
    using mle::Shader;
    EXPECT_EQ(Shader::stageFromExtension("shader.vert.spv"), vk::ShaderStageFlagBits::eVertex);
    EXPECT_EQ(Shader::stageFromExtension("shader.frag.spv"), vk::ShaderStageFlagBits::eFragment);
    EXPECT_EQ(Shader::stageFromExtension("shader.comp.spv"), vk::ShaderStageFlagBits::eCompute);
}

TEST(Shader, TypeSize) {
    using mle::Shader;
    EXPECT_EQ(Shader::typeSize(Shader::DataType::FLOAT), 4);
    EXPECT_EQ(Shader::typeSize(Shader::DataType::VEC2), 8);
    EXPECT_EQ(Shader::typeSize(Shader::DataType::VEC3), 12);
    EXPECT_EQ(Shader::typeSize(Shader::DataType::VEC4), 16);
    EXPECT_EQ(Shader::typeSize(Shader::DataType::MAT4), 64);
}

TEST(Shader, VkFormatToDataType) {
    using mle::Shader;
    EXPECT_EQ(Shader::vkFormatToDataType(vk::Format::eR32Sfloat), Shader::DataType::FLOAT);
    EXPECT_EQ(Shader::vkFormatToDataType(vk::Format::eR32G32Sfloat), Shader::DataType::VEC2);
    EXPECT_EQ(Shader::vkFormatToDataType(vk::Format::eR32G32B32Sfloat), Shader::DataType::VEC3);
    EXPECT_EQ(Shader::vkFormatToDataType(vk::Format::eR32G32B32A32Sfloat), Shader::DataType::VEC4);
    EXPECT_EQ(Shader::vkFormatToDataType(vk::Format::eR32Uint), Shader::DataType::UINT);
    EXPECT_EQ(Shader::vkFormatToDataType(vk::Format::eR32Sint), Shader::DataType::INT);
}
