#include "Camera.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

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
    proj_dirty_ = false;
    view_proj_dirty_ = true;
}

void Camera::updateViewProj() {
    view_proj_ = getProj() * getView();
    view_proj_dirty_ = false;
}

void Camera::walk(const vec3f& offset) {
    const vec3f forward = glm::normalize(target_ - eye_);
    const vec3f right = glm::normalize(glm::cross(forward, up_));
    const vec3f up = glm::normalize(glm::cross(right, forward));

    const vec3f world_offset = offset.x * right + offset.y * up + offset.z * forward;

    eye_ += world_offset;
    target_ += world_offset;

    view_dirty_ = true;
    view_proj_dirty_ = true;
}

void Camera::lookUpDown(f32 angle_rad) {
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
}  // namespace mle::renderer
