#pragma once

#include <map>

#include "entt/entt.hpp"
#include "mle/common/Color.h"
#include "mle/common/Hash.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/game/Types.h"
#include "mle/renderer/Camera.h"
#include "mle/renderer/CubeMap.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"

namespace mle::game {
class DefaultSceneRenderer {
  public:
    struct BaseComp {
        f32 last_seen{0};
    };
    struct TransformComp {
        vec3f pos{};
        f32 rot = 0;
        f32 time = 0;
    };
    struct TargetTransformComp : TransformComp {};

    struct ModelComp {
        renderer::ModelRef v;
    };
    struct LightComp {
        vec3f color{};
        f32 intensity{};
        f32 time{};
    };
    struct TargetLightComp : LightComp {};

    struct AnimationComp {
        // renderer::AnimationRef v;
        f32 time;
    };
    struct TargetAnimationComp : AnimationComp {};

  public:
    constexpr static f32 CHUNK_SIZE = 32;

  public:
    MLE_NO_COPY_MOVE(DefaultSceneRenderer)

    DefaultSceneRenderer() = default;
    ~DefaultSceneRenderer() = default;

    void init(ServerOutED& server_out_ed);

    void shutdown();

    void render(f32 dt);

    [[nodiscard]] renderer::ImageRef getTarget() const { return target_img_.get(); }

    void cameraFollow(entt::entity e, vec3f offset = {-3, 50, -30});
    auto& camera() { return camera_; };

    void setSunDirection(const vec3f& dir) { sun_dir_ = glm::normalize(dir); }
    void setSunColor(const vec3f& color) { sun_color_ = color; }
    void setSunIntensity(f32 intensity) { sun_intensity_ = intensity; }
    void setFloorColor(const vec3f& color) { floor_color_ = color; }

    [[nodiscard]] vec2f cursorPosOnWorld(vec2f cursor_pos);

  private:
    void initSun();
    void initG();
    void initPlane();
    void initLighting();
    // void initSkybox();
    // void initDebug();

    struct RenderingData {
        renderer::RenderingThread thread;

        f32 dt;

        mat4f view{};
        mat4f proj{};
        mat4f view_proj{};
        mat4f inv_view_proj{};

        Frustum camera_frustum{};

        Frustum sun_frustum{};
        renderer::Camera sun_cam;
        mat4f sun_vp{};

        vec2f camera_center_y0{};
        std::vector<vec2f> camera_frustum_inter_y0;

        std::map<renderer::ModelRef, std::vector<mat4f>> rendered_objs;
        std::vector<vec2i> rendered_planes;
        renderer::BufferSlice rendered_transforms;
    };

    void updateEntities(RenderingData& rdata);

    void renderSun(RenderingData& rdata);
    void calcSunCamera(RenderingData& rdata);
    void renderGBuffer(RenderingData& rdata);
    void renderLighting(RenderingData& rdata);
    // void renderSkybox(RenderingData& rdata);
    // void renderDebug(RenderingData& rdata);

    void cullSun(RenderingData& rdata);
    void cullCamera(RenderingData& rdata);

  private:
    renderer::Camera camera_{};

    entt::registry reg_;

    f32 last_time_ = 0.0F;
    f32 server_time_ = 0.0F;

    vec3f floor_color_{.1, .9, .3};

    vec3f sun_dir_ = glm::normalize(vec3f{-.3, -1, -.4});
    vec3f sun_color_{1};
    f32 sun_intensity_ = 1;
    vec3f fog_color_ = {.005, .01, .02};
    f32 fog_start_ = 100.0F;
    f32 fog_density_ = 0.01F;

    renderer::ImageHnd albedo_img_;
    renderer::ImageHnd normal_img_;
    renderer::ImageHnd material_img_;
    renderer::ImageHnd depth_img_;
    renderer::ImageHnd target_img_;
    renderer::ImageHnd shadow_img_;
    // renderer::CubeMap skybox_;

    struct {
        renderer::PipelineRef sun;
        renderer::PipelineRef vox;
        renderer::PipelineRef plane;
        renderer::PipelineRef lighting;
        renderer::PipelineRef polygon;
        // renderer::PipelineRef skybox;
    } pipelines_{};

    struct {
        std::vector<std::pair<Polygon3D, Color>> polygons;
    } debug_;

    ServerTimeListener server_time_listener_;
    ServerNewEnttListener server_new_entt_listener_;
    ServerEnttModelListener server_entt_model_listener_;
    ServerEnttPositionListener server_entt_position_listener_;
    ServerEnttRotationListener server_entt_rotation_listener_;
};
}  // namespace mle::game
