#pragma once

#include "mle/common/Color.h"
#include "mle/common/Utils.h"
#include "mle/renderer/Camera.h"
#include "mle/renderer/Pipeline.h"
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
    void createGPipeline();
    void createPlanePipeline();
    void createLightingPipeline();
    void createWDS();

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
};
}  // namespace mle::renderer
