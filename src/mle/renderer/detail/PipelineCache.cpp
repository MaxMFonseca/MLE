#include "PipelineCache.h"

#include "mle/renderer/Renderer.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer::detail {
namespace {
Pipeline::CI sceneVoxCI() {
    Pipeline::CI ci;
    ci.vertex_shader = getShader("mle/scene/vox.vert");
    ci.fragment_shader = getShader("mle/scene/vox.frag");
    ci.depth = true;
    ci.color_attachment_formats = {getDefaultColorFormat(), getDefaultColorFormat(), getDefaultColorFormat()};
    ci.color_attachment_formats = {getDefaultColorFormat(), getDefaultColorFormat(), getDefaultColorFormat()};
    ci.color_attachment_formats = {getDefaultColorFormat(), getDefaultColorFormat(), getDefaultColorFormat()};
    ci.blend_attachments = makeDefaultBlendAttachmentStates(3, false);
    ci.topology = vk::PrimitiveTopology::eTriangleList;
    return ci;
}

Pipeline::CI sceneVoxRecolorCI() {
    Pipeline::CI ci;
    ci.vertex_shader = getShader("mle/scene/vox_recolor.vert");
    ci.fragment_shader = getShader("mle/scene/vox.frag");
    ci.depth = true;
    ci.color_attachment_formats = {getDefaultColorFormat(), getDefaultColorFormat(), getDefaultColorFormat()};
    ci.color_attachment_formats = {getDefaultColorFormat(), getDefaultColorFormat(), getDefaultColorFormat()};
    ci.color_attachment_formats = {getDefaultColorFormat(), getDefaultColorFormat(), getDefaultColorFormat()};
    ci.blend_attachments = makeDefaultBlendAttachmentStates(3, false);
    ci.topology = vk::PrimitiveTopology::eTriangleList;
    return ci;
}

Pipeline::CI scenePlaneCI() {
    Pipeline::CI ci;
    ci.vertex_shader = getShader("mle/scene/plane.vert");
    ci.fragment_shader = getShader("mle/scene/plane.frag");
    ci.depth = true;
    ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
    ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
    ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
    ci.blend_attachments = makeDefaultBlendAttachmentStates(3, false);
    ci.topology = vk::PrimitiveTopology::eTriangleStrip;
    return ci;
}

Pipeline::CI sceneCubeMapCI() {
    Pipeline::CI ci;
    ci.vertex_shader = getShader("mle/scene/skybox.vert");
    ci.fragment_shader = getShader("mle/scene/skybox.frag");
    ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
    ci.blend_attachments = makeDefaultBlendAttachmentStates(1, false);
    ci.topology = vk::PrimitiveTopology::eTriangleList;
    ci.cull_mode = vk::CullModeFlagBits::eNone;
    return ci;
}
Pipeline::CI sceneLightingCI() {
    Pipeline::CI ci;
    ci.vertex_shader = getShader("mle/fs_triangle.vert");
    ci.fragment_shader = getShader("mle/scene/light.frag");
    ci.depth = true;
    ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
    ci.blend_attachments = makeDefaultBlendAttachmentStates(1);
    ci.topology = vk::PrimitiveTopology::eTriangleList;
    // ci.depth_bias = true;
    ci.cull_mode = vk::CullModeFlagBits::eNone;
    return ci;
}

Pipeline::CI sceneSunCI() {
    Pipeline::CI ci;
    ci.vertex_shader = getShader("mle/scene/sun.vert");
    ci.depth = true;
    return ci;
}
}  // namespace

void PipelineCache::init() {
    MLE_I("Creating builtin pipelines");

    setPipeline("mle/scene/vox", sceneVoxCI());
    setPipeline("mle/scene/vox_recolor", sceneVoxRecolorCI());
    setPipeline("mle/scene/plane", scenePlaneCI());
    setPipeline("mle/scene/cube_map", sceneCubeMapCI());
    setPipeline("mle/scene/sun", sceneSunCI());
    setPipeline("mle/scene/lighting", sceneLightingCI());
}

PipelineRef PipelineCache::setPipeline(const std::string& name, const Pipeline::CI& pipeline_ci) {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_I("Setting pipeline: {}", name);

    auto it = pipelines_.find(name);
    if (it != pipelines_.end()) {
        MLE_W("Replacing existing pipeline: {}", name);
        it->second->init(pipeline_ci);
        return it->second.get();
    }

    auto er = pipelines_.emplace(name, Pipeline::createHnd(pipeline_ci));
    return er.first->second.get();
}

PipelineRef PipelineCache::getPipeline(const std::string& name) {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_T("Getting pipeline: {}", name);

    auto it = pipelines_.find(name);
    if (it != pipelines_.end()) {
        return it->second.get();
    }

    MLE_E("Pipeline not found: {}", name);
    return nullptr;
}

void PipelineCache::shutdown() {
    MLE_I("Shutting down PipelineCache");
    pipelines_.clear();
}

PipelineRef PipelineCache::getPipelineOrSet(const std::string& name, const Pipeline::CI& pipeline_ci) {
    auto it = pipelines_.find(name);

    if (it != pipelines_.end()) {
        return it->second.get();
    }

    return setPipeline(name, pipeline_ci);
}
}  // namespace mle::renderer::detail
