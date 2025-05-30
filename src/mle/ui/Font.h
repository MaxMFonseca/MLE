#pragma once

#include <stb_truetype.h>

#include <expected>
#include <map>

#include "mle/common/UnicodeBuffer.h"
#include "mle/renderer/TextureAtlas.h"
#include "mle/ui/Types.h"

namespace mle::ui {
class Font {
  public:
    struct CreateInfo {
        std::string font_name{};
        int height{};
        f32 cursor_width = 0.05;
        vec2i padding{};
        vec2i atlas_chars_per_dim{};
        UnicodeBuffer pre_load_string{};
    };
    using CI = CreateInfo;

    struct CodepointData {
        int glyph_idx = 0;
        f32 advance = 0, lsb = 0;
        vec2f size{}, origin{};
        renderer::Texture texture{};
    };

  public:
    Font(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(const Font&) = delete;
    Font& operator=(Font&&) = delete;

    explicit Font(const CI& ci, Bytes&& ttf_data, renderer::TextureAtlas& atlas);
    ~Font() = default;

    [[nodiscard]] const auto& getName() const { return font_name_; }
    [[nodiscard]] auto getHeigh() const { return height_; }
    [[nodiscard]] auto getLineGap() const { return line_gap_; }
    [[nodiscard]] auto getAscent() const { return ascent_; }
    [[nodiscard]] auto getDescent() const { return descent_; }
    [[nodiscard]] f32 getGlyphKernAdvance(int a, int b) const;
    [[nodiscard]] f32 getCursorWidth() const { return cursor_width_; }

    void loadChars(const UnicodeBuffer& chars);

    static Bytes loadFont(const fs::path& path);

    const CodepointData& getCodepointData(char32 c);

  private:
    const CodepointData& loadCodepoint(char32 c);

  private:
    renderer::TextureAtlas& atlas_;
    Bytes ttf_data_;

    std::string font_name_;

    stbtt_fontinfo font_info_{};

    f32 height_ = 0;
    f32 scale_ = 0;

    f32 ascent_ = 0;
    f32 descent_ = 0;
    f32 line_gap_ = 0;
    f32 cursor_width_ = 0;

    int sdf_padding_ = 0;

    std::string group_name_;

    std::map<char32, CodepointData> characters_;
    CodepointData default_char_{};
};

class FontManager final {
  public:
    FontManager(const FontManager&) = delete;
    FontManager(FontManager&&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager& operator=(FontManager&&) = delete;

    FontManager();
    ~FontManager() = default;

    Result update() { return atlas_->update(); }

    FontRef addFont(const sol::table& table);
    FontRef addFont(Font::CI ci, const fs::path& path = "");

    FontRef getFont(const std::string& name = "");

    auto& getAtlas() { return atlas_; }

    void setDefaultFont(const std::string& name) { default_font_ = getFont(name); }
    void setDefaultFont(FontRef font) { default_font_ = font; }

  private:
    std::map<std::string, FontHnd> fonts_;
    renderer::TextureAtlasHnd atlas_;
    FontRef default_font_{};
};
}  // namespace mle::ui

namespace fmt {
template <>
struct formatter<mle::ui::Font::CodepointData> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Font::CodepointData& p, FormatContext& ctx) const {
        return format_to(ctx.out(), "[glyph_idx: {}, advance: {}, lsb: {}, size: {}, origin: {}, texture: {}]", p.glyph_idx, p.advance, p.lsb, p.size, p.origin,
                         p.texture);
    }
};
}  // namespace fmt
