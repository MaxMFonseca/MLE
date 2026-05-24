#pragma once

#include "mle/renderer/Font.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/renderable/RenderableI.h"
#include "mle/window/TextBox.h"
#include "mle/window/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::renderable {
struct TextPacket : public RenderablePacketI {
    struct CharsInstance {
        vec2f pos;
        vec2f size;
        vec2f texture_pos;
        vec2f texture_size;
    };

    struct PerImageData {
        ImageRef image_ref;
        usize instance_offset;
        usize instance_count;
    };

    BufferHnd chars_buffer;
    std::vector<PerImageData> per_image_data;

    Color color = Color::ONE;
    Color border_color = Color::ZERO;
    f32 border_thickness = 0.F;
    bool bold = false;

    TextPacket() = default;
    ~TextPacket() override;

    MLE_NO_COPY_MOVE(TextPacket);

    void render(CompRenderingCtx& ctx) override;
};

struct Text : public RenderableI {
    std::u32string text;
    entt::id_type font_id{};
    Color color = Color::ONE;
    Color border_color = Color::ZERO;
    f32 border_thickness = 0.F;
    bool bold = false;
    TargetBound font_height_tb;
    Font::JustifyMode justify_mode = Font::JustifyMode::START;
    f32 line_max_aspect = 0.F;
    bool wrap = false;
    bool multiline_input = false;
    bool chars_buffer_needs_update = true;

    Font::RenderText render_text;

    std::vector<TextPacket::PerImageData> per_image_data;
    std::vector<TextPacket::CharsInstance> current_rendering_chars_instance_data;

    f32 font_height_px_f{};

    TextBoxHnd input_tb;

    void makeCharsBuffer(vec2u viewport_size);

    void makeInputBox(const Entt& ew, const sol::object& obj);
    void enableInputBox() const;
    void disableInputBox() const;
    [[nodiscard]] std::string getValue() const;

    void setText(const Entt& ew, std::u32string src);
    void setText(const Entt& ew, std::string_view src);
    void setFont(const Entt& ew, const char* cstr);
    void setColor(const Color& c);
    void setColor(const sol::object& obj) { setColor(Color::fromLua(obj)); }
    void setBorderColor(const Color& c);
    void setBorderColor(const sol::object& obj) { setBorderColor(Color::fromLua(obj)); }
    void setBorderThickness(f32 t);
    void setBold(bool t);
    void setFontHeight(const Entt& ew, const sol::object& obj);
    void setJustifyMode(const Entt& ew, std::string_view mode_str);
    void setLineMaxAspect(const Entt& ew, f32 v);
    void setWrap(const Entt& ew, bool w = true);

    void set(const Entt& ew, const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds(const Entt& ew, vec2u max_size) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"Text"}; }

    void doUpdatePacket(const Entt& ew, RenderablePacketI* packet) override;

    Text() = default;
    ~Text() override;

    MLE_NO_COPY_MOVE(Text);

    static void apply(const Entt& ew, const sol::object& obj);
    static void applyInputEnable(const Entt& ew, const sol::object& obj);
    static void applyInputDisable(const Entt& ew, const sol::object& obj);
    static void applyInputClear(const Entt& ew, const sol::object& obj);

    static Expected<std::reference_wrapper<Text>> getFromEntt(const Entt& ew);
};
}  // namespace mle::ui::renderable
