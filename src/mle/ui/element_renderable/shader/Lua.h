#pragma once

#include "mle/renderer/Pipeline.h"
#include "mle/ui/element_renderable/Shader.h"

namespace mle::ui::element_renderable {
class Lua : public Shader {
  public:
    Lua(const Lua&) = delete;
    Lua(Lua&&) = delete;
    Lua& operator=(const Lua&) = delete;
    Lua& operator=(Lua&&) = delete;

    explicit Lua(ElementRef element);
    ~Lua() override = default;

    void set(const sol::table& table) override;
    bool render() override;
    void exec() const override;

    void setOnShaderRender(const sol::function& v);
    void setPipeline(const sol::object& v);

    template <typename T>
    void setField(const std::string& name, const T& v) {
        auto f = pipeline_->getPushConstantField(name);
        MLE_ASSERT(renderer::ShaderModule::sizeOf(f.type) == sizeof(T));
        memcpy(pc_data_.data() + f.offset, &v, sizeof(T));
    }

    void setFieldFloat(const std::string& name, const sol::object& o);
    void setFieldVec2(const std::string& name, const sol::object& o);
    void setFieldVec3(const std::string& name, const sol::object& o);
    void setFieldVec4(const std::string& name, const sol::object& o);
    void setFieldMat2(const std::string& name, const sol::object& o);
    void setFieldMat4(const std::string& name, const sol::object& o);
    static void registerLuaTypes();

  private:
    mle::renderer::PipelineRef pipeline_{};
    sol::function on_shader_render_;
    sol::function on_pipeline_set_;
    std::array<byte, 128> pc_data_{static_cast<byte>(0)};
    bool vs_is_default_ = false;
};
}  // namespace mle::ui::element_renderable
