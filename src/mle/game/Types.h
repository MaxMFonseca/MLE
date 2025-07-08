#pragma once

#include <variant>

#include "entt/entt.hpp"
#include "mle/common/EventDispatcher.h"
#include "mle/renderer/Types.h"

namespace mle::game {
namespace server_out_events {
struct Time {
    f32 time_s = 0;
};

struct NewEntt {
    entt::entity e;
};

struct EnttTransform {
    entt::entity e;
    vec3f pos{0, 0, 0};
    vec3f scale{1, 1, 1};
    quat rot{1, 0, 0, 0};
};

struct EnttModel {
    entt::entity e;
    renderer::ModelRef v;
};

using Variant = std::variant<Time, EnttTransform, NewEntt, EnttModel>;
}  // namespace server_out_events

struct ServerOutData {
    // TODO:  add owner
    f32 time_s;
    std::vector<server_out_events::Variant> events;
};

using ServerOutED = EDFromVariant<server_out_events::Variant>::Type;

using ServerTimeListener = ServerOutED::ListenerHnd<server_out_events::Time>;
using ServerNewEnttListener = ServerOutED::ListenerHnd<server_out_events::NewEntt>;
using ServerEnttTransformListener = ServerOutED::ListenerHnd<server_out_events::EnttTransform>;
using ServerEnttModelListener = ServerOutED::ListenerHnd<server_out_events::EnttModel>;
}  // namespace mle::game
