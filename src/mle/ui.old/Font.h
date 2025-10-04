#pragma once

#include <stb_truetype.h>

#include "Types.h"
#include "mle/common/Consts.h"
#include "mle/common/RectPacker.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types2D.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"

namespace mle::ui {
class Font {
  public:
    static constexpr u32 INVALID_CODEPOINT = 0xDFFF;

    struct Glyph {
        u32 texture_idx{};
        Rectf texture_rect{};

        f32 advance = 0;
        f32 lsb = 0;
        vec2f size{0};
        vec2f origin{0};
    };

    struct RenderText {
        struct Token {
            enum class Type : u8 { CHAR, SPACE, BR, INVALID };
            Type type = Type::INVALID;
            u32 texture_idx{};
            Rectf texture_rect{};
            Rectf rect{};
        };
        f32 width = 0;
        u32 char_count = 0;
        std::vector<Token> tokens{};
    };

    struct CreateInfo {
        std::string name;
        fs::path path;

        std::u32string pre_load_string;
    };
    using CI = CreateInfo;

  public:
    MLE_NO_COPY_MOVE(Font)

    ~Font() = default;

    [[nodiscard]] auto getHeight() const { return height_; }
    [[nodiscard]] RenderText makeText(const std::string& text);

  private:
    friend class detail::FontCache;
    friend struct fmt::formatter<mle::ui::Font>;

    Font() = default;

    void init(CI ci);

    Rectu addTexture(const void* data, vec2u size);
    void createAtlas();
    Glyph getGlyph(char32 codepoint);
    Glyph loadCodepoint(char32 codepoint);

  private:
    std::string name_;
    Bytes ttf_data_{};
    stbtt_fontinfo f_info_{};
    f32 height_ = DEFAULT_FONT_HEIGHT;
    f32 scale_ = 0.0F;
    f32 ascent_ = 0;
    f32 descent_ = 0;
    f32 line_gap_ = 0;
    int sdf_padding_ = 0;

    std::map<char32, Glyph> chars_{};
    Glyph default_char_{};

    vec2u atlas_size_{DEFAULT_FONT_HEIGHT * 16};
    std::vector<u32> atlases_;
    RectPacker packer_{};
    bool atlas_full_ = true;
};
}  // namespace mle::ui

namespace fmt {
template <>
struct formatter<mle::ui::Font::Glyph> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Font::Glyph& glyph, FormatContext& ctx) const {
        return format_to(ctx.out(), "[ texture_idx: {}, texture_rect: {}, advance: {}, lsb: {}, size: {}, origin: {} ]", glyph.texture_idx, glyph.texture_rect,
                         glyph.advance, glyph.lsb, glyph.size, glyph.origin);
    }
};

template <>
struct formatter<mle::ui::Font> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Font& font, FormatContext& ctx) const {
        auto out = ctx.out();
        out = fmt::format_to(out, "Font {{\n");
        out = fmt::format_to(out, "  name: \"{}\",\n", font.name_);
        out = fmt::format_to(out, "  scale: {:.3f}, ascent: {}, descent: {}, line_gap: {}, height: {}, sdf_padding: {},\n", font.scale_, font.ascent_,
                             font.descent_, font.line_gap_, font.height_, font.sdf_padding_);
        out = fmt::format_to(out, "  ttf_data: [{} bytes],\n", font.ttf_data_.size());

        out = fmt::format_to(out, "  glyps ({} entries): {{\n", font.chars_.size());
        for (const auto& [codepoint, glyph] : font.chars_) {
            out = fmt::format_to(out, "    codepoint:{} => {}", as<int>(codepoint), glyph);
        }
        out = fmt::format_to(out, "  }},\n");

        out = fmt::format_to(out, "  atlas count: {},\n", font.atlases_.size());

        // // out = fmt::format_to(out, "  atlases ({}): [\n", font.atlases_.size());
        // // for (const auto& atlas : font.atlases_) {
        // //     out = fmt::format_to(out, "    {},\n", atlas);
        // // }
        // // out = fmt::format_to(out, "  ]\n}}\n");

        return out;
    }
};

template <>
struct formatter<mle::ui::Font::RenderText::Token> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Font::RenderText::Token& token, FormatContext& ctx) const {
        return format_to(ctx.out(), "[ type: {}, texture_idx: {}, texture_rect: {}, rect: {} ]", static_cast<u8>(token.type), token.texture_idx,
                         token.texture_rect, token.rect);
    }
};

template <>
struct formatter<mle::ui::Font::RenderText> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Font::RenderText& text, FormatContext& ctx) const {
        auto out = ctx.out();
        out = fmt::format_to(out, "RenderText {{\n");
        out = fmt::format_to(out, "  width: {},\n", text.width);
        out = fmt::format_to(out, "  tokens ({}): [\n", text.tokens.size());
        for (const auto& token : text.tokens) {
            out = fmt::format_to(out, "    {},\n", token);
        }
        out = fmt::format_to(out, "  ]\n}}\n");
        return out;
    }
};

}  // namespace fmt
