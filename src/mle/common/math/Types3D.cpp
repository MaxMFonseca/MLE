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

    const f32 t = glm::dot(origin_ - ray.origin(), normal_) / denom;
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

RectPlane RectPlane::fromSquareLBCorner(vec3f corner, vec3f normal, f32 size) {
    RectPlane plane;
    plane.normal_ = glm::normalize(normal);
    plane.width_ = size;
    plane.height_ = size;

    plane.recomputeBasis();

    plane.center_ = corner + 0.5F * size * (plane.tangent_ + plane.bitangent_);

    return plane;
}

void Frustum::ABCDPlane::normalize() {
    f32 len = glm::length(vec3f(a, b, c));
    if (len > 0.0F) {
        a /= len;
        b /= len;
        c /= len;
        d /= len;
    }
}

Frustum::Frustum(const mat4f& m) :
    l_(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]),
    r_(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]),
    t_(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]),
    b_(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]),
    n_(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]),
    f_(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]) {
    l_.normalize();
    r_.normalize();
    t_.normalize();
    b_.normalize();
    n_.normalize();
    f_.normalize();
}

f32 Frustum::signedDist(const vec3f& p) const {
    return std::max({l_.signedDist(p), r_.signedDist(p), b_.signedDist(p), t_.signedDist(p), n_.signedDist(p), f_.signedDist(p)});
}

bool Frustum::contains(const vec3f& p) const {
    return l_.signedDist(p) >= 0 && r_.signedDist(p) >= 0 && b_.signedDist(p) >= 0 && t_.signedDist(p) >= 0 && n_.signedDist(p) >= 0 && f_.signedDist(p) >= 0;
}
bool Frustum::contains(const RectPlane& plane) const {
    auto corners = plane.getEdges();
    for (const ABCDPlane& p : {l_, r_, b_, t_, n_, f_}) {
        bool all_outside = true;
        for (const auto& c : corners) {
            if (p.signedDist(c) >= 0) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return false;
        }
    }
    return true;
}

bool Frustum::contains(const Sphere& s) const {
    const vec3f& c = s.center();
    const f32 r = s.radius();

    return std::ranges::all_of(std::array{l_, r_, b_, t_, n_, f_}, [&](const ABCDPlane& p) { return p.signedDist(c) >= -r; });
}

bool Frustum::contains(const Box<f32>& box) const {
    const vec3f min = box.min();
    const vec3f max = box.max();

    std::array<vec3f, 8> corners = {
        vec3f{min.x, min.y, min.z}, vec3f{max.x, min.y, min.z}, vec3f{min.x, max.y, min.z}, vec3f{max.x, max.y, min.z},
        vec3f{min.x, min.y, max.z}, vec3f{max.x, min.y, max.z}, vec3f{min.x, max.y, max.z}, vec3f{max.x, max.y, max.z},
    };

    for (const ABCDPlane& p : {l_, r_, b_, t_, n_, f_}) {
        bool all_outside = true;
        for (const auto& corner : corners) {
            if (p.signedDist(corner) >= 0) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return false;
        }
    }
    return true;
}
}  // namespace mle
