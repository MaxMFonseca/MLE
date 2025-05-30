#pragma once

#include "mle/ui/element_renderable/Shader.h"
namespace mle::ui::element_renderable {
class ProgressBar : public Shader {
  public:
    ProgressBar(const ProgressBar&) = delete;
    ProgressBar(ProgressBar&&) = delete;
    ProgressBar& operator=(const ProgressBar&) = delete;
    ProgressBar& operator=(ProgressBar&&) = delete;

    explicit ProgressBar(ElementRef element);
    ~ProgressBar() override = default;

    void set(const sol::table& table) override;
    bool render() override;
    void exec() const override;

    [[nodiscard]] f32 getProgress() const { return pc_data_.progress; }
    [[nodiscard]] Color getColor0() const { return pc_data_.color0; }
    [[nodiscard]] Color getColor1() const { return pc_data_.color1; }

    void setProgress(f32 v);
    void setColor0(const sol::object& v) { setColor0(Color::fromLua(v)); }
    void setColor0(Color v);
    void setColor1(const sol::object& v) { setColor1(Color::fromLua(v)); }
    void setColor1(Color v);

    static void registerLuaType();

  private:
    mle::renderer::PipelineRef pipeline_{};

    struct {
        VertexData vertex_data{};
        Color color0 = Color::BLACK;
        Color color1 = Color::WHITE;
        f32 progress = 0;
    } pc_data_;
};
}  // namespace mle::ui::element_renderable
