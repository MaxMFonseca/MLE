#pragma once

#include "mle/common/Color.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/renderer/Camera.h"
#include "mle/renderer/CubeMap.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
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

    struct Chunk {
        std::unordered_map<ID, Object> objects;
        std::unordered_map<ID, Light> lights;
        Texture texture;
        std::array<Color, 4> colors;
        bool ready = false;
    };

    struct CreateInfo {
        vec2i image_extent;
        vec2i chunk_count;
        int chunk_size;
        std::string cube_map;
    };
    using CI = CreateInfo;

  public:
    MLE_NO_COPY_MOVE(SceneRenderer)

    SceneRenderer() = default;
    ~SceneRenderer() = default;

    void init(const CI& ci);

    // TODO: void resizeImages(vec2i new_extent);

    void shutdown();

    void render();

    [[nodiscard]] ImageRef getTarget() const { return target_image_.get(); }

    auto& camera() { return camera_; };

    auto& getChunk(int x, int y) { return chunks_.at(x).at(y); }

  private:
    struct RenderReadyChunk {
        std::vector<Object> objects;
        std::vector<Light> lights;

        struct Plane {
            std::array<vec2f, 4> uv_coords;
            std::array<Color, 4> colors;
            vec2f left_bottom;
            // Texture texture;
        } plane;
    };

    std::optional<RenderReadyChunk> renderChunk(vec2i position, Chunk& chunk, const mat4f& vp, const Frustum& frustum) const;

  private:
    void createSun();
    void createGPipeline();
    void createPlanePipeline();
    void createLightingPipeline();
    void createWDS();

    void createCubeMap(const std::string& name);

    void renderCubeMap(RenderingThread& thread);

  private:
    Camera camera_;

    // TODO: make this look less stupid
    std::vector<std::vector<Chunk>> chunks_;

    f32 chunk_size_{};

    struct Images {
        ImageHnd albedo;
        ImageHnd normal;
        ImageHnd material;
        ImageHnd depth;
    } g_;

    ImageHnd target_image_;

    PipelineHnd g_pipeline_;
    PipelineHnd plane_pipeline_;
    PipelineHnd lighting_pipeline_;
    vk::Sampler sampler_;
    vk::DescriptorSetLayout lighting_dsl_;

    std::array<vk::DescriptorImageInfo, 3> lighting_dii_;
    std::vector<vk::WriteDescriptorSet> lighting_wds_;

    CubeMap cube_map_;

    struct {
        PipelineHnd pipeline;
        ImageHnd image;
        Color color;
        f32 intensity{1.0F};
        vec3f position;
        vec3f direction = glm::normalize(vec3f(0.1F, -1.0F, 0.1F));
    } the_sun_;

    struct {
        struct Polygon {
            std::vector<vec3f> points;
            Color color = Color{1.F, 0, 0, 0.1};
        };

        std::vector<Polygon> polygons;
        PipelineHnd polygon_pipeline;

    } debug_;

    void createDebug();
    void renderDebug(RenderingThread& thread, const mat4f& vp);
};
}  // namespace mle::renderer
