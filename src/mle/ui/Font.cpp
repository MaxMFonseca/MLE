#include "Font.h"

#include <utf8/unchecked.h>

#include "UI.h"
#include "mle/common/Types.h"
#include "mle/core/Unwrap.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"

namespace mle::ui {

void Font::init(CI ci) {
    MLE_ASSERT_LOG(!ci.name.empty() || !ci.path.empty(), "Font name or path must be provided");

    if (ci.name.empty()) {
        auto no_ext = ci.path;
        no_ext.replace_extension("");
        ci.name = res::removeBasePath(no_ext.string());
        MLE_D("Shader name not provided, using: {}", ci.name);
    }

    if (ci.path.empty()) {
        ci.path = ci.name;
        if (ci.engine) {
            ci.path = res::addMleFontPath(ci.name);
        } else {
            ci.path = res::addUserFontPath(ci.name);
        }
        ci.path += ".ttf";
    }

    MLE_I("Creating font {}, path: {}", ci.name, ci.path.string());

    name_ = std::move(ci.name);

    ttf_data_ = unwrap(mle::readFile(ci.path, true));

    auto init_font_result = stbtt_InitFont(&f_info_, rAs<u8*>(ttf_data_.data()), 0);
    MLE_ASSERT(init_font_result);

    scale_ = stbtt_ScaleForPixelHeight(&f_info_, height_);

    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&f_info_, &ascent, &descent, &line_gap);

    auto amd = as<f32>(ascent - descent);

    ascent_ = as<f32>(ascent) / amd;
    descent_ = as<f32>(descent) / amd;
    line_gap_ = as<f32>(line_gap) / amd;

    sdf_padding_ = as<int>(std::roundf(height_ / 8));

    u32 border_size = as<u32>(height_ / 16.F);
    vec2u default_char_size{height_ / 4, as<int>(as<f32>(ascent) * scale_) / 16 * 13};
    std::vector<u8> default_char_data;
    default_char_data.resize(as<usize>(default_char_size.x * default_char_size.y), 0x00);
    for (u32 i = 0; i < default_char_size.y; ++i) {
        for (u32 j = 0; j < default_char_size.x; ++j) {
            if ((i < border_size) || (i > default_char_size.y - border_size) || (j < border_size) || (j > default_char_size.x - border_size)) {
                default_char_data[(i * default_char_size.x) + j] = 0xFF;
            }
        }
    }

    auto idx_rect = addTexture(default_char_data.data(), default_char_size);
    default_char_.texture_idx = atlases_.back();
    default_char_.texture_rect.pos.x = as<f32>(idx_rect.pos.x) / as<f32>(atlas_size_.x);
    default_char_.texture_rect.pos.y = as<f32>(idx_rect.pos.y) / as<f32>(atlas_size_.y);
    default_char_.texture_rect.size.x = as<f32>(idx_rect.size.x) / as<f32>(atlas_size_.x);
    default_char_.texture_rect.size.y = as<f32>(idx_rect.size.y) / as<f32>(atlas_size_.y);
    default_char_.advance = as<f32>(default_char_size.x) * 1.2F / height_;
    default_char_.size.x = as<f32>(default_char_size.x) / height_;
    default_char_.size.y = as<f32>(default_char_size.y) / height_;
    default_char_.origin.y = ascent_ / 16 * 3 / height_;

    for (char32 i : ci.pre_load_string) {
        loadCodepoint(i);
    }
}

Font::Glyph Font::loadCodepoint(char32 codepoint) {
    auto it = chars_.find(codepoint);
    if (it != chars_.end()) {
        return it->second;
    }

    auto intc = as<int>(codepoint);
    MLE_T("Loading codepoint: {}, font: {}", intc, name_);

    auto glyph_idx = stbtt_FindGlyphIndex(&f_info_, static_cast<int>(intc));

    if (glyph_idx == 0) {
        MLE_E("codepoint: {} not found in font: {}", intc, name_);
        return default_char_;
    }

    vec2i size, origin;
    auto* sdf_result = stbtt_GetGlyphSDF(&f_info_, scale_, glyph_idx, sdf_padding_, 128, 16.0F, &size.x, &size.y, &origin.x, &origin.y);

    if (!sdf_result) {
        MLE_E("Failed to make SDF for {} in font {}", intc, name_);
        return default_char_;
    }

    auto texture_rect = addTexture(sdf_result, size);

    int advance = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&f_info_, glyph_idx, &advance, &lsb);
    advance = as<int>(std::roundf(as<f32>(advance) * scale_));
    lsb = as<int>(std::roundf(as<f32>(lsb) * scale_));

    Glyph ret;

    ret.texture_idx = atlases_.back();
    ret.texture_rect.pos.x = as<f32>(texture_rect.pos.x) / as<f32>(atlas_size_.x);
    ret.texture_rect.pos.y = as<f32>(texture_rect.pos.y) / as<f32>(atlas_size_.y);
    ret.texture_rect.size.x = as<f32>(texture_rect.size.x) / as<f32>(atlas_size_.x);
    ret.texture_rect.size.y = as<f32>(texture_rect.size.y) / as<f32>(atlas_size_.y);
    ret.size.x = as<f32>(size.x) / height_;
    ret.size.y = as<f32>(size.y) / height_;
    ret.origin.x = as<f32>(origin.x) / height_;
    ret.origin.y = as<f32>(origin.y) / height_;
    ret.advance = as<f32>(advance) / height_;
    ret.lsb = as<f32>(lsb) / height_;

    chars_.emplace(codepoint, ret);

    return ret;
}

void Font::createAtlas() {
    MLE_T("Creating new atlas for font: {}", name_);

    packer_ = {};
    packer_.padding = {2, 2};
    packer_.max_extent = atlas_size_;

    renderer::Image::CI ci;
    ci.format = vk::Format::eR8Unorm;
    ci.extent = packer_.max_extent;
    ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    auto img = renderer::Image::createHnd(ci);

    auto atlas_t = renderer::addTexture(fmt::format("{}.{}", name_, atlases_.size()), std::move(img));

    atlases_.emplace_back(atlas_t.idx);

    atlas_full_ = false;
}

Rectu Font::addTexture(const void* data, vec2u size) {  // NOLINT
    if (atlas_full_) {
        createAtlas();
    }

    auto& packer_rect = packer_.rects_.emplace_back();
    packer_rect.size = size;
    if (packer_.pack()) {
        auto rect = packer_.get();

        renderer::enqueueTextureUpdateJob({.buffer = renderer::Image::createStagingBuffer(data, size, 1), .area = rect, .idx = atlases_.back()});

        return rect;
    }

    atlas_full_ = true;
    return addTexture(data, size);
}

Font::Glyph Font::getGlyph(char32 codepoint) {
    auto it = chars_.find(codepoint);
    if (it == chars_.end()) {
        MLE_T("Glyph for codepoint {} not found in font {}, loading...", as<int>(codepoint), name_);
        auto glyph = loadCodepoint(codepoint);
        return glyph;
    }
    return it->second;
}

Font::RenderText Font::makeText(const std::string& text) {
    RenderText ret;
    f32 current_x = 0;

    if (!utf8::is_valid(text)) {
        MLE_W("Text is not valid UTF-8: {}", text);
        return ret;
    }

    std::u32string text32;
    utf8::unchecked::utf8to32(text.begin(), text.end(), std::back_inserter(text32));

    [[maybe_unused]] char32 last_codepoint = 0;

    for (char32 c : text32) {
        MLE_VC((int)c);
        RenderText::Token& token = ret.tokens.emplace_back();

        if (c == U' ') {
            token.type = RenderText::Token::Type::SPACE;
            token.rect.pos.x = current_x;
            token.rect.size.x = default_char_.advance;
            current_x += default_char_.advance;
            last_codepoint = c;
            continue;
        }

        if (c == U'\n') {
            token.type = RenderText::Token::Type::BR;
            last_codepoint = c;
            continue;
        }

        auto glyph = getGlyph(c);
        token.type = RenderText::Token::Type::CHAR;
        token.texture_idx = glyph.texture_idx;
        token.texture_rect = glyph.texture_rect;

        if (!last_codepoint) {
            current_x = -glyph.origin.x;
        }
        if (last_codepoint && last_codepoint != U'\n') {
            int kern = stbtt_GetCodepointKernAdvance(&f_info_, static_cast<int>(last_codepoint), static_cast<int>(c));
            current_x += std::roundf(as<f32>(kern) * scale_) / height_;
        }

        token.rect.pos.x = current_x + glyph.origin.x;
        token.rect.pos.y = ascent_ + glyph.origin.y;
        token.rect.size.x = glyph.size.x;
        token.rect.size.y = glyph.size.y;

        current_x += glyph.advance;
        last_codepoint = c;
        ret.char_count++;
    }

    ret.width = current_x;
    return ret;
}

}  // namespace mle::ui
