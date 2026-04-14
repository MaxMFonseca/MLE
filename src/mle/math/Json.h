#pragma once

#include <json.hpp>

#include "mle/math/Types.h"

namespace nlohmann {
// NOLINTBEGIN(readability-identifier-naming) not my naming convention
template <>
struct adl_serializer<mle::vec3f> {
    static void from_json(const json& j, mle::vec3f& v) {
        v.x = j.at(0).get<mle::f32>();
        v.y = j.at(1).get<mle::f32>();
        v.z = j.at(2).get<mle::f32>();
    }
    static void to_json(json& j, const mle::vec3f& v) { j = json::array({v.x, v.y, v.z}); }
};

template <>
struct adl_serializer<mle::quat> {
    static void from_json(const json& j, mle::quat& q) {
        q.w = j.at(0).get<mle::f32>();
        q.x = j.at(1).get<mle::f32>();
        q.y = j.at(2).get<mle::f32>();
        q.z = j.at(3).get<mle::f32>();
    }
    static void to_json(json& j, const mle::quat& q) { j = json::array({q.w, q.x, q.y, q.z}); }
};

template <>
struct adl_serializer<mle::vec2u> {
    static void from_json(const json& j, mle::vec2u& v) {
        v.x = j.at(0).get<mle::u32>();
        v.y = j.at(1).get<mle::u32>();
    }
    static void to_json(json& j, const mle::vec2u& v) { j = json::array({v.x, v.y}); }
};

template <>
struct adl_serializer<mle::vec2i> {
    static void from_json(const json& j, mle::vec2i& v) {
        v.x = j.at(0).get<mle::i32>();
        v.y = j.at(1).get<mle::i32>();
    }
    static void to_json(json& j, const mle::vec2i& v) { j = json::array({v.x, v.y}); }
};

// NOLINTEND(readability-identifier-naming)
}  // namespace nlohmann
