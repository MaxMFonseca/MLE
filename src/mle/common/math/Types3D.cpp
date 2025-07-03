#include "Types3D.h"

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"

namespace mle {
std::optional<NearFar> Ray3f::intersect(const Boxf& box) const {
    f32 near = -inf<f32>();
    f32 far = inf<f32>();

    for (i32 i = 0; i < 3; ++i) {
        f32 t1 = (box.pos[i] - origin_[i]) * inv_direction_[i];
        f32 t2 = (box.pos[i] + box.size[i] - origin_[i]) * inv_direction_[i];

        if (t1 > t2) {
            std::swap(t1, t2);
        }

        near = std::max(near, t1);
        far = std::min(far, t2);

        if (near > far || far < 0) {
            return std::nullopt;
        }
    }

    return NearFar{.near = near < 0 ? 0 : near, .far = far};
}

BoxFace pointFace(const Boxf& box, vec3f point, f32 epsilon) {
    if (feq(point.x, box.left(), epsilon)) {
        return BoxFace::LEFT;
    }
    if (feq(point.x, box.right(), epsilon)) {
        return BoxFace::RIGHT;
    }
    if (feq(point.y, box.bottom(), epsilon)) {
        return BoxFace::BOTTOM;
    }
    if (feq(point.y, box.top(), epsilon)) {
        return BoxFace::TOP;
    }
    if (feq(point.z, box.front(), epsilon)) {
        return BoxFace::FRONT;
    }
    if (feq(point.z, box.back(), epsilon)) {
        return BoxFace::BACK;
    }
    MLE_UNREACHABLE_LOG("Point is not on any face of the box");
}

std::optional<f32> Plane::intersect(const Ray3f& ray) const {
    const f32 denom = glm::dot(normal_, ray.direction());
    if (feq(denom, 0.0F, 1e-6F)) {
        return std::nullopt;
    }

    const f32 t = glm::dot(point_ - ray.origin(), normal_) / denom;
    if (t < 0.0F) {
        return std::nullopt;
    }

    return t;
}

std::optional<f32> RectPlane::intersect(const Ray3f& ray) const {
    Plane infinite_plane(center_, normal_);
    auto t = infinite_plane.intersect(ray);
    if (!t) {
        return std::nullopt;
    }

    vec3f hit_pos = ray.pointAt(t.value());

    vec3f to_hit = hit_pos - center_;
    f32 u = glm::dot(to_hit, tangent_);
    f32 v = glm::dot(to_hit, bitangent_);

    if (std::abs(u) > width_ * 0.5F || std::abs(v) > height_ * 0.5F) {
        return std::nullopt;
    }

    return t;
}

void RectPlane::recomputeBasis() {
    vec3f up = (std::abs(normal_.y) < 0.99F) ? vec3f{0, 1, 0} : vec3f{1, 0, 0};
    tangent_ = glm::normalize(glm::cross(up, normal_));
    bitangent_ = glm::normalize(glm::cross(normal_, tangent_));
}

std::array<vec3f, 4> RectPlane::getEdges() const {
    const vec3f half_tangent = tangent_ * (width_ * 0.5F);
    const vec3f half_bitangent = bitangent_ * (height_ * 0.5F);

    const vec3f top_left = center_ - half_tangent + half_bitangent;
    const vec3f top_right = center_ + half_tangent + half_bitangent;
    const vec3f bottom_right = center_ + half_tangent - half_bitangent;
    const vec3f bottom_left = center_ - half_tangent - half_bitangent;

    return {top_left, top_right, bottom_right, bottom_left};
}
}  // namespace mle
