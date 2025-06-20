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

        int advance = 0;
        int lsb = 0;
        vec2i size{0};
        vec2i origin{0};
    };

    struct Text {};

    struct CreateInfo {
        std::string name;
        fs::path path = "";
        bool engine = false;

        std::u32string pre_load_string;
    };
    using CI = CreateInfo;

  public:
    MLE_NO_COPY_MOVE(Font)

    ~Font() = default;

  private:
    friend class detail::FontCache;
    friend struct fmt::formatter<mle::ui::Font>;

    Font() = default;

    void init(CI ci);

    Rectu addTexture(const void* data, vec2u size);
    void createAtlas();
    Glyph loadCodepoint(char32 codepoint);

  private:
    std::string name_;
    stbtt_fontinfo f_info_{};
    Bytes ttf_data_{};
    f32 scale_ = 0.0F;
    int ascent_ = 0;
    int descent_ = 0;
    int line_gap_ = 0;
    int height_ = DEFAULT_FONT_HEIGHT;
    int sdf_padding_ = 0;
    vec2u atlas_size_{DEFAULT_FONT_HEIGHT * 10};

    std::map<char32, Glyph> chars_{};
    Glyph default_char_{};

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
}  // namespace fmt
