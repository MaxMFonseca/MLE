#pragma once

#include "mle/ui/shader/ShaderI.h"

namespace mle::ui::shader {
struct LuaShaderPacket : public ShaderPacketI {
    using ParamVariant = std::variant<std::string, float, int, bool, vec2f, vec2i, vec4f, vec4i>;
    using ParamMap = std::map<std::string, ParamVariant>;

    LuaShaderPacket() = default;
    ~LuaShaderPacket() override = default;

    MLE_NO_COPY_MOVE(LuaShaderPacket);

    ParamMap params;

    const Pipeline* pipeline{};
    void render(CompRenderingCtx& ctx) override;
};

struct LuaShader : public ShaderI {
    const sol::table table;

    const Pipeline* pipeline{};
    LuaShaderPacket::ParamMap params;

    void set(const sol::object& obj) override;
    void setParams(const sol::table& table);
    void setParam(const std::string& name, const sol::object& obj);

    void setPipeline(const sol::object& obj);

    void doUpdatePacket(ShaderPacketI* packet) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"LuaShader"}; }

    static void apply(const Entt& ew, const sol::object& obj);
    static void applyParams(const Entt& ew, const sol::object& obj);
};
}  // namespace mle::ui::shader
