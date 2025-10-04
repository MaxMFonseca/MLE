#pragma once

#include <stb_truetype.h>

#include "mle/common/RectPacker.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types2D.h"
#include "mle/ui/Font.h"
#include "mle/ui/Types.h"

namespace mle::ui::detail {
constexpr int DEFAULT_FONT_HEIGHT = 64;

class FontCache {
  public:
    MLE_NO_COPY_MOVE(FontCache)

    FontCache() = default;
    ~FontCache() = default;

    void init();
    void reset();

    void update();

    FontRef add(Font::CI&& ci);

    FontRef get(const std::string& name);

  private:
    std::vector<FontHnd> fonts_;  // Since we will probably have only a few fonts, we can use a vector here.
};
}  // namespace mle::ui::detail
