#pragma once

#include <variant>

#include "entt/entt.hpp"
#include "mle/common/EventDispatcher.h"
#include "mle/renderer/Types.h"

namespace mle::game {
namespace secs {
struct Transform {
    vec3f pos{0, 0, 0};
    vec3f scale{1, 1, 1};
    quat rotation{1, 0, 0, 0};
};

struct Model {
    std::string model_string;  /// The idea here is that this can be anything, ie, "Player;hair:3;skin:1;shirt:2;pants:4;shoes:5"
};

struct Animation {};
struct Sound {};
struct Light {};
}  // namespace secs

namespace out_ev {
struct Time {
    f32 time_s = 0;
};

struct EnttCreated {
    entt::entity e{};
};

struct EnttDestroyed {
    entt::entity e{};
};

struct EnttTransform {
    vec3f pos{0, 0, 0};
    vec3f scale{1, 1, 1};
    quat rotation{1, 0, 0, 0};
    entt::entity e{};
};

struct EnttModel {
    entt::entity e{};
    std::string model_string;
};

using Variant = std::variant<Time, EnttCreated, EnttDestroyed, EnttTransform, EnttModel>;
}  // namespace out_ev

struct ServerOutPackage {
    f32 time_s;
    std::vector<out_ev::Variant> events;
};

using ServerOutED = EDFromVariant<out_ev::Variant>::Type;

using ServerTimeListener = ServerOutED::ListenerHnd<out_ev::Time>;
using ServerEnttCreatedListener = ServerOutED::ListenerHnd<out_ev::EnttCreated>;
using ServerEnttDestroyedListener = ServerOutED::ListenerHnd<out_ev::EnttDestroyed>;
using ServerEnttTransformListener = ServerOutED::ListenerHnd<out_ev::EnttTransform>;
using ServerEnttModelListener = ServerOutED::ListenerHnd<out_ev::EnttModel>;
}  // namespace mle::game
