#pragma once

#include <map>

#include "Pipeline.h"
#include "Types.h"

// TODO: Maybe instead of storing created pieplines I could store its create info and shared ptr, and create/destroy on demand
// The same for other resources too ?
namespace mle {
class PipelineCache final {
  public:
    MLE_NO_COPY_MOVE(PipelineCache)

    ~PipelineCache() = default;

    const Pipeline& setPipeline(const std::string& name, const Pipeline::CI& pipeline_ci);
    const Pipeline& getPipeline(const std::string& name);

  private:
    friend class Renderer;
    PipelineCache() = default;
    void init() {};
    void shutdown();

    std::map<std::string, PipelineHnd> pipelines_;
};
}  // namespace mle
