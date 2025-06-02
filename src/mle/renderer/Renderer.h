#pragma once

#include "Events.h"
#include "Types.h"
#include "mle/common/Result.h"

namespace mle::renderer {
struct CreateInfo {};
using CI = CreateInfo;

Result init(const CI& ci);
void shutdown();

}  // namespace mle::renderer
