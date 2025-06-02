#include "Font.h"

#include <cstddef>
#include <functional>

#include "mle/common/Types.h"
#include "mle/common/UnicodeBuffer.h"
#include "mle/common/Utils.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/TextureAtlas.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "stb_include.h"

namespace mle::ui {
Font::Font(const CI& ci, Bytes&& ttf_data, renderer::TextureAtlas& atlas) :
    atlas_(atlas),
    ttf_data_(std::move(ttf_data)),
    font_name_(ci.font_name),
    height_(static_cast<f32>(ci.height)),
    sdf_padding_(ci.height / 16) {
    MLE_I("Creating font {}", ci.font_name);

    auto init_font_result = stbtt_InitFont(&font_info_, (u8*)ttf_data_.data(), 0);  // NOLINT
    MLE_ASSERT(init_font_result);

    scale_ = stbtt_ScaleForPixelHeight(&font_info_, height_);

    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&font_info_, &ascent, &descent, &line_gap);

    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetFontBoundingBox(&font_info_, &x0, &y0, &x1, &y1);
    vec2f max_char_size = {static_cast<f32>(x1 - x0) * scale_, static_cast<f32>(y1 - y0) * scale_};

    ascent_ = (static_cast<f32>(ascent) * scale_) / height_;
    descent_ = (static_cast<f32>(descent) * scale_) / height_;
    line_gap_ = (static_cast<f32>(line_gap) * scale_) / height_;

    renderer::TextureAtlas::GroupCreateInfo group_ci;
    group_ci.name = font_name_;
    group_ci.atlas_padding = ci.padding;
    group_ci.atlas_extent = ci.atlas_chars_per_dim * vec2i(static_cast<int>(height_) + ci.padding.x, static_cast<int>(height_) + ci.padding.y);
    group_ci.format = vk::Format::eR8Unorm;
    atlas_.addGroup(group_ci);

    cursor_width_ = (max_char_size.x * ci.cursor_width) / height_;

    sdf_padding_ = static_cast<int>(std::roundf(height_ / 8));

    renderer::Image::RawData default_char_data;
    default_char_data.channels = 1;
    default_char_data.extent = {max_char_size.x / 2.F, height_ * 2 / 3};
    int border_size = std::ceil(std::sqrt(height_));
    default_char_data.pixels.resize(static_cast<usize>(default_char_data.extent.x) * default_char_data.extent.y);
    for (int i = 0; i < default_char_data.extent.y; ++i) {
        for (int j = 0; j < default_char_data.extent.x; ++j) {
            if ((i < border_size) || (i > default_char_data.extent.y - border_size) || (j < border_size) || (j > default_char_data.extent.x - border_size)) {
                default_char_data.pixels[(i * default_char_data.extent.x) + j] = 0xFF;
            }
        }
    }
    default_char_.texture = atlas_.addTexture("default", default_char_data, font_name_);
    default_char_.size = vec2f(default_char_data.extent) / height_;
    default_char_.origin = {-default_char_.size.x * 0.1, -default_char_.size.y};
    default_char_.advance = default_char_.size.x * 1.2F;

    for (usize i = 0; i < ci.pre_load_string.getSize(); i++) {
        if (!loadCodepoint(ci.pre_load_string.get(i)).glyph_idx) {
            MLE_E("Failed to load codepoint of preload string: {}", (int)ci.pre_load_string.get(i));
        };
    }

    MLE_I("Font {} created. height: {}, scale: {}, ascent: {}, descent: {}, cursor_width: {}, line_gap: {}", font_name_, height_, scale_, ascent_, descent_,
          cursor_width_, line_gap_);
    MLE_VD(default_char_);
}

const Font::CodepointData& Font::loadCodepoint(char32 c) {
    MLE_ASSERT(!characters_.contains(c));
    MLE_T("Loading codepoint: {}, font: {}", (u32)c, font_name_);

    auto glyph_idx = stbtt_FindGlyphIndex(&font_info_, static_cast<int>(c));

    if (glyph_idx == 0) {
        MLE_E("codepoint: {} not found in font: {}", (int)c, font_name_);
        return default_char_;
    }

    vec2i size, origin;
    // clang-format off
    auto* sdf_result = stbtt_GetGlyphSDF(
                                         &font_info_, 
                                         scale_, 
                                         glyph_idx, 
                                         sdf_padding_, 
                                         128, 
                                         16.0F, 
                                         &size.x, 
                                         &size.y, 
                                         &origin.x, 
                                         &origin.y);
    // clang-format on

    int advance = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&font_info_, glyph_idx, &advance, &lsb);

    auto& cp_data = characters_[c];
    cp_data.glyph_idx = glyph_idx;
    cp_data.advance = (static_cast<f32>(advance) * scale_) / height_;
    cp_data.lsb = (static_cast<f32>(lsb) * scale_) / height_;

    if (sdf_result != nullptr) {
        cp_data.size = vec2f(size) / height_;
        cp_data.origin = vec2f(origin) / height_;

        renderer::Image::RawData char_data;
        char_data.channels = 1;
        char_data.extent = size;
        auto data_size = static_cast<usize>(size.x) * size.y;
        char_data.pixels.resize(data_size);
        memcpy(char_data.pixels.data(), sdf_result, data_size);
        stbtt_FreeSDF(sdf_result, nullptr);
        cp_data.texture = atlas_.addTexture(std::to_string(c), char_data, font_name_);
    }

    MLE_T("font: {}. Added codepoint: {}, {}", font_name_, (int)c, cp_data);

    return cp_data;
}

const Font::CodepointData& Font::getCodepointData(char32 c) {
    auto it = characters_.find(c);
    if (it == characters_.end()) {
        return loadCodepoint(c);
    }
    return it->second;
}

f32 Font::getGlyphKernAdvance(int a, int b) const {
    if (a && b) {
        return (static_cast<f32>(stbtt_GetGlyphKernAdvance(&font_info_, a, b)) * scale_) / height_;
    }
    return 0;
}

FontManager::FontManager() {
    MLE_I("Creating FontManager");

    renderer::TextureAtlas::GroupCreateInfo group_ci;
    atlas_ = std::make_unique<renderer::TextureAtlas>(group_ci);
}

FontRef FontManager::getFont(const std::string& name) {
    if (name.empty()) {
        return default_font_;
    }

    auto it = fonts_.find(name);
    if (it == fonts_.end()) {
        MLE_E("Font {} not found, returning default font {}", name, fonts_.begin()->first);
        return default_font_;
    }
    return fonts_.at(name).get();
}

FontRef FontManager::addFont(const sol::table& table) {
    Font::CI font_ci{};

    if (auto name = table[1]; name.valid()) {
        font_ci.font_name = name;
    } else if (auto name = table["name"]; name.valid()) {
        font_ci.font_name = name;
    } else {
        MLE_UNREACHABLE_LOG("Font name not found");
    }

    fs::path path;
    if (auto r_path = table["path"]; r_path.valid()) {
        path = addFontsBasePath(r_path.get<std::string>());
    } else {
        path = addFontsBasePath(font_ci.font_name + ".ttf");
    }

    font_ci.height = table["height"];
    font_ci.padding = lua::asVec2i(table["padding"]);
    font_ci.atlas_chars_per_dim = lua::asVec2i(table["atlas_chars_per_dim"]);
    font_ci.pre_load_string = UnicodeBuffer{table["load_string"].get<std::string>()};
    if (auto r_cw = table["cursor_width"]; r_cw.valid()) {
        font_ci.cursor_width = r_cw;
    }
    return addFont(std::move(font_ci), path);
}

FontRef FontManager::addFont(Font::CI ci, const fs::path& path) {
    MLE_ASSERT(!ci.font_name.empty());
    MLE_ASSERT_LOG(!fonts_.contains(ci.font_name), "Font {} already exists", ci.font_name);

    MLE_D("Adding font: {}, file: {}", ci.font_name, path);
    FontRef f = fonts_.emplace(ci.font_name, std::make_unique<Font>(ci, readFile(path), *atlas_)).first->second.get();
    if (fonts_.size() == 1) {
        default_font_ = f;
    }
    return f;
}

}  // namespace mle::ui
