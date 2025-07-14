#include "Shader.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/element/Types.h"

namespace mle::ui::element {
void Shader::render(const RenderContext& ctx) const {
    MLE_ASSERT(pipeline_);

    auto table = fn_(EntityWrapper{ctx.self});

    ctx.thread->setPipeline(pipeline_.get());

    if (const auto pc_table_r = lua::tryGetKey<sol::table>(table, "pc"); pc_table_r) {
        std::array<u8, 128> pc_data{0};
        for (const auto& [key, value] : *pc_table_r) {
            auto pc_field = pipeline_->getPushConstantField(key.as<std::string>());
            switch (pc_field.type) {
                using DT = renderer::DataType;
                case DT::F32: {
                    auto v = lua::as<f32>(value);
                    std::memcpy(pc_data.data() + pc_field.offset, &v, pc_field.size);
                } break;
                case DT::I32: {
                    auto v = lua::as<i32>(value);
                    std::memcpy(pc_data.data() + pc_field.offset, &v, pc_field.size);
                } break;
                case DT::U32: {
                    auto v = lua::as<u32>(value);
                    std::memcpy(pc_data.data() + pc_field.offset, &v, pc_field.size);
                } break;
                case DT::VEC2F: {
                    auto v = lua::as<vec2f>(value);
                    std::memcpy(pc_data.data() + pc_field.offset, &v, pc_field.size);
                } break;
                case DT::VEC4F: {
                    auto v = lua::as<vec4f>(value);
                    std::memcpy(pc_data.data() + pc_field.offset, &v, pc_field.size);
                } break;
                default:
                    MLE_UNREACHABLE_LOG("Unsupported push constant type: {}", pc_field.type);
                    break;
            }
        }
        ctx.thread->pushConstants(pc_data.data());
    }

    ctx.thread->draw(1, 4);
}

void Shader::apply(entt::entity /*self*/, const sol::object& o) {
    auto table = lua::as<sol::table>(o);

    renderer::Pipeline::CI pipeline_ci;
    auto vs = lua::getKey<std::string>(table, "vert");
    auto fs = lua::getKey<std::string>(table, "frag");
    pipeline_ci.vertex_shader = renderer::getShader(vs + ".vert");
    pipeline_ci.fragment_shader = renderer::getShader(fs + ".frag");
    pipeline_ci.color_attachment_formats.emplace_back(renderer::getDefaultColorFormat());
    pipeline_ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
    pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
    pipeline_ = renderer::Pipeline::createHnd(pipeline_ci);

    fn_ = lua::getKey<sol::function>(table, "fn");
}
}  // namespace mle::ui::element
