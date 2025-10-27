#include "PipelineCache.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "vulkan/vulkan.hpp"

namespace mle {
void PipelineCache::shutdown() {
    MLE_I("Shutting down PipelineCache");
    pipelines_.clear();
}

const Pipeline& PipelineCache::setPipeline(const std::string& name, const Pipeline::CI& pipeline_ci) {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_I("Setting pipeline: {}", name);
    auto hnd = Pipeline::createHnd(pipeline_ci);
    std::scoped_lock lock(mutex_);
    auto er = pipelines_.emplace(name, std::move(hnd));
    return *er.first->second;
}

const Pipeline& PipelineCache::getPipeline(const std::string& name) const {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_T("Getting pipeline: {}", name);
    std::scoped_lock lock(mutex_);
    auto it = pipelines_.find(name);
    if (it != pipelines_.end()) {
        return *it->second;
    }
    MLE_UNREACHABLE_LOG("Pipeline not found: {}", name);
}

const Pipeline* PipelineCache::tryGetPipeline(const std::string& name) const {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_T("Trying to get pipeline: {}", name);
    std::scoped_lock lock(mutex_);
    auto it = pipelines_.find(name);
    if (it != pipelines_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Pipeline* PipelineCache::tryLuaPipeline(const sol::table& table) {
    const auto pipeline_name_r = table["name"];
    if (!pipeline_name_r.valid()) {
        MLE_E("Name field is required.");
        return nullptr;
    }
    auto pipeline_name = pipeline_name_r.get<std::string>();
    if (const auto* ret = tryGetPipeline(pipeline_name)) {
        return ret;
    }
    Pipeline::CI pipeline_ci;
    if (const auto vert_r = table["vert"]; lua::valid<std::string>(vert_r)) {
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get(lua::as<std::string>(vert_r));
    }
    if (const auto frag_r = table["frag"]; lua::valid<std::string>(frag_r)) {
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get(lua::as<std::string>(frag_r));
    }
    if (const auto push_descriptor_r = table["push_descriptor"]; lua::valid<int>(push_descriptor_r)) {
        pipeline_ci.push_descriptor = lua::as<int>(push_descriptor_r);
    }
    std::vector<vk::Format> color_formats;
    if (const sol::object colors_r = table["colors"]; lua::valid<sol::table>(colors_r)) {
        for (auto& [_, str] : colors_r.as<sol::table>()) {
            auto format_str = str.as<std::string>();
            auto format_r = VkCtx::getFormat(format_str.c_str());
            if (!format_r.has_value()) {
                MLE_E("Failed to create pipeline, invalid color format.");
                return nullptr;
            }
            auto format = Renderer::i().vk().getVkImageFormat(*format_r);
            color_formats.push_back(format);
        }
    }
    std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments = Pipeline::makeDefaultBlendAttachments(color_formats.size());
    pipeline_ci.color_attachment_formats = color_formats;
    pipeline_ci.blend_attachments = blend_attachments;
    if (const auto topology_r = table["topology"]; lua::valid<std::string>(topology_r)) {
        const auto str = lua::as<std::string>(topology_r);
        if (str == "triangle_strip") {
            pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        } else {
            MLE_E("Unknown topology string {}", str);
            return nullptr;
        }
    }
    if (const auto cull_r = table["cull"]; lua::valid<std::string>(cull_r)) {
        const auto str = lua::as<std::string>(cull_r);
        if (str == "none") {
            pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        } else {
            MLE_E("Unknown cull string {}", str);
            return nullptr;
        }
    }
    return &setPipeline(pipeline_name, pipeline_ci);
}
}  // namespace mle
