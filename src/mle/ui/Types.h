#pragma once

#include <entt/entt.hpp>

#include "mle/common/ECS.h"
#include "mle/common/Types.h"
#include "mle/common/Utils.h"

namespace mle::ui {
class Font;
using FontHnd = std::unique_ptr<Font>;
using FontRef = Font*;

namespace detail {
class FontCache;
}  // namespace detail
}  // namespace mle::ui
