#include "Lua.h"

#include <cstring>

#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "sol/forward.hpp"

namespace mle::ui::element_renderable {
Lua::Lua(ElementRef element) :
    Shader(element) {
    MLE_LOG_THIS;
}

void Lua::set(const sol::table& table) {
    if (sol::object r_on_shader_render = table["on_shader_render"]; r_on_shader_render.valid()) {
        setOnShaderRender(r_on_shader_render);
    }
    if (sol::object r_on_pipeline_set = table["onPipelineSet"]; r_on_pipeline_set.valid()) {
        on_pipeline_set_ = r_on_pipeline_set;
    }
    if (sol::object r_on_shader_render = table["onShaderRender"]; r_on_shader_render.valid()) {
        setOnShaderRender(r_on_shader_render);
    }
    if (sol::object r_pipeline = table["pipeline"]; r_pipeline.valid()) {
        setPipeline(r_pipeline);
    }
}

void Lua::setOnShaderRender(const sol::function& v) {
    on_shader_render_ = v;
    element_->setRequestingRender();
}

void Lua::setPipeline(const sol::object& v) {
    MLE_ASSERT_LOG(v.is<sol::table>(), "Maybe string?");
    sol::table t = v;
    renderer::PipelineGetResult pgr{};
    if (sol::object r_name = t[1]; r_name.valid()) {
        pgr = renderer::getPipeline(r_name.as<std::string>());
    }
    if (sol::object r_name = t["name"]; r_name.valid()) {
        pgr = renderer::getPipeline(r_name.as<std::string>());
    }
    pipeline_ = pgr.pipeline;
    if (pgr.is_new) {
        auto r_setup = t["setup"];
        MLE_ASSERT(r_setup.valid());
        sol::table setup = r_setup;
        if (auto r_vert = setup["vert"]; r_vert.valid()) {
            pipeline_->setVertexShader(addShadersBasePath(r_vert.get<std::string>()));
        } else {
            pipeline_->setVertexShader(addShadersBasePath("mle/ui/ui_shader_quad.vert"));
            vs_is_default_ = true;
        }
        auto r_frag = setup["frag"];
        MLE_ASSERT(r_frag.valid());
        pipeline_->setFragmentShader(addShadersBasePath(r_frag.get<std::string>()));

        pipeline_->setTopology(vk::PrimitiveTopology::eTriangleStrip);
        pipeline_->setPolygonMode(vk::PolygonMode::eFill);
        pipeline_->setBlendAttachments(mle::renderer::makeDefaultBlendAttachmentStates(1));
        pipeline_->setColorAttachmentFormats({mle::renderer::getDefaultColorFormat()});
        pipeline_->build();
    }

    if (on_pipeline_set_.valid()) {
        on_pipeline_set_(element_, this);
    }
}

bool Lua::render() {
    if (element_->isRequestingRender()) {
        MLE_ASSERT_LOG(on_shader_render_.valid(), "I cant why would this happen");
        if (vs_is_default_) {
            auto vs = makeVertexData();
            std::memcpy(pc_data_.data(), &vs, sizeof(vs));
        }
        on_shader_render_(element_, this);
    }
    return false;
}

void Lua::exec() const {
    MLE_ASSERT_LOG(pipeline_, "Pipeline is not set");
    renderer::Pipeline::ExecInfo exec_info;
    exec_info.instance_count = 1;
    exec_info.index_count = 4;
    exec_info.push_constants = &pc_data_;

    pipeline_->exec(exec_info);
}

void Lua::setFieldFloat(const std::string& name, const sol::object& o) {
    setField(name, lua::asF32(o));
}

void Lua::setFieldVec2(const std::string& name, const sol::object& o) {
    setField(name, lua::asVec2f(o));
}

void Lua::setFieldVec3(const std::string& name, const sol::object& o) {
    setField(name, lua::asVec3f(o));
}

void Lua::setFieldVec4(const std::string& name, const sol::object& o) {
    setField(name, lua::asVec4f(o));
}

void Lua::setFieldMat2(const std::string& name, const sol::object& o) {
    setField(name, o.as<mat2f>());
}

void Lua::setFieldMat4(const std::string& name, const sol::object& o) {
    setField(name, o.as<mat4f>());
}

void Lua::registerLuaTypes() {
    auto ut = lua::newUsertype<Lua>("ui_Element_Renderable_Shader_Lua", sol::no_constructor, sol::base_classes, sol::bases<Element::Renderable, Shader>());
    ut["setFieldF"] = &Lua::setFieldFloat;
    ut["setFieldVec2"] = &Lua::setFieldVec2;
    ut["setFieldVec3"] = &Lua::setFieldVec3;
    ut["setFieldVec4"] = &Lua::setFieldVec4;
    ut["setFieldMat2"] = &Lua::setFieldMat2;
    ut["setFieldMat4"] = &Lua::setFieldMat4;
}
}  // namespace mle::ui::element_renderable
