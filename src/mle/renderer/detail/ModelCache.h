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

    Expected<Model> get(const std::string& name);
    Result loadModel(const std::string& name, std::function<void(Model)>&& callback = {});

    void addCallbackToUploaded(const std::string& name, std::function<void(Model)>&& callback);

  private:
    void finishedUpload(const std::string& name);

  private:
    struct ModelData {
        BufferHnd vertex_buffer;
        BufferHnd index_buffer;
        int index_count = 0;
        enum class State : u8 { OK, UPLOADING, OUT } state = State::OUT;
    };
    std::map<std::string, ModelData> models_;

    struct ModelUploadData {
        std::string name;
        std::vector<std::function<void(Model)>> callbacks;
        BufferHnd v_staging;
        BufferHnd i_staging;
        vk::Semaphore semaphore;
    };

    std::vector<ModelUploadData> uploading_;
};
}  // namespace mle::renderer::detail
