#pragma once

#include <string>
#include <unordered_map>

#include "mle/utils/Color.h"

namespace mle {
class ColorCache {
  public:
    ColorCache();  // This should add engine default colors

    void add(const std::string& name, Color color);
    void add(const sol::table& table);
    Color get(const std::string& name) const;

    void fillTable(sol::table& table) const;

  private:
    std::unordered_map<std::string, Color> colors_;
};
}  // namespace mle
