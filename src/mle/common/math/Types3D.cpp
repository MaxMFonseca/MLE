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

}  // namespace mle
