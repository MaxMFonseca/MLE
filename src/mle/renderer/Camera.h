#pragma once

#include "glm/exponential.hpp"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types3D.h"
#include "spdlog/common.h"
namespace mle::renderer {
class Camera {
  public:
    enum class ProjType : u8 { PERSPECTIVE, ORTH };

  public:
    Camera(const Camera&) = default;
    Camera(Camera&&) = default;
    Camera& operator=(const Camera&) = default;
    Camera& operator=(Camera&&) = default;

    Camera() = default;
    ~Camera() = default;

    void setEye(const vec3f& eye);
    void setPosition(const vec3f& position) { setEye(position); }
    void setTarget(const vec3f& target);
    void setUp(const vec3f& up);

    void lookAtDir(const vec3f& dir);
    void move(const vec3f& offset);

    void walk(const vec3f& offset);

    void rotateAroundTarget(f32 yaw_rad, f32 pitch_rad);
    void rotateTargetAroundEye(vec3f v);

    void lookUp(f32 angle_rad);

    void zoom(f32 delta);

    void setFov(f32 fov_deg);
    void setAspect(f32 aspect);
    void setNear(f32 near_z);
    void setFar(f32 far_z);

    void setLeft(f32 left);
    void setRight(f32 right);
    void setBottom(f32 bottom);
    void setTop(f32 top);

    void setRect(f32 left, f32 right, f32 bottom, f32 top);
    void setRect(f32 v) { setRect(-v, v, -v, v); }

    [[nodiscard]] ProjType getProjType() const { return proj_type_; }

    void setProjType(ProjType type);

    mat4f getView();
    mat4f getProj();
    mat4f getViewProj();
    mat4f getInvViewProj() { return glm::inverse(getViewProj()); }

    auto getType() { return proj_type_; }

    [[nodiscard]] auto getPos() const { return eye_; }
    [[nodiscard]] auto getEye() const { return eye_; }
    [[nodiscard]] auto getTarget() const { return target_; }
    [[nodiscard]] auto getUp() const { return up_; }

    [[nodiscard]] auto getFov() const { return fov_deg_; }
    [[nodiscard]] auto getAspect() const { return aspect_; }
    [[nodiscard]] auto getNear() const { return near_; }
    [[nodiscard]] auto getFar() const { return far_; }

    [[nodiscard]] auto getLeft() const { return left_; }
    [[nodiscard]] auto getRight() const { return right_; }
    [[nodiscard]] auto getBottom() const { return bottom_; }
    [[nodiscard]] auto getTop() const { return top_; }

    Frustum getFrustum() { return Frustum{getViewProj()}; }

  private:
    void updateView();
    void updateProj();
    void updateViewProj();

  private:
    vec3f eye_{0.0F, 0.0F, 5.0F};
    vec3f target_{0.0F, 0.0F, 0.0F};
    vec3f up_{0.0F, 1.0F, 0.0F};

    mat4f view_{1.0F};
    bool view_dirty_ = true;

    f32 fov_deg_ = 60.0F;
    f32 aspect_ = 16.0F / 9.0F;
    f32 near_ = 0.1F;
    f32 far_ = 1000.0F;

    f32 left_ = -1.0F, right_ = 1.0F, bottom_ = -1.0F, top_ = 1.0F;

    ProjType proj_type_ = ProjType::PERSPECTIVE;

    mat4f proj_{1.0F};
    bool proj_dirty_ = true;

    mat4f view_proj_ = mat4f(1.0F);
    bool view_proj_dirty_ = true;
};
}  // namespace mle::renderer

namespace fmt {
template <>
struct formatter<mle::renderer::Camera> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::renderer::Camera& cam, FormatContext& ctx) const {
        if (cam.getProjType() == mle::renderer::Camera::ProjType::PERSPECTIVE) {
            return format_to(ctx.out(), "Camera[pos: {} target: {}] (Perspective, FOV: {:.2f}, Aspect: {:.2f}, Near: {:.2f}, Far: {:.2f})", cam.getPos(),
                             cam.getTarget(), cam.getFov(), cam.getAspect(), cam.getNear(), cam.getFar());
        }

        return format_to(ctx.out(),
                         "Camera[pos: {} target: {}] (Orthographic, Left: {:.2f}, Right: {:.2f}, Bottom: {:.2f}, Top: {:.2f}, Near: {:.2f}, Far: {:.2f})",
                         cam.getPos(), cam.getTarget(), cam.getLeft(), cam.getRight(), cam.getBottom(), cam.getTop(), cam.getNear(), cam.getFar());
    }
};
}  // namespace fmt
