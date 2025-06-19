#include "Font.h"

#include "UI.h"
#include "mle/core/Unwrap.h"

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

    // int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    // stbtt_GetFontBoundingBox(&f_info_, &x0, &y0, &x1, &y1);
    // vec2f max_char_size = {as<f32>(x1 - x0) * scale_, as<f32>(y1 - y0) * scale_};

    ascent_ = as<int>((as<f32>(ascent) * scale_));
    descent_ = as<int>((as<f32>(descent) * scale_));
    line_gap_ = as<int>((as<f32>(line_gap) * scale_));

    sdf_padding_ = as<int>(std::roundf(fheight / 8));

    // for (usize i = 0; i < ci.pre_load_string.size(); i++) {
    //     if (!loadCodepoint(ci.pre_load_string.at(i)).glyph_idx) {
    //         MLE_E("Failed to load codepoint of preload string: {}", (int)ci.pre_load_string.get(i));
    //     };
    // }
}
}  // namespace mle::ui
