#include "Font.h"

#include "UI.h"
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

    f32 fheight = as<f32>(height_);

    scale_ = stbtt_ScaleForPixelHeight(&f_info_, fheight);

    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&f_info_, &ascent, &descent, &line_gap);

    ascent_ = as<int>((as<f32>(ascent) * scale_));
    descent_ = as<int>((as<f32>(descent) * scale_));
    line_gap_ = as<int>((as<f32>(line_gap) * scale_));

    sdf_padding_ = as<int>(std::roundf(fheight / 8));

    u32 border_size = height_ / 16;
    vec2u default_char_size{height_ / 4, ascent_ / 16 * 13};
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
    default_char_.advance = as<int>(as<f32>(default_char_size.x) * 1.2F);

    for (char32 i : ci.pre_load_string) {
        loadCodepoint(i);
    }
}

Font::Glyph Font::loadCodepoint(char32 codepoint) {
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

    Glyph ret;

    ret.texture_idx = atlases_.back();
    ret.texture_rect.pos.x = as<f32>(texture_rect.pos.x) / as<f32>(atlas_size_.x);
    ret.texture_rect.pos.y = as<f32>(texture_rect.pos.y) / as<f32>(atlas_size_.y);
    ret.texture_rect.size.x = as<f32>(texture_rect.size.x) / as<f32>(atlas_size_.x);
    ret.texture_rect.size.y = as<f32>(texture_rect.size.y) / as<f32>(atlas_size_.y);
    ret.size = size;
    ret.origin = origin;
    ret.advance = advance;
    ret.lsb = lsb;

    return ret;
}

//     auto& cp_data = characters_[c];
//     cp_data.glyph_idx = glyph_idx;
//     cp_data.advance = (static_cast<f32>(advance) * scale_) / height_;
//     cp_data.lsb = (static_cast<f32>(lsb) * scale_) / height_;
//
//     if (sdf_result != nullptr) {
//         cp_data.size = vec2f(size) / height_;
//         cp_data.origin = vec2f(origin) / height_;
//
//         renderer::Image::RawData char_data;
//         char_data.channels = 1;
//         char_data.extent = size;
//         auto data_size = static_cast<usize>(size.x) * size.y;
//         char_data.pixels.resize(data_size);
//         memcpy(char_data.pixels.data(), sdf_result, data_size);
//         stbtt_FreeSDF(sdf_result, nullptr);
//         cp_data.texture = atlas_.addTexture(std::to_string(c), char_data, font_name_);
//     }
//
//     MLE_T("font: {}. Added codepoint: {}, {}", font_name_, (int)c, cp_data);
//
//     return cp_data;
// }

void Font::createAtlas() {
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

//
// u32 texture_idx;
// Rectf texture_rect;
//
// int height_ = 0;
// int advance = 0;
// int lsb = 0;
// default_char_.texture_idx = atlas_t.idx;
//
// default_char_.texture = atlas_.addTexture("default", default_char_data, font_name_);
// default_char_.size = vec2f(default_char_data.extent) / height_;
// default_char_.origin = {-default_char_.size.x * 0.1, -default_char_.size.y};
// default_char_.advance = default_char_.size.x * 1.2F;
// }

// const Font::CodepointData& Font::getCodepointData(char32 c) {
//     auto it = characters_.find(c);
//     if (it == characters_.end()) {
//         return loadCodepoint(c);
//     }
//     return it->second;
// }
//
// f32 Font::getGlyphKernAdvance(int a, int b) const {
//     if (a && b) {
//         return (static_cast<f32>(stbtt_GetGlyphKernAdvance(&font_info_, a, b)) * scale_) / height_;
//     }
//     return 0;
// }

}  // namespace mle::ui
