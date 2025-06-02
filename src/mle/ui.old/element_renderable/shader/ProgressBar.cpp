#include "ProgressBar.h"

#include "mle/lua/Lua.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"

namespace mle::ui::element_renderable {
ProgressBar::ProgressBar(ElementRef element) :
    Shader(element) {
    MLE_LOG_THIS;
    auto pgr = renderer::getPipeline("uiProgressBar");
    pipeline_ = pgr.pipeline;
    if (pgr.is_new) {
        pipeline_->setVertexShader(addShadersBasePath("mle/ui/ui_shader_quad.vert"));
        pipeline_->setFragmentShader(addShadersBasePath("mle/ui/progress_bar.frag"));
        pipeline_->setTopology(vk::PrimitiveTopology::eTriangleStrip);
        pipeline_->setPolygonMode(vk::PolygonMode::eFill);
        pipeline_->setBlendAttachments(mle::renderer::makeDefaultBlendAttachmentStates(1));
        pipeline_->setColorAttachmentFormats({mle::renderer::getDefaultColorFormat()});
    }

    registerLuaType();
}

void ProgressBar::registerLuaType() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    void (ProgressBar::*set_color0_o)(const sol::object&) = &ProgressBar::setColor0;
    void (ProgressBar::*set_color1_o)(const sol::object&) = &ProgressBar::setColor1;

    auto sut = lua::newUsertype<ProgressBar>("ui_Element_Renderable_Shader_ProgressBar", sol::no_constructor, sol::base_classes,
                                             sol::bases<Element::Renderable, Shader>());

    sut["progress"] = sol::property(&ProgressBar::getProgress, &ProgressBar::setProgress);
    sut["color0"] = sol::property(&ProgressBar::getColor0, set_color0_o);
    sut["color1"] = sol::property(&ProgressBar::getColor1, set_color1_o);

    auto rut = lua::getUserType<Element::Renderable>("ui_Element_Renderable");
    rut["asProgressBar"] = &Element::Renderable::as<ProgressBar>;
}

void ProgressBar::set(const sol::table& table) {
    if (sol::object r_progress = table["progress"]; r_progress.valid()) {
        setProgress(r_progress.as<f32>());
    }
    if (sol::object r_color0 = table["color0"]; r_color0.valid()) {
        setColor0(r_color0);
    }
    if (sol::object r_color1 = table["color1"]; r_color1.valid()) {
        setColor1(r_color1);
    }
}

void ProgressBar::setProgress(f32 v) {
    pc_data_.progress = v;
    element_->setRequestingRender();
}

void ProgressBar::setColor0(Color v) {
    v = v.toLinear();
    if (pc_data_.color0 != v) {
        pc_data_.color0 = v;
        element_->setRequestingRender();
    }
}

void ProgressBar::setColor1(Color v) {
    v = v.toLinear();
    if (pc_data_.color1 != v) {
        pc_data_.color1 = v;
        element_->setRequestingRender();
    }
}

bool ProgressBar::render() {
    if (element_->isRequestingRender()) {
        pc_data_.vertex_data = makeVertexData();
    }
    return false;
}

void ProgressBar::exec() const {
    pipeline_->build();

    renderer::Pipeline::ExecInfo exec_info;
    exec_info.instance_count = 1;
    exec_info.index_count = 4;
    exec_info.push_constants = &pc_data_;

    pipeline_->exec(exec_info);
}
}  // namespace mle::ui::element_renderable
