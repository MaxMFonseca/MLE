#pragma once

#include "../Types.h"

namespace mle::ui::comp {
struct Shader {
    bool preserve_background = false;
    bool include_border = true;
    bool include_children = true;
};
}  // namespace mle::ui::comp
