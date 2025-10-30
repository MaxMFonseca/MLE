#include "LuaShader.h"

#include "mle/client/Client.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "sol/forward.hpp"

namespace mle::ui::shader {
void LuaShader::apply(const Entt& ew, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to LuaShader::apply for entt {}", ew.e());
        return;
    }
    auto* shader = ew.tryGet<comp::Shader>();
    LuaShader* self_p = nullptr;
    if (!shader) {
        shader = &ew.emplace<comp::Shader>(std::make_unique<LuaShader>());
        self_p = as<LuaShader*>(shader->impl.get());
        shader->packet_buffers_.at(0) = std::make_shared<LuaShaderPacket>();
        shader->packet_buffers_.at(1) = std::make_shared<LuaShaderPacket>();
        shader->packet_buffers_.at(2) = std::make_shared<LuaShaderPacket>();
    } else {
        MLE_ASSERT(shader->impl);
        if (shader->impl->getType() != LuaShader::type()) {
            MLE_E("Shader::apply called on entt {} with incompatible Shader type. {}x{}", ew.fullName(), shader->impl->getType(), LuaShader::type());
            return;
        }
        self_p = as<LuaShader*>(shader->impl.get());
    }
    self_p->set(obj);
}

void LuaShader::applyParams(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (!lua::valid<sol::table>(obj)) {
        MLE_E("Shader::applySetShaderParams called on entt {} with wrong obj type. {}", ew.fullName(), obj.get_type());
        return;
    }
    auto table = obj.as<sol::table>();
    MLE_ASSERT(ew.has<comp::Shader>());
    auto& shader = ew.get<comp::Shader>();
    MLE_ASSERT(shader.impl);
    if (shader.impl->getType() != LuaShader::type()) {
        MLE_E("Shader::applySetShaderParams called on entt {} with incompatible Shader type. {}x{}", ew.fullName(), shader.impl->getType(), LuaShader::type());
        return;
    }
    auto* self_p = as<LuaShader*>(shader.impl.get());
    auto& self = *self_p;
    for (const auto& [key, value] : table) {
        self.setParam(lua::as<std::string>(key), value);
    }
}

void LuaShader::setParams(const sol::table& table) {
    for (const auto& [key, value] : table) {
        if (key.get_type() != sol::type::string) {
            MLE_E("LuaShader::setParams key is not a string. {}", key.get_type());
            continue;
        }
        setParam(lua::as<std::string>(key), value);
    }
};

void LuaShader::setParam(const std::string& name, const sol::object& obj) {
    // std::map<std::string, std::variant<std::string, float, int, bool, vec2f, vec2i, vec4f, vec4i>> params;
    if (obj.is<std::string>()) {
        params[name] = obj.as<std::string>();
    } else if (obj.is<float>()) {
        params[name] = obj.as<float>();
    } else if (obj.is<vec4f>()) {
        params[name] = obj.as<vec4f>();
    } else if (obj.is<vec2f>()) {
        params[name] = obj.as<vec2f>();
    } else if (obj.is<vec4i>()) {
        params[name] = obj.as<vec4i>();
    } else if (obj.is<vec2i>()) {
        params[name] = obj.as<vec2i>();
    } else if (obj.is<int>()) {
        params[name] = obj.as<int>();
    } else if (obj.is<bool>()) {
        params[name] = obj.as<bool>();
    } else {
        MLE_E("Unsupported param T passed to LuaShader on key: {}", name);
    }
    versionUp();
};

void LuaShader::set(const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to LuaShader::set");
        return;
    }

    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto pipeline_r = table["pipeline"]; pipeline_r.valid()) {
            setPipeline(pipeline_r);
        }
        if (const auto params_r = table["params"]; lua::valid<sol::table>(params_r)) {
            setParams(params_r.get<sol::table>());
        }
        setBase(table);
        return;
    }

    MLE_W("Unsupported object type provided to LuaShader::set");
};

void LuaShader::setPipeline(const sol::object& obj) {
    if (obj.is<sol::table>()) {
        pipeline = Renderer::i().pipelineCache().tryLuaPipeline(obj);
    } else if (obj.is<std::string>()) {
        pipeline = Renderer::i().pipelineCache().tryGetPipeline(obj.as<std::string>());
    } else {
        MLE_E("Unexpected pipeline field type on shader. {}", obj.get_type());
        return;
    }
    versionUp();
};

void LuaShader::doUpdatePacket(ShaderPacketI* packet) {
    auto* packet_p = as<LuaShaderPacket*>(packet);

    if (pipeline == nullptr) {
        packet_p->pipeline = nullptr;
        MLE_W("Shader without pipeline.");
        return;
    }
    packet_p->pipeline = pipeline;
    packet_p->params = params;
};

void LuaShaderPacket::render(CompRenderingCtx& ctx) {
    if (!pipeline) {
        return;
    }

    auto& thread = ctx.thread;

    std::array<u8, 128> pc{};

    thread.setPipeline(pipeline);

    for (const Shader::PushConstantField& pc_field : pipeline->getPushConstantFields()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) It is checked
        u8* data = pc.data() + pc_field.offset;
        MLE_ASSERT((pc_field.offset + pc_field.size) <= pc.size());

        if (pc_field.name == "in_time") {
            f32 s = Client::i().getRunningStopwatch().elapsedSecFloat();
            std::memcpy(data, &s, sizeof(float));
            continue;
        }
        if (pc_field.name == "in_viewport_size") {
            vec2f viewport = ctx.thread.getViewport().size();
            std::memcpy(data, &viewport, sizeof(vec2f));
            continue;
        }
        if (pc_field.name == "in_rounding_corners_radius") {
            std::memcpy(data, &ctx.rounding_corners_radius_px, sizeof(vec4i));
            continue;
        }

        auto it = std::ranges::find_if(params, [&](auto& p) { return p.first == std::string("pc_") + pc_field.name; });
        if (it == params.end()) {
            continue;
        }

        switch (pc_field.type) {
            case Shader::DataType::FLOAT: {
                auto* v_r = std::get_if<float>(&it->second);
                if (v_r) {
                    std::memcpy(data, v_r, sizeof(float));
                } else {
                    MLE_W("Push constant field {} expected FLOAT type.", pc_field.name);
                }
            } break;
            case Shader::DataType::VEC4: {
                auto* v_r = std::get_if<vec4f>(&it->second);
                if (v_r) {
                    std::memcpy(data, v_r, sizeof(vec4f));
                } else {
                    MLE_W("Push constant field {} expected VEC4 type.", pc_field.name);
                }
            } break;
            default:
                MLE_TODO;
        }
    }

    thread.pushConstants(&pc);

    thread.draw(4, 1, 0, 0);
};

}  // namespace mle::ui::shader
