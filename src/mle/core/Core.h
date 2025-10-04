#pragma once

namespace mle {
struct CoreInitInfo {
    int argc = 0;
    char** argv = nullptr;
};
void init(const CoreInitInfo& ii);
}  // namespace mle
