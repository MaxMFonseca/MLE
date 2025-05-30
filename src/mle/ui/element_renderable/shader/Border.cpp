#include "Border.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"

namespace mle::ui::element_renderable {
Border::Border(ElementRef element) :
    Shader(element) {
    auto pgr = mle::renderer::getPipeline("uiRect");
    pipeline_ = pgr.pipeline;
    if (pgr.is_new) {
        pipeline_->setVertexShader(mle::addShadersBasePath("ui/uv_quad_instanced.vert"));
        pipeline_->setFragmentShader(mle::addShadersBasePath("ui/border.frag"));
        pipeline_->setTopology(vk::PrimitiveTopology::eTriangleStrip);
        pipeline_->setPolygonMode(vk::PolygonMode::eFill);
        pipeline_->setBlendAttachments(mle::renderer::makeDefaultBlendAttachmentStates(1));
        pipeline_->setColorAttachmentFormats({mle::renderer::getDefaultColorFormat()});
    }
}

void Border::set(const sol::table& table) {
    if (sol::object r_roundness = table["roundness"]; r_roundness.valid()) {
        setRoundness(r_roundness);
    }
    if (sol::object r_color = table["color"]; r_color.valid()) {
        setColor(r_color);
    }
    if (sol::object r_edge_pos = table["edge_pos"]; r_edge_pos.valid()) {
        setEdgePos(r_edge_pos);
    }
    if (sol::object r_edge_thickness = table["edge_thickness"]; r_edge_thickness.valid()) {
        setEdgeThickness(r_edge_thickness);
    }
}

void Border::setRoundness(const sol::object& obj) {
    if (obj.is<f32>()) {
        auto f = obj.as<f32>();
        setRoundness({f, f, f, f});
    } else {
        setRoundness(mle::lua::asVec4f(obj));
    }
}

void Border::setRoundness(const vec4f& roundness) {
    ubo_data_.roundness = roundness;
    element_->setRequestingRender();
}

void Border::setColor(const sol::object& obj) {
    setColor(Color::fromLua(obj));
}

void Border::setColor(const Color& color) {
    ubo_data_.color = color.toLinear();
    element_->setRequestingRender();
}

void Border::setEdgePos(const sol::object& obj) {
    setEdgePos(obj.as<f32>());
}

void Border::setEdgePos(f32 edge_pos) {
    ubo_data_.edge_pos = edge_pos;
    element_->setRequestingRender();
}

void Border::setEdgeThickness(const sol::object& obj) {
    setEdgeThickness(obj.as<f32>());
}

void Border::setEdgeThickness(f32 edge_thickness) {
    ubo_data_.edge_thickness = edge_thickness;
    element_->setRequestingRender();
}

struct Instance {
    vec2f pos;
    vec2f size;
};

bool Border::render() {
    ubo_data_.clamp_pos = element_->getRectInternalClamped().pos;
    ubo_data_.clamp_size = element_->getRectInternalClamped().size;
    ubo_data_.aspect_ratio = element_->getAspectRatio();

    return false;
}

void Border::exec() const {
    pipeline_->build();

    Instance instance{};
    instance.pos = element_->getRectOnRoot().pos;
    instance.size = element_->getRectOnRoot().size;

    renderer::Buffer::CI instance_ci;
    instance_ci.size = sizeof(Instance);
    instance_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    instance_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    renderer::BufferRef instance_buffer = renderer::createBufferForFrame(instance_ci);
    instance_buffer->update(&instance);

    renderer::Buffer::CI ubo_ci;
    ubo_ci.size = sizeof(UBO);
    ubo_ci.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    ubo_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    renderer::BufferRef ubo = renderer::createBufferForFrame(ubo_ci);
    ubo->update(&ubo_data_);

    renderer::Pipeline::ExecInfo exec_info;
    exec_info.instance_count = 1;
    exec_info.instance_buffer = instance_buffer;
    exec_info.index_count = 4;

    auto& d0 = exec_info.descriptor_sets.emplace_back();
    d0.dstBinding = 0;
    d0.descriptorType = vk::DescriptorType::eUniformBuffer;
    d0.descriptorCount = 1;
    std::array buffer_info = {ubo->makeDescriptorInfo()};
    d0.setBufferInfo(buffer_info);

    pipeline_->exec(exec_info);
}
}  // namespace mle::ui::element_renderable
