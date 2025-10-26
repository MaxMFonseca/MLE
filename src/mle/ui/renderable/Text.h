#pragma once

#include "mle/renderer/Font.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/renderable/RenderableI.h"
#include "mle/window/Types.h"

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

    std::unique_ptr<Buffer> chars_buffer;
    std::vector<PerImageData> per_image_data;

    Color color = Color::ONE;

    TextPacket() = default;
    ~TextPacket() override = default;

    MLE_NO_COPY_MOVE(TextPacket);

    void render(CompRenderingCtx& ctx) override;
};

struct Text : public RenderableI {
    std::u32string text;
    entt::id_type font_id{};
    Color color = Color::ONE;
    TargetBound font_height_tb;
    Font::JustifyMode justify_mode = Font::JustifyMode::START;
    f32 line_max_aspect = 0.F;

    Font::RenderText render_text;

    std::vector<TextPacket::PerImageData> per_image_data;
    std::vector<TextPacket::CharsInstance> current_rendering_chars_instance_data;

    f32 font_height_px_f{};

    TextBoxHnd input_tb;

    bool wrap = false;
    bool multiline_input = false;

    void makeCharsBuffer();

    void setText(std::u32string src);
    void setText(std::string_view src);

    void makeInputBox(const Entt& e, const sol::object& obj);
    void enableInputBox() const;
    void disableInputBox() const;

    void setFont(const char* cstr) { font_id = entt::hashed_string{cstr}; }

    void setJustifyMode(std::string_view mode_str);
    void setWrap(bool w = true) { wrap = w; }

    void setColor(const Color& c);
    void setColor(const sol::object& obj) { setColor(Color::fromLua(obj)); }

    void set(const Entt& e, const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds(const Entt& e, vec2u max_size) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"Text"}; }

    void doUpdatePacket(RenderablePacketI* packet) override;

    ~Text() override;

    static void apply(const Entt& e, const sol::object& obj);
    static void applyInputEnable(const Entt& e, const sol::object& obj);
    static void applyInputDisable(const Entt& e, const sol::object& obj);
    static void applyInputClear(const Entt& e, const sol::object& obj);

    static Expected<std::reference_wrapper<Text>> getFromEntt(const Entt& e);
};
}  // namespace mle::ui::renderable
