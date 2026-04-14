#include "FontCache.h"

#include "mle/core/Assert.h"
#include "mle/utils/ECS.h"

namespace mle {
void FontCache::init() {
    MLE_I("Initializing FontCache, loading default font: {}.", DEFAULT_FONT_NAME);

    Path path = ResPath::RES;
    path /= ResPath::FONTS;
    path /= DEFAULT_FONT_NAME;
    path += ".ttf";

    Font::CI ci;
    ci.ttf_path = path;
    ci.font_height = 64;
    ci.init_load_ascii = true;
    ci.atlas_scale = 16;

    default_font_ = std::unique_ptr<Font>(new Font());
    if (default_font_->init(ci) != Result::OK) {
        MLE_E("Failed to load default font: {}", path.string());
        default_font_.reset();
    }
};

void FontCache::shutdown() {
    MLE_I("Shutting down FontCache, unloading {} fonts.", fonts_.size());
    fonts_.clear();
    default_font_.reset();
};

void FontCache::add(entt::id_type font_id, const Font::CI& font_ci) {
    FontHnd font = std::unique_ptr<Font>(new Font());
    if (font->init(font_ci) != Result::OK) {
        MLE_E("Failed to load font id: {}", as<u32>(font_id));
        return;
    }

    fonts_.emplace(font_id, std::move(font));
};

void FontCache::add(const std::string& font_name) {
    Path path = ResPath::RES;
    path /= ResPath::FONTS;
    path /= font_name;
    if (!path.has_extension()) {
        path += ".ttf";
    } else {
        if (path.extension() != ".ttf") {
            MLE_E("FontCache only supports ttf fonts. Failed to add font: {}", path.string());
            return;
        }

        MLE_W("Only ttf fonts are supported. So use the cache without the extension, for consistency.");
    }

    Font::CI ci;
    ci.ttf_path = path;
    ci.font_height = 64;
    ci.init_load_ascii = true;
    ci.atlas_scale = 16;

    add(entt::hashed_string(font_name.c_str()), ci);
};

FontRef FontCache::get(entt::id_type font_id) {
    if (font_id == 0 || font_id == entt::hashed_string("default")) {
        return default_font_.get();
    }

    auto it = fonts_.find(font_id);
    if (it == fonts_.end()) {
        MLE_W("Font id '{}' not found in cache, returning default font.", as<u32>(font_id));
        return default_font_.get();
    }
    return it->second.get();
};

FontRef FontCache::get(const std::string& font_name) {
    return get(entt::hashed_string(font_name.c_str()));
};

}  // namespace mle
