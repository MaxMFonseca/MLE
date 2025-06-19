#include "FontCache.h"

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/core/Unwrap.h"

namespace mle::ui::detail {
void FontCache::init() {
    MLE_I("Initializing font cache");
}

void FontCache::reset() {
    MLE_I("Resetting font cache");
    fonts_.clear();
}

FontRef FontCache::add(Font::CI&& ci) {
    FontHnd font{new Font()};
    font->init(std::move(ci));

    MLE_ASSERT(fonts_.end() == std::ranges::find_if(fonts_, [&font](const FontHnd& f) { return f->name_ == font->name_; }))

    FontRef ret = font.get();
    fonts_.emplace_back(std::move(font));
    return ret;
}

FontRef FontCache::get(const std::string& name) {
    for (const auto& font : fonts_) {
        if (font->name_ == name) {
            return font.get();
        }
    }
    MLE_E("Font with name '{}' not found. Retuning default.", name);
    MLE_ASSERT_LOG(!fonts_.empty(), "Font cache is empty!");
    return fonts_.front().get();  // This should always be valid
}
}  // namespace mle::ui::detail
