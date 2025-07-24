#include "Camera.h"

#include <cmath>
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

#include "mle/common/Utils.h"

namespace mle::renderer {
void Camera::setEye(const vec3f& eye) {
    eye_ = eye;
    view_dirty_ = true;
}

void Camera::lookAtDir(const vec3f& dir) {
    target_ = eye_ + glm::normalize(dir);
    view_dirty_ = true;
}

void Camera::move(const vec3f& offset) {
    eye_ += offset;
    target_ += offset;
    view_dirty_ = true;
}

void Camera::setTarget(const vec3f& target) {
    target_ = target;
    view_dirty_ = true;
}

void Camera::setUp(const vec3f& up) {
    up_ = up;
    view_dirty_ = true;
}

void Camera::rotateAroundTarget(f32 yaw_rad, f32 pitch_rad) {
    const vec3f direction = eye_ - target_;
    const f32 radius = glm::length(direction);

    f32 theta = std::atan2(direction.z, direction.x);
    f32 phi = std::acos(direction.y / radius);

    theta += yaw_rad;
    phi += pitch_rad;

    const f32 epsilon = 0.001F;
    phi = glm::clamp(phi, epsilon, glm::pi<f32>() - epsilon);

    vec3f new_direction = {radius * sin(phi) * cos(theta), radius * cos(phi), radius * sin(phi) * sin(theta)};

    eye_ = target_ + new_direction;
    view_dirty_ = true;
}

void Camera::setFov(f32 fov_deg) {
    fov_deg_ = fov_deg;
    proj_dirty_ = true;
}

void Camera::setAspect(f32 aspect) {
    aspect_ = aspect;
    proj_dirty_ = true;
}

void Camera::setNear(f32 near_z) {
    near_ = near_z;
    proj_dirty_ = true;
}

void Camera::setFar(f32 far_z) {
    far_ = far_z;
    proj_dirty_ = true;
}

void Camera::setLeft(f32 left) {
    left_ = left;
    proj_dirty_ = true;
}

void Camera::setRight(f32 right) {
    right_ = right;
    proj_dirty_ = true;
}

void Camera::setBottom(f32 bottom) {
    bottom_ = bottom;
    proj_dirty_ = true;
}

void Camera::setTop(f32 top) {
    top_ = top;
    proj_dirty_ = true;
}

void Camera::setProjType(ProjType type) {
    if (proj_type_ != type) {
        proj_type_ = type;
        proj_dirty_ = true;
    }
}

mat4f Camera::getView() {
    if (view_dirty_) {
        updateView();
    }
    return view_;
}

mat4f Camera::getProj() {
    if (proj_dirty_) {
        updateProj();
    }
    return proj_;
}

mat4f Camera::getViewProj() {
    if (view_proj_dirty_ || view_dirty_ || proj_dirty_) {
        updateViewProj();
    }
    return view_proj_;
}

void Camera::updateView() {
    view_ = glm::lookAt(eye_, target_, up_);
    view_dirty_ = false;
    view_proj_dirty_ = true;
}

void Camera::updateProj() {
    if (proj_type_ == ProjType::PERSPECTIVE) {
        proj_ = glm::perspective(glm::radians(fov_deg_), aspect_, near_, far_);
    } else {
        proj_ = glm::ortho(left_, right_, bottom_, top_, near_, far_);
    }
    proj_[1][1] = -proj_[1][1];
    proj_dirty_ = false;
    view_proj_dirty_ = true;
}

void Camera::updateViewProj() {
    view_proj_ = getProj() * getView();
    view_proj_dirty_ = false;
}

void Camera::walk(const vec3f& offset) {
    eye_ += offset;
    target_ += offset;

    view_dirty_ = true;
    view_proj_dirty_ = true;
}

void Camera::lookUp(f32 angle_rad) {
    vec3f forward = glm::normalize(target_ - eye_);
    vec3f right = glm::normalize(glm::cross(forward, up_));

    mat4f rot = glm::rotate(mat4f(1.0F), angle_rad, right);
    vec3f new_forward = glm::normalize(vec3f(rot * vec4f(forward, 0.0F)));

    const f32 epsilon = 0.001F;
    if (std::abs(glm::dot(new_forward, up_)) > 1.0F - epsilon) {
        return;
    }

    target_ = eye_ + new_forward;
    up_ = glm::normalize(glm::cross(right, new_forward));

    view_dirty_ = true;
    view_proj_dirty_ = true;
}

void Camera::setRect(f32 left, f32 right, f32 bottom, f32 top) {
    setLeft(left);
    setRight(right);
    setBottom(bottom);
    setTop(top);
}

void Camera::rotateTargetAroundEye(vec3f v) {
    vec3f offset = target_ - eye_;

    quat qyaw = glm::angleAxis(v.y, glm::vec3(0, 1, 0));
    quat qpitch = glm::angleAxis(v.x, glm::vec3(1, 0, 0));
    quat qroll = glm::angleAxis(v.z, glm::vec3(0, 0, 1));
    auto rot = qyaw * qpitch * qroll;

    offset = rot * offset;

    target_ = eye_ + offset;

    view_dirty_ = true;
    view_proj_dirty_ = true;
}

void Camera::zoom(f32 delta) {
    vec3f offset = target_ - eye_;
    f32 distance = glm::max(glm::length(offset) - delta, 0.1F);
    eye_ = target_ - glm::normalize(offset) * distance;

    view_dirty_ = true;
    view_proj_dirty_ = true;
}

std::vector<AABB> Camera::computeViewClusters(u32 tile_size_px, u32 screen_width, u32 screen_height, u32 z_slices) const {
    std::vector<AABB> clusters;

    if (proj_type_ != ProjType::PERSPECTIVE) {
        MLE_UNREACHABLE_LOG("computeViewClusters only supports perspective projection for now");
    }

    const u32 x_tiles = (screen_width + tile_size_px - 1) / tile_size_px;
    const u32 y_tiles = (screen_height + tile_size_px - 1) / tile_size_px;

    clusters.reserve(as<usize>(x_tiles * y_tiles * z_slices));

    const f32 fov_rad = glm::radians(fov_deg_);
    const f32 tan_half_fov = std::tan(fov_rad * 0.5F);

    const f32 log_near = std::log(near_ + 1.0F);
    const f32 log_far = std::log(far_ + 1.0F);
    const f32 log_range = log_far - log_near;

    for (u32 z = 0; z < z_slices; ++z) {
        const f32 zf = as<f32>(z) / as<f32>(z_slices);
        const f32 zn = as<f32>(z + 1) / as<f32>(z_slices);

        const f32 z_near = std::exp(log_near + (log_range * zf)) - 1.0F;
        const f32 z_far = std::exp(log_near + (log_range * zn)) - 1.0F;

        for (u32 y = 0; y < y_tiles; ++y) {
            const f32 y0 = (as<f32>(y) / as<f32>(y_tiles) * 2.0F) - 1.0F;
            const f32 y1 = (as<f32>(y + 1) / as<f32>(y_tiles) * 2.0F) - 1.0F;

            for (u32 x = 0; x < x_tiles; ++x) {
                const f32 x0 = (as<f32>(x) / as<f32>(x_tiles) * 2.0F) - 1.0F;
                const f32 x1 = (as<f32>(x + 1) / as<f32>(x_tiles) * 2.0F) - 1.0F;

                std::array<vec3f, 8> corners{};
                u32 i = 0;
                for (f32 ndc_z : {z_near, z_far}) {
                    for (f32 ndc_y : {y0, y1}) {
                        for (f32 ndc_x : {x0, x1}) {
                            const f32 view_x = ndc_x * ndc_z * tan_half_fov * aspect_;
                            const f32 view_y = ndc_y * ndc_z * tan_half_fov;
                            const f32 view_z = -ndc_z;
                            corners.at(i++) = vec3f{view_x, view_y, view_z};
                        }
                    }
                }

                AABB aabb;
                for (const auto& c : corners) {
                    aabb.expand(c);
                }

                clusters.push_back(aabb);
            }
        }
    }

    return clusters;
};
}  // namespace mle::renderer
