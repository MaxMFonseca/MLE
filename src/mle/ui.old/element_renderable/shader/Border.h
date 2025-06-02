#pragma once

#include "mle/ui/element_renderable/Shader.h"

namespace mle::ui::element_renderable {
class Border : public Shader {
  public:
    Border(const Border&) = delete;
    Border(Border&&) = delete;
    Border& operator=(const Border&) = delete;
    Border& operator=(Border&&) = delete;

    explicit Border(ElementRef element);
    ~Border() override = default;

    void set(const sol::table& table) override;
    bool render() override;
    void exec() const override;

    void setRoundness(const sol::object& obj);
    void setRoundness(const vec4f& roundness);
    void setColor(const sol::object& obj);
    void setColor(const Color& color);
    void setEdgePos(const sol::object& obj);
    void setEdgePos(f32 edge_pos);
    void setEdgeThickness(const sol::object& obj);
    void setEdgeThickness(f32 edge_thickness);

  private:
    mle::renderer::PipelineRef pipeline_;

    struct UBO {
        vec2f clamp_pos;
        vec2f clamp_size;
        vec4f roundness{0};
        Color color = Color::WHITE;
        f32 aspect_ratio;
        f32 edge_pos{0.01};
        f32 edge_thickness{0.1};
    } ubo_data_;
};
}  // namespace mle::ui::element_renderable
