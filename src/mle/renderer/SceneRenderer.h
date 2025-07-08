#pragma once

#include <boost/mpl/void_fwd.hpp>
#include <map>

#include "mle/common/Color.h"
#include "mle/common/Hash.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/renderer/Camera.h"
#include "mle/renderer/CubeMap.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"

// TODO: This should be a template
namespace mle::renderer {
class SceneRenderer {
  public:
    struct Object {
        ModelRef model;
        mat4f transform;
    };

    struct Light {
        vec3f position;
        vec3f color;
        f32 intensity;
    };

    struct CreateInfo {
        vec2i image_extent;
        std::string skybox;
    };
    using CI = CreateInfo;

    struct LightingUBO {
        mat4f view;
        mat4f proj;
        mat4f view_proj;
        mat4f inv_view_proj;
        mat4f sun_light_mtx;
        vec3f sun_dir;
        f32 pad0{0.123};
        vec3f sun_color;
        f32 sun_intensity{1.0F};
    };

    struct Chunk {
        std::unordered_map<ID, Object> objects;
        std::unordered_map<ID, Light> lights;
    };

    static constexpr usize SHADOW_RESOLUTION = 4096;
    static constexpr usize CHUNK_SIZE = 32;

  public:
    MLE_NO_COPY_MOVE(SceneRenderer)

    SceneRenderer() = default;
    ~SceneRenderer() = default;

    void init(const CI& ci);

    // TODO: void resizeImages(vec2i new_extent);

    void shutdown();

    void render();

    [[nodiscard]] ImageRef getTarget() const { return target_img_.get(); }

    auto& camera() { return camera_; };

    auto& chunk(vec2i pos) { return chunks_[pos]; }

    void setSunDirection(const vec3f& dir) { sun_dir_ = glm::normalize(dir); }
    void setSunColor(const vec3f& color) { sun_color_ = color; }
    void setSunIntensity(f32 intensity) { sun_intensity_ = intensity; }
    void setFloorColor(const vec3f& color) { floor_color_ = color; }

  private:
    void initSun();
    void initG(const CI& ci);
    void initPlane();
    void initLighting(const CI& ci);
    void initSkybox(const CI& ci);
    void initDebug();

    struct RenderingData {
        RenderingThread thread;

        mat4f view{};
        mat4f proj{};
        mat4f view_proj{};
        mat4f inv_view_proj{};

        Frustum camera_frustum{};

        Frustum sun_frustum{};
        Camera sun_cam;
        mat4f sun_vp{};

        vec2f camera_center_y0{};
        std::vector<vec2f> camera_frustum_inter_y0;

        std::map<ModelRef, std::vector<mat4f>> rendered_objs;
        std::vector<vec2i> rendered_planes;
        BufferSlice rendered_transforms;
    };

    void renderSun(RenderingData& rdata);
    void calcSunCamera(RenderingData& rdata);
    void renderGBuffer(RenderingData& rdata);
    void renderLighting(RenderingData& rdata);
    void renderSkybox(RenderingData& rdata);
    void renderDebug(RenderingData& rdata);

    void cullSun(RenderingData& rdata);
    void cullCamera(RenderingData& rdata);

  private:
    Camera camera_{};

    std::unordered_map<vec2i, Chunk> chunks_;

    vec3f sun_dir_ = {.1, -1, .1};
    vec3f sun_color_{1};
    f32 sun_intensity_ = 1;
    vec3f floor_color_{.1, .9, .3};

    ImageHnd albedo_img_;
    ImageHnd normal_img_;
    ImageHnd material_img_;
    ImageHnd depth_img_;
    ImageHnd target_img_;
    ImageHnd shadow_img_;
    CubeMap skybox_;

    struct {
        PipelineRef vox;  // TODO: This should not be here, the objects should be rendered with the model pipeline
        PipelineRef plane;
        PipelineRef lighting;
        PipelineRef skybox;
        PipelineRef polygon;
        PipelineRef sun;
    } pipelines_{};

    struct {
        std::vector<std::pair<Polygon3D, Color>> polygons;
    } debug_;
};
}  // namespace mle::renderer
