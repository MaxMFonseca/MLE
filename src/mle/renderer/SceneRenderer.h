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
        vec2f pos;
        Color color;
        std::vector<Object> objects;
        std::vector<Light> lights;
    };

  public:
    MLE_NO_COPY_MOVE(SceneRenderer)

    SceneRenderer() = default;
    ~SceneRenderer() = default;

    void init(vec2i extent);
    void shutdown();

    void render();

    [[nodiscard]] ImageRef getTarget() const { return target_image_.get(); }

    auto& camera() { return camera_; };

    void setChunk(int x, int y, Chunk&& chunk);

  private:
    void createGPipeline();
    void createPlanePipeline();
    void createLightingPipeline();
    void createWDS();

  private:
    Camera camera_;

    std::array<std::array<Chunk, 5>, 5> chunks_;

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
