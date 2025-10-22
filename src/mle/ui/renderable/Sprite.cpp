#include "Sprite.h"

#include "../Entt.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/components/Renderable.h"

namespace mle::ui::renderable {
void Sprite::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to Sprite::apply for entt {}", e.e());
        return;
    }
    auto* renderable = e.tryGet<comp::Renderable>();
    Sprite* self_p = nullptr;
    if (!renderable) {
        renderable = &e.emplace<comp::Renderable>(std::make_unique<Sprite>());
        self_p = as<Sprite*>(renderable->impl.get());
    } else {
        MLE_ASSERT(renderable->impl);
        if (renderable->impl->getType() != Sprite::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType().data(),
                  Sprite::type().data());
            return;
        }

        self_p = as<Sprite*>(renderable->impl.get());
    }
    self_p->set(obj);
};

void Sprite::setTexture(const std::string& src) {
    texture_id = entt::hashed_string{src.c_str()};
    auto load_r = Renderer::i().textureCache().loadTexture(src);
    if (load_r.has_value()) {
        image = load_r.value();
    } else if (load_r.error() != Result::NOT_READY) {
        MLE_E("Failed to load texture {}: {}", src, load_r.error());
        texture_id = {};
    }
}

void Sprite::set(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<std::string>()) {
        auto texture_src = obj.as<std::string>();
        texture_id = entt::hashed_string{texture_src.c_str()};
        image = nullptr;
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto texture_r = lua::getFirstKey(table, "texture", 1); lua::valid<std::string>(texture_r)) {
            setTexture(texture_r.as<std::string>());
        }
        return;
    }

    MLE_W("Unsupported object type provided to Sprite::set");
};

namespace {
auto getPipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/rect.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/ui/sprite.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("mle_ui_sprite", pipeline_ci);
    }
    return pipeline;
}
}  // namespace

void Sprite::render(Ctx& ctx) {
    if (texture_id == 0) {
        return;
    }
    if (!image) {
        auto load_r = Renderer::i().textureCache().get(texture_id);
        if (load_r.has_value()) {
            image = load_r.value();
        } else if (load_r.error() != Result::NOT_READY) {
            MLE_E("Failed to get texture id {}: {}", texture_id, load_r.error());
            texture_id = 0;
            return;
        }
    }

    auto& thread = ctx.thread;

    const auto* pipeline = getPipeline();

    thread.setPipeline(pipeline);

    vk::DescriptorImageInfo b0_0_di = image->getDescriptorInfo();
    auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);

    thread.pushDescriptor(0, push_writes);

    thread.draw(4, 1, 0, 0);
};

}  // namespace mle::ui::renderable
