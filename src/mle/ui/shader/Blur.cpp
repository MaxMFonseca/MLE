#include "Blur.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"

namespace mle::ui::shader {
namespace {
auto getPipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.compute_shader = &Renderer::i().shaderCache().get("mle/ui/blur.comp");
        pipeline_ci.push_descriptor = 0;
        pipeline = &Renderer::i().pipelineCache().setPipeline("mle/ui/blur", pipeline_ci);
    }
    return pipeline;
}
}  // namespace

void Blur::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to Blur::apply for entt {}", e.e());
        return;
    }
    auto* shader = e.tryGet<comp::Shader>();
    Blur* self_p = nullptr;
    if (!shader) {
        shader = &e.emplace<comp::Shader>(std::make_unique<Blur>());
        self_p = as<Blur*>(shader->impl.get());
        shader->packet_buffers_.at(0) = std::make_shared<BlurPacket>();
        shader->packet_buffers_.at(1) = std::make_shared<BlurPacket>();
        shader->packet_buffers_.at(2) = std::make_shared<BlurPacket>();
    } else {
        MLE_ASSERT(shader->impl);
        if (shader->impl->getType() != Blur::type()) {
            MLE_E("Shader::apply called on entt {} with incompatible Shader type. {}x{}", e.fullName(), shader->impl->getType(), Blur::type());
            return;
        }
        self_p = as<Blur*>(shader->impl.get());
    }
    self_p->set(obj);
};

void Blur::set(const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to Blur::set");
        return;
    }

    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto radius_r = table["radius"]; lua::valid<int>(radius_r)) {
            radius = lua::as<int>(radius_r);
        }
        if (const auto sigma_r = table["sigma"]; lua::valid<f32>(sigma_r)) {
            sigma = lua::as<f32>(sigma_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to Blur::set");
};

void Blur::doUpdatePacket(ShaderPacketI* packet) {
    auto* packet_p = as<BlurPacket*>(packet);
    packet_p->radius = radius;
    packet_p->sigma = sigma;
};

void BlurPacket::render(CompRenderingCtx& ctx) {
    auto& thread = ctx.thread;

    if (!image || image->getExtent().x < as<u32>(ctx.viewport.width()) || image->getExtent().y < as<u32>(ctx.viewport.height())) {
        Image::CI image_ci{};
        image_ci.format = thread.getColor0()->getFormat();
        image_ci.extent = ctx.viewport.size();
        if (image) {
            Renderer::i().frameRenderer().deleteAfterFrame(std::move(image));
        }
        image = Image::createHnd(image_ci);
    }

    const auto* pipeline = getPipeline();

    thread.endRendering();

    thread.getColor0()->transitionState(thread.cmd(), Image::State::COMPUTE_R);
    image->transitionState(thread.cmd(), Image::State::COMPUTE_RW);

    thread.setPipeline(pipeline);

    struct PC {
        vec2i viewport_size;
        vec2i target_offset;
        vec4i rounding_corners_radius_px;
        int pass;
        float sigma;
        int radius;
    } pc{};

    pc.viewport_size = ctx.viewport.size();
    pc.target_offset = ctx.viewport.pos();
    pc.rounding_corners_radius_px = ctx.rounding_corners_radius_px;
    pc.sigma = sigma;
    pc.radius = radius;
    pc.pass = 0;

    thread.pushConstants(&pc);

    auto src_di0 = thread.getColor0()->getDescriptorInfo();
    auto dst_di0 = image->getDescriptorInfo();

    auto pass0_writes = pipeline->makeWrites(0, nullptr, &src_di0, &dst_di0);

    thread.pushDescriptor(0, pass0_writes);

    thread.dispatchCompute((ctx.viewport.width() / 16) + 1, (ctx.viewport.height() / 16) + 1, 1);

    image->transitionState(thread.cmd(), Image::State::COMPUTE_R);
    thread.getColor0()->transitionState(thread.cmd(), Image::State::COMPUTE_RW);

    pc.pass = 1;

    thread.pushConstants(&pc);

    auto src_di1 = image->getDescriptorInfo();
    auto dst_di1 = thread.getColor0()->getDescriptorInfo();

    auto pass1_writes = pipeline->makeWrites(0, nullptr, &src_di1, &dst_di1);

    thread.pushDescriptor(0, pass1_writes);

    thread.dispatchCompute((ctx.viewport.width() / 16) + 1, (ctx.viewport.height() / 16) + 1, 1);

    thread.beginRendering();
};
}  // namespace mle::ui::shader
