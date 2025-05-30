#pragma once

#include "mle/ui/Element.h"
#include "mle/ui/Types.h"

namespace mle::ui::element_renderable {
class Shader : public Element::Renderable {
  public:
    struct VertexData {
        vec2f pos;
        vec2f size;
        vec2f clamp_pos;
        vec2f clamp_size;
    };

  public:
    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&&) = delete;

    explicit Shader(ElementRef element) :
        Element::Renderable(element) {}
    ~Shader() override = default;

    virtual void exec() const = 0;

    void log(const std::string& prefix) const override { MLE_T("{}  Shader: {}", prefix, (void*)this); }
    void getRenderData(int layer, RenderableData& data) const override { data[layer].shaders.emplace_back(this); }

    [[nodiscard]] VertexData makeVertexData() const;

  private:
};
}  // namespace mle::ui::element_renderable
