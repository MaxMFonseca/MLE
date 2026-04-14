#pragma once

#include "mle/math/Types2D.h"
#include "mle/renderer/Font.h"
#include "mle/utils/ECS.h"
#include "mle/utils/Justify.h"
#include "mle/utils/Utils.h"

namespace mle {
class FontCache {
  public:
    struct Text {
        struct Line {
            struct Char {
                u32 codepoint{};
                Recti rect{};
            };
            std::vector<Char> chars{};
        };
        std::vector<Line> lines{};
        int line_height = 0;
    };

  public:
    MLE_NO_COPY_MOVE(FontCache);
    FontCache() = default;
    ~FontCache() = default;

    void init();
    void shutdown();

    void add(entt::id_type font_id, const Font::CI& font_ci);
    void add(const std::string& font_name);
    FontRef get(const std::string& font_name);
    FontRef get(entt::id_type font_id);

  private:
    std::map<entt::id_type, FontHnd> fonts_;
    FontHnd default_font_;

    static constexpr const char* DEFAULT_FONT_NAME = "mle/JetBrainsMonoNerdFont-Regular";
};

}  // namespace mle
