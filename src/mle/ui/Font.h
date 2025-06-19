#pragma once

#include <stb_truetype.h>

#include "Types.h"
#include "mle/common/RectPacker.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types2D.h"

namespace mle::ui {
class Font {
  public:
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

    struct Glyph {
        u32 texture_idx;
        Rectu rect;
    };
    std::map<char32_t, Glyph> glyps_{};

    std::vector<RectPacker> atlases_;
};
}  // namespace mle::ui

namespace fmt {
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

        out = fmt::format_to(out, "  glyps ({} entries): {{\n", font.glyps_.size());
        for (const auto& [codepoint, glyph] : font.glyps_) {
            out = fmt::format_to(out, "    codepoint:{} => {{ tex: {}, rect: {} }},\n", as<int>(codepoint), glyph.texture_idx, glyph.rect);
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
