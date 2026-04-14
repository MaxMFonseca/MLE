#include "ColorCache.h"

#include "mle/core/Logger.h"
#include "sol/sol.hpp"

namespace mle {

ColorCache::ColorCache() {
    add("WHITE", Color::WHITE);
    add("BLACK", Color::BLACK);
    add("RED", Color::RED);
    add("GREEN", Color::GREEN);
    add("BLUE", Color::BLUE);
    add("MAGENTA", Color::MAGENTA);
    add("YELLOW", Color::YELLOW);
    add("CYAN", Color::CYAN);
    add("GRAY", Color::GRAY);
    add("LIGHT_GRAY", Color::LIGHT_GRAY);
    add("DARK_GRAY", Color::DARK_GRAY);
}

void ColorCache::add(const std::string& name, Color color) {
    MLE_D("New color {}: {}", name, color);
    colors_[name] = color;
}

void ColorCache::add(const sol::table& table) {
    for (const auto& kv : table) {
        if (kv.first.get_type() == sol::type::string) {
            std::string name = kv.first.as<std::string>();
            if (name.empty() || name[0] == '#') {
                MLE_W("Invalid color name '{}', skipping", name);
                continue;
            }
            Color color = Color::fromLua(kv.second);
            add(name, color);
        }
    }
}

void ColorCache::fillTable(sol::table& table) const {
    for (const auto& [name, color] : colors_) {
        table[name] = color;
    }
}

Color ColorCache::get(const std::string& name) const {
    if (name.empty()) {
        MLE_W("Empty color name, returning WHITE");
        return Color::WHITE;
    }
    if (name[0] == '#') {
        return Color::fromString(name);
    }

    auto it = colors_.find(name);
    if (it != colors_.end()) {
        return it->second;
    }
    MLE_W("Color '{}' not found in cache, returning WHITE", name);
    return Color::WHITE;
}
}  // namespace mle
