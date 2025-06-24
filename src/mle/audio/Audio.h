#pragma once

#include <variant>

#include "mle/common/math/Types.h"
namespace mle::audio {
struct PlaySound {
    std::string name;
    u8 type = 0;
};
struct PlayMusic {
    std::string name;
};
struct StopMusic {};
struct ResumeMusic {};
struct StopAll {};
struct Mute {};
struct Volume {
    u8 type = 0;  // MASTER
    u8 val = 0;
};

using Command = std::variant<PlaySound, PlayMusic, StopMusic, ResumeMusic, StopAll, Mute, Volume>;

void init();
void shutdown();
void update();
void enqueueCommand(Command cmd);
}  // namespace mle::audio
