#pragma once

#include <variant>

#include "entt/entt.hpp"
#include "mle/common/EventDispatcher.h"
#include "mle/renderer/Types.h"

namespace mle::game {
namespace server_comp {
struct Transform {
    vec3f pos{0, 0, 0};
    vec3f scale{1, 1, 1};
    f32 rotation{0};
};

struct Model {
    std::string model_string;  /// The idea here is that this can be anything, ie, "Player;hair:3;skin:1;shirt:2;pants:4;shoes:5"
};
struct Animation {};
struct Sound {};
struct Light {};
}  // namespace server_comp

namespace server_out_events {
struct Time {
    f32 time_s = 0;
};

struct NewEntt {
    entt::entity e{};
};

struct EnttPosition {
    entt::entity e{};
    vec3f pos{0, 0, 0};
};

struct EnttRotation {
    entt::entity e{};
    f32 v = 0;
};

struct EnttModel {
    entt::entity e{};
    std::string model_string;
};

using Variant = std::variant<Time, EnttPosition, EnttRotation, NewEntt, EnttModel>;
}  // namespace server_out_events

struct ServerOutPackage {
    f32 time_s;
    std::vector<server_out_events::Variant> events;
};

using ServerOutED = EDFromVariant<server_out_events::Variant>::Type;

using ServerTimeListener = ServerOutED::ListenerHnd<server_out_events::Time>;
using ServerNewEnttListener = ServerOutED::ListenerHnd<server_out_events::NewEntt>;
using ServerEnttModelListener = ServerOutED::ListenerHnd<server_out_events::EnttModel>;
using ServerEnttPositionListener = ServerOutED::ListenerHnd<server_out_events::EnttPosition>;
using ServerEnttRotationListener = ServerOutED::ListenerHnd<server_out_events::EnttRotation>;
}  // namespace mle::game
