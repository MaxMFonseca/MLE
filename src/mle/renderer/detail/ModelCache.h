#pragma once

#include "mle/common/Utils.h"
#include "mle/renderer/Types.h"
#include "tiny_gltf.h"

// FIXME: make this thread safe
namespace mle::renderer::detail {
class ModelCache {
  public:
    MLE_NO_COPY_MOVE(ModelCache)

    ModelCache() = default;
    ~ModelCache() = default;

    void init();
    void reset();

    // void update();
    void frameBegin();

    [[nodiscard]] ModelRef get(const std::string& name);
    ModelRef loadModel(const std::string& name);
    ModelRef add(const std::string& name, const UploadModelData& model_data);

  private:
    void finishedUpload(const std::string& name);

  private:
    std::map<std::string, ModelHnd> models_;

    struct UploadingData {
        struct UploadingMeshData {
            BufferHnd v_staging;
            BufferHnd i_staging;
        };
        vk::Semaphore semaphore;
        std::vector<UploadingMeshData> meshes;
        std::string name;
    };
    std::vector<UploadingData> uploading_;
};
}  // namespace mle::renderer::detail
