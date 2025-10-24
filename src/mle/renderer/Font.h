#pragma once

#include <map>

#include "mle/math/Types2D.h"
#include "mle/renderer/TextureAtlas.h"
#include "mle/utils/ECS.h"
#include "mle/utils/File.h"
#include "mle/utils/Justify.h"
#include "stb_truetype.h"

namespace mle {
class Font final {
  public:
    static constexpr u32 INVALID_CODEPOINT = 0xDFFF;
    static constexpr int DEFAULT_FONT_HEIGHT = 32;

    struct Glyph {
        char32 codepoint{INVALID_CODEPOINT};
        f32 advance = 0;
        f32 lsb = 0;
        vec2f size{0};
        vec2f origin{0};
    };

    struct CreateInfo {
        Path ttf_path;
        int font_height = DEFAULT_FONT_HEIGHT;
        int atlas_scale = 16;
        bool init_load_ascii = true;
    };
    using CI = CreateInfo;

    enum class JustifyMode : u8 {
        START = as<u8>(JustifyF32::LineMode::START),
        CENTER = as<u8>(JustifyF32::LineMode::CENTER),
        END = as<u8>(JustifyF32::LineMode::END),
        SPACE_BETWEEN = as<u8>(JustifyF32::LineMode::SPACE_BETWEEN),
    };

    struct MakeTextIn {
        std::u32string str;
        JustifyMode justify_mode = JustifyMode::START;
        bool wrap = false;
        f32 line_max_aspect = 0;
    };

    struct RenderText {
        struct Char {
            char32 codepoint{INVALID_CODEPOINT};
            u32 idx{};
            Rectf rect{};
        };
        std::vector<Char> chars{};
        u32 line_count = 0;
        f32 line_max_width = 0;
    };

  public:
    MLE_NO_COPY_MOVE(Font)

    ~Font() = default;

    [[nodiscard]] auto getHeight() const { return height_; }
    [[nodiscard]] RenderText makeText(const MakeTextIn& in);
    std::pair<ImageRef, TextureAtlas::Entry> getTextureEntry(char32 codepoint);

  private:
    friend class FontCache;
    Font() = default;

    Result init(const CI& ci);

    void addTexture(char32 codepoint, const void* data, vec2u size);
    Glyph getGlyph(char32 codepoint);
    void loadString(const std::u32string& str);
    Expected<Glyph> loadCodepoint(char32 codepoint);

  private:
    Bytes ttf_data_{};
    stbtt_fontinfo fontinfo_{};
    int height_i_ = DEFAULT_FONT_HEIGHT;
    f32 height_ = DEFAULT_FONT_HEIGHT;
    f32 scale_ = 0;
    f32 ascent_ = 0;
    f32 descent_ = 0;
    f32 line_gap_ = 0;
    int sdf_padding_ = 0;

    std::unordered_map<char32, Glyph> cps_{};
    Glyph default_char_{};

    std::mutex atlas_mutex_{};
    TextureAtlas atlas_{};
};
}  // namespace mle
