#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <functional>

#include "mle/math/Types.h"
#include "mle/utils/ECS.h"

namespace mle::audio {
struct PlayParams {
    vec3f position{};
    f32 volume = 1.0F;
    vec3f velocity{};
    f32 pitch = 1.0F;
    f32 start_offset_s = 0.0F;
    u32 fade_in_ms = 0;
    bool spatial = false;
    u8 bus = 0;
};

namespace cmd {
struct Load {
    std::string name;
    bool stream = false;
    bool loop = false;
};

struct PlayOneShot {
    entt::id_type sound_id{};
    u32 priority = 1;
    PlayParams params{};
};

struct StartStream {
    entt::id_type sound_id{};
    PlayParams params{};
    bool loop = false;
    u8 id = 0;
};

struct StopStream {
    u8 id = 0;
};

struct PauseStream {
    u8 id = 0;
};

struct ResumeStream {
    u8 id = 0;
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

using Cmd = std::variant<cmd::Load, cmd::PlayOneShot, cmd::StartStream, cmd::StopStream, cmd::PauseStream, cmd::ResumeStream, cmd::SetVolume, cmd::SetListener,
                         cmd::SetDistanceParams, cmd::StopAll>;
}  // namespace mle::audio
