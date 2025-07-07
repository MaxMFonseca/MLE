#pragma once

#include <map>

#include "mle/common/Color.h"
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
        std::string cube_map;
    };
    using CI = CreateInfo;

    struct LightingUBO {
        mat4f view;
        mat4f proj;
        mat4f view_proj;
        mat4f inv_view_proj;
        mat4f sun_light_mtx;
        vec3f sun_dir;
        vec3f sun_color;
    };

    struct Chunk {
        std::unordered_map<ID, Object> objects;
        std::unordered_map<ID, Light> lights;

        bool ready = false;
    };

    static constexpr usize SHADOW_RESOLUTION = 4096;
    static constexpr usize CHUNK_SIZE = 32;
    static constexpr usize WORLD_VIEW_SIZE = 5;
    static constexpr usize MAX_LOADED_WORLD_VIEWS = 9;

    struct WorldView {
        std::array<std::array<Chunk, WORLD_VIEW_SIZE>, WORLD_VIEW_SIZE> chunks{};
        vec2i position{};
        bool valid = false;
    };

    struct Dimension {
        std::array<WorldView, MAX_LOADED_WORLD_VIEWS> world_views;  // ring buffer
        Color plane_color{0U, 1, 0};
        Color sun_color{1U, 1, 1};
        f32 sun_intensity{1.0F};
        vec3f sun_dir{1.F, -1.0F, -1.F};
    };

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

    // FIXME: remove this
    auto& getDimensions() { return dimensions_; }

  private:
    void initSun();
    void initG(const CI& ci);
    void initPlane();
    void initLighting(const CI& ci);
    void initCubeMap(const CI& ci);
    void initDebug();

    struct RenderingData {
        explicit RenderingData(Dimension& dim) :
            dim(dim) {};

        const Dimension& dim;
        ID dim_id{0U};

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
    };

    void renderSun(RenderingData& rdata);
    static void calcSunCamera(RenderingData& rdata);
    void renderGBuffer(RenderingData& rdata);
    void renderLighting(RenderingData& rdata);
    void renderCubeMap(RenderingData& rdata);
    void renderDebug(RenderingData& rdata);

    void cullSun(RenderingData& rdata);
    void cullCamera(RenderingData& rdata);

  private:
    Camera camera_;

    std::unordered_map<ID, Dimension> dimensions_;

    ImageHnd albedo_img_;
    ImageHnd normal_img_;
    ImageHnd material_img_;
    ImageHnd depth_img_;
    ImageHnd target_img_;
    ImageHnd shadow_img_;
    CubeMap cube_map_;

    struct {
        PipelineRef vox;  // TODO: This should not be here, the objects should be rendered with the model pipeline
        PipelineRef plane;
        PipelineRef lighting;
        PipelineRef cube_map;
        PipelineRef polygon;
        PipelineRef sun;
    } pipelines_{};

    struct {
        std::vector<std::pair<Polygon3D, Color>> polygons;
    } debug_;
};
}  // namespace mle::renderer
