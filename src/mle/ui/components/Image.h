#pragma once

#include "mle/renderer/Types.h"

namespace mle::ui::comp {
struct RootImage {
    ImageHnd image;
};

struct BlitExternalImage {
    ImageRef image;
};
}  // namespace mle::ui::comp
