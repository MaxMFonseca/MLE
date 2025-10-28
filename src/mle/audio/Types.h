#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <functional>

#include "mle/math/Types.h"
#include "mle/utils/ECS.h"

namespace mle::audio {
namespace cmd {
struct Load {
    std::string name;
    bool stream = false;
};

struct Play {
    entt::id_type sound_id;
    vec3f position{};
    f32 volume = 1.0F;
    vec3f velocity{};
    f32 pitch = 1.0F;
    f32 start_offset_s = 0.0F;
    u32 fade_in_ms = 0;
    u32 priority = 1;
    bool loop = false;
    bool spatial = false;
    bool stream = false;
};

struct Stop {
    ID id;
};
struct Pause {
    ID id;
};
struct Resume {
    ID id;
};

struct SetVolume {
    f32 volume = 1.0F;
    u8 bus = max<u8>();
};

struct SetListener {
    vec3f position{};
    vec3f velocity{};
    vec3f forward{};
    vec3f up{};
};

struct SetDistanceParams {
    f32 reference_distance = 1.0F;
    f32 rolloff_factor = 1.0F;
    f32 max_distance = 50.0F;
};

struct StopAll {
    u32 fade_out_ms = 0;
};
}  // namespace cmd

using Cmd = std::variant<cmd::Load, cmd::Play, cmd::Stop, cmd::Pause, cmd::Resume, cmd::SetVolume, cmd::SetListener, cmd::SetDistanceParams, cmd::StopAll>;
}  // namespace mle::audio
