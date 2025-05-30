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

Boxf::Face pointFace(const Boxf& box, vec3f point, f32 epsilon) {
    if (feq(point.x, box.left(), epsilon)) {
        return Boxf::Face::LEFT;
    }
    if (feq(point.x, box.right(), epsilon)) {
        return Boxf::Face::RIGHT;
    }
    if (feq(point.y, box.bottom(), epsilon)) {
        return Boxf::Face::BOTTOM;
    }
    if (feq(point.y, box.top(), epsilon)) {
        return Boxf::Face::TOP;
    }
    if (feq(point.z, box.front(), epsilon)) {
        return Boxf::Face::FRONT;
    }
    if (feq(point.z, box.back(), epsilon)) {
        return Boxf::Face::BACK;
    }
    MLE_UNREACHABLE_LOG("Point is not on any face of the box");
}
}  // namespace mle
