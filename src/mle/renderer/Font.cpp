#include "Font.h"

#include "mle/core/Result.h"
#include "mle/utils/Justify.h"
#include "mle/utils/String.h"

namespace mle {
Result Font::init(const CI& ci) {
    MLE_I("Loading ttf path: {}", ci.ttf_path.string());

    auto ttf_data_r = mle::readFileBytes(ci.ttf_path);
    if (!ttf_data_r) {
        MLE_E("Failed to read ttf file: {}", ci.ttf_path.string());
        return ttf_data_r.error();
    }
    ttf_data_ = std::move(ttf_data_r.value());

    auto init_font_result = stbtt_InitFont(&fontinfo_, rAs<u8*>(ttf_data_.data()), 0);

    if (!init_font_result) {
        MLE_E("Failed to init font from ttf data: {}", ci.ttf_path.string());
        return Result::FAILED_TO_OPEN;
    }

    height_i_ = ci.font_height;
    height_ = as<f32>(height_i_);
    scale_ = stbtt_ScaleForPixelHeight(&fontinfo_, height_);

    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&fontinfo_, &ascent, &descent, &line_gap);

    auto amd = as<f32>(ascent - descent);

    ascent_ = as<f32>(ascent) / amd;
    descent_ = as<f32>(descent) / amd;
    line_gap_ = as<f32>(line_gap) / amd;

    sdf_padding_ = as<int>(std::roundf(height_ / 8));

    int space_advance_width = 0, space_lsb = 0;
    stbtt_GetCodepointHMetrics(&fontinfo_, ' ', &space_advance_width, &space_lsb);
    space_advance_width = as<int>(std::roundf(as<f32>(space_advance_width) * scale_));
    space_lsb = as<int>(std::roundf(as<f32>(space_lsb) * scale_));

    if (space_advance_width <= 0) {
        space_advance_width = as<int>(std::roundf(height_ / 4));
    }

    u32 border_size = as<u32>(height_ / 16.F);
    vec2u default_char_size{space_advance_width * 0.8, as<int>(as<f32>(ascent) * scale_) / 16 * 13};
    std::vector<u8> default_char_data;
    default_char_data.resize(as<usize>(default_char_size.x * default_char_size.y), 0x00);
    for (u32 i = 0; i < default_char_size.y; ++i) {
        for (u32 j = 0; j < default_char_size.x; ++j) {
            if ((i < border_size) || (i > default_char_size.y - border_size) || (j < border_size) || (j > default_char_size.x - border_size)) {
                default_char_data[(i * default_char_size.x) + j] = 0xFF;
            }
        }
    }

    TextureAtlas::CI texture_atlas_ci{};
    texture_atlas_ci.extent = vec2u{ci.atlas_scale * ci.font_height, ci.atlas_scale * ci.font_height};
    texture_atlas_ci.format = Image::Format::TEXTURE_1U;
    atlas_.init(texture_atlas_ci);

    addTexture(INVALID_CODEPOINT, default_char_data.data(), default_char_size);
    default_char_.codepoint = INVALID_CODEPOINT;
    default_char_.advance = as<f32>(space_advance_width) / height_;
    default_char_.size.x = as<f32>(default_char_size.x) / height_;
    default_char_.size.y = as<f32>(default_char_size.y) / height_;
    default_char_.origin.y = ascent_ / 16 * 3 / height_;

    if (ci.init_load_ascii) {
        loadString(U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:'\",.<>?/\\`~");
    }
    return Result::OK;
}

Expected<Font::Glyph> Font::loadCodepoint(char32 codepoint) {
    if (const auto it = cps_.find(codepoint); it != cps_.end()) {
        return it->second;
    }

    auto intc = as<int>(codepoint);
    MLE_T("Loading codepoint: {}", intc);

    auto glyph_idx = stbtt_FindGlyphIndex(&fontinfo_, static_cast<int>(intc));

    if (glyph_idx == 0) {
        MLE_E("codepoint: {} not found in font", intc);
        return std::unexpected(Result::NOT_FOUND);
    }

    vec2i size, origin;
    auto* sdf_result = stbtt_GetGlyphSDF(&fontinfo_, scale_, glyph_idx, sdf_padding_, 128, 16.0F, &size.x, &size.y, &origin.x, &origin.y);

    if (!sdf_result) {
        MLE_E("Failed to make SDF for cp {}", intc);
        return std::unexpected(Result::FAILED_TO_CREATE);
    }

    addTexture(codepoint, sdf_result, size);

    stbtt_FreeSDF(sdf_result, nullptr);

    int advance = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&fontinfo_, glyph_idx, &advance, &lsb);
    advance = as<int>(std::roundf(as<f32>(advance) * scale_));
    lsb = as<int>(std::roundf(as<f32>(lsb) * scale_));

    Glyph ret;
    ret.codepoint = codepoint;
    ret.size.x = as<f32>(size.x) / height_;
    ret.size.y = as<f32>(size.y) / height_;
    ret.origin.x = as<f32>(origin.x) / height_;
    ret.origin.y = as<f32>(origin.y) / height_;
    ret.advance = as<f32>(advance) / height_;
    ret.lsb = as<f32>(lsb) / height_;

    auto emplace_r = cps_.emplace(codepoint, ret);

    if (!emplace_r.second) {
        MLE_E("Failed to emplace glyph for cp {}", intc);
        return std::unexpected(Result::FAILED_TO_CREATE);
    }

    return ret;
}

void Font::loadString(const std::u32string& str) {
    for (char32 c : str) {
        std::ignore = loadCodepoint(c);
    }
}

Font::Glyph Font::getGlyph(char32 codepoint) {
    auto it = cps_.find(codepoint);
    if (it == cps_.end()) {
        MLE_T("Glyph for codepoint {} not found, loading...", as<int>(codepoint));
        auto load_result = loadCodepoint(codepoint);
        if (!load_result) {
            MLE_E("Failed to load glyph for codepoint {}: {}", as<int>(codepoint), load_result.error());
            return default_char_;
        }
        return load_result.value();
    }
    return it->second;
}

void Font::addTexture(char32 codepoint, const void* data, vec2u size) {
    Image::RawData raw_data;
    raw_data.extent = size;
    raw_data.channels = 1;
    raw_data.pixels.resize(as<usize>(size.x * size.y));
    std::memcpy(raw_data.pixels.data(), data, raw_data.pixels.size());

    std::scoped_lock lock(atlas_mutex_);
    atlas_.enqueueCopy(codepoint, raw_data);
    atlas_.requestFlushOnFrame();
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) I know
Font::RenderText Font::makeText(const MakeTextIn& in) {
    struct Word {
        bool space = false;
        usize start_char_idx = 0;
        usize end_char_idx = 0;
        usize word_idx = 0;
    };
    std::vector<std::vector<Word>> text_lines;
    std::vector<f32> word_widths;
    std::vector<f32> char_word_positions;
    char_word_positions.resize(in.str.size(), 0.0F);

    MLE_VC(in.justify_mode);

    text_lines.emplace_back();
    for (usize i = 0; i < in.str.size(); ++i) {
        auto& text_line = text_lines.back();
        if (in.str[i] == U'\n') {
            text_lines.emplace_back();
            continue;
        }
        if (in.str[i] == U' ') {
            Word& word = text_line.emplace_back();
            word.space = true;
            word.start_char_idx = i;
            usize word_len = 0;
            while (i < in.str.size() && in.str[i] == U' ') {
                char_word_positions[i] = as<f32>(word_len) * default_char_.advance;
                word_len++;
                ++i;
            }
            word.end_char_idx = i;
            --i;

            word.word_idx = word_widths.size();
            word_widths.emplace_back(as<f32>(word_len) * default_char_.advance);
            continue;
        }

        Word& word = text_line.emplace_back();
        word.start_char_idx = i;
        while (i < in.str.size() && in.str[i] != U' ' && in.str[i] != U'\n') {
            ++i;
        }
        word.end_char_idx = i;
        --i;

        f32 word_width = 0;
        char32 last_cp = 0;
        for (usize j = word.start_char_idx; j < word.end_char_idx; ++j) {
            auto glyph = getGlyph(in.str[j]);
            if (last_cp) {
                int kern = stbtt_GetCodepointKernAdvance(&fontinfo_, static_cast<int>(last_cp), static_cast<int>(in.str[j]));
                word_width += std::roundf(as<f32>(kern) * scale_) / height_;
            }
            char_word_positions[j] = word_width;
            word_width += glyph.advance;
            last_cp = in.str[j];
        }
        word.word_idx = word_widths.size();
        word_widths.emplace_back(word_width);
    }

    auto justify_mode = JustifyF32::LineMode(in.justify_mode);
    auto justify_mode_last_line = JustifyF32::LineMode(in.justify_mode == JustifyMode::SPACE_BETWEEN ? JustifyMode::START : in.justify_mode);
    f32 text_width = 0.0F;

    JustifyF32::Lines justified_lines;
    bool justify = false;
    if (in.wrap && justify_mode != JustifyF32::LineMode::SPACE_BETWEEN) {
        isize current_word_idx = 0;
        for (const auto& text_line : text_lines) {
            if (text_line.empty()) {
                justified_lines.emplace_back();
                continue;
            }

            std::span<f32> word_sizes(word_widths.begin() + current_word_idx, text_line.size());
            auto new_lines = JustifyF32::wrap(word_sizes, 0, justify_mode, justify_mode_last_line, in.line_max_aspect);
            justified_lines.insert(justified_lines.end(), new_lines.begin(), new_lines.end());
            current_word_idx += as<isize>(text_line.size());
        }
        text_width = in.line_max_aspect;
    } else if (in.wrap) {
        justify = true;
        isize current_word_idx = 0;
        for (const auto& text_line : text_lines) {
            if (text_line.empty()) {
                justified_lines.emplace_back();
                continue;
            }

            std::vector<f32> word_sizes;
            usize line_word_idx = 0;
            for (const auto& word : text_line) {
                if (!word.space) {
                    word_sizes.push_back(word_widths[as<usize>(current_word_idx) + line_word_idx]);
                }
            }

            auto new_lines = JustifyF32::wrap(std::span<f32>(word_sizes), default_char_.advance, justify_mode, justify_mode_last_line, in.line_max_aspect);
            justified_lines.insert(justified_lines.end(), new_lines.begin(), new_lines.end());
            current_word_idx += as<isize>(text_line.size());
        }
        text_width = in.line_max_aspect;
    } else {
        isize current_word_idx = 0;
        std::vector<f32> line_widths;
        for (int i = 0; i < as<int>(text_lines.size()); ++i) {
            const auto& text_line = text_lines[i];
            if (text_line.empty()) {
                justified_lines.emplace_back();
                continue;
            }
            std::span<f32> word_sizes(word_widths.begin() + current_word_idx, text_line.size());
            auto j_r = JustifyF32::noWrap(word_sizes, 0);
            justified_lines.emplace_back(j_r.first);
            line_widths.push_back(j_r.second);
            current_word_idx += as<isize>(text_line.size());
            text_width = glm::max(text_width, j_r.second);
        }
        switch (in.justify_mode) {
            case JustifyMode::START: {
                MLE_NOOP;
            } break;
            case JustifyMode::CENTER: {
                for (usize i = 0; i < justified_lines.size(); ++i) {
                    f32 offset = (text_width - line_widths[i]) / 2.0F;
                    for (auto& pos : justified_lines[i]) {
                        pos += offset;
                    }
                }
            } break;
            case JustifyMode::END: {
                for (usize i = 0; i < justified_lines.size(); ++i) {
                    f32 offset = text_width - line_widths[i];
                    for (auto& pos : justified_lines[i]) {
                        pos += offset;
                    }
                }
            } break;
            case JustifyMode::SPACE_BETWEEN: {
                MLE_W("Cannot use SPACE_BETWEEN justify mode without wrapping enabled.");
            } break;
        }
    }

    RenderText ret;

    ret.text_extent.x = text_width;
    f32 line_count_f = as<f32>(justified_lines.size());
    ret.text_extent.y = line_count_f + ((line_count_f - 1) * line_gap_);

    if (!justify) {
        usize current_word_on_justified_line_idx = 0;
        usize current_justified_line_idx = 0;
        auto current_justified_line_it = justified_lines.begin();
        for (const auto& text_line : text_lines) {
            for (const auto& text_word : text_line) {
                if (current_word_on_justified_line_idx >= current_justified_line_it->size()) {
                    current_justified_line_it++;
                    current_justified_line_idx++;
                    current_word_on_justified_line_idx = 0;
                }

                auto word_offset = current_justified_line_it->at(current_word_on_justified_line_idx);

                for (usize j = text_word.start_char_idx; j < text_word.end_char_idx; ++j) {
                    Font::RenderText::Char chr;
                    chr.codepoint = in.str[j];
                    chr.idx = j;
                    auto glyph = in.str[j] == U' ' ? default_char_ : getGlyph(in.str[j]);
                    chr.rect.setPosX(word_offset + char_word_positions[j] + glyph.origin.x);
                    chr.rect.setPosY(ascent_ + glyph.origin.y + (as<f32>(current_justified_line_idx) + line_gap_));
                    chr.rect.setSizeX(glyph.size.x);
                    chr.rect.setSizeY(glyph.size.y);
                    ret.chars.push_back(chr);
                }

                current_word_on_justified_line_idx++;
            }
        }
    } else {
        usize current_word_on_justified_line_idx = 0;
        usize current_justified_line_idx = 0;
        auto current_justified_line_it = justified_lines.begin();
        f32 current_line_space_size = 0.0F;
        for (const auto& text_line : text_lines) {
            for (const auto& text_word : text_line) {
                if (current_word_on_justified_line_idx >= current_justified_line_it->size()) {
                    current_justified_line_it++;
                    current_justified_line_idx++;

                    if (current_justified_line_idx == justified_lines.size() - 1) {
                        current_line_space_size = default_char_.advance;
                    } else {
                        current_line_space_size = in.line_max_aspect;
                        for (const auto& jw : *current_justified_line_it) {
                            current_line_space_size -= jw;
                        }
                        f32 space_count = as<f32>(current_justified_line_it->size()) - 1;
                        current_line_space_size /= space_count;
                    }
                }

                if (text_word.space) {
                    if (current_word_on_justified_line_idx != 0) {
                        auto word_offset = current_justified_line_it->at(current_word_on_justified_line_idx) - word_widths[text_word.word_idx - 1];
                        for (usize j = text_word.start_char_idx; j < text_word.end_char_idx; ++j) {
                            Font::RenderText::Char chr;
                            chr.codepoint = ' ';
                            chr.idx = j;
                            auto glyph = default_char_;
                            chr.rect.setPosX(word_offset + (as<f32>(j) * current_line_space_size));
                            chr.rect.setPosY(ascent_ + glyph.origin.y + (as<f32>(current_justified_line_idx) + line_gap_));
                            chr.rect.setSizeX(current_line_space_size);
                            chr.rect.setSizeY(glyph.size.y);
                            ret.chars.push_back(chr);
                        }
                    }
                } else {
                    auto word_offset = current_justified_line_it->at(current_word_on_justified_line_idx);
                    for (usize j = text_word.start_char_idx; j < text_word.end_char_idx; ++j) {
                        Font::RenderText::Char chr;
                        chr.codepoint = in.str[j];
                        chr.idx = j;
                        auto glyph = getGlyph(in.str[j]);
                        chr.rect.setPosX(word_offset + char_word_positions[j] + glyph.origin.x);
                        chr.rect.setPosY(ascent_ + glyph.origin.y + (as<f32>(current_justified_line_idx) + line_gap_));
                        chr.rect.setSizeX(glyph.size.x);
                        chr.rect.setSizeY(glyph.size.y);
                        ret.chars.push_back(chr);
                    }
                    current_word_on_justified_line_idx++;
                }
            }
        }
    }

    ret.line_count = as<u32>(justified_lines.size());

    return ret;
}

std::pair<ImageRef, TextureAtlas::Entry> Font::getTextureEntry(char32 codepoint) {
    std::scoped_lock lock(atlas_mutex_);
    auto atlas_r = atlas_.get(codepoint);
    if (!atlas_r) {
        return atlas_.get(INVALID_CODEPOINT).value();
    }
    return atlas_r.value();
};
}  // namespace mle
