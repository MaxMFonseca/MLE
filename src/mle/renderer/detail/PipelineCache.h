#pragma once

#include <map>

#include "../Pipeline.h"

namespace mle::renderer::detail {
class PipelineCache final {
  public:
    MLE_NO_COPY_MOVE(PipelineCache)

    PipelineCache() = default;
    ~PipelineCache() = default;

    void init();
    void shutdown();

    PipelineRef setPipeline(const std::string& name, const Pipeline::CI& pipeline_ci);
    PipelineRef getPipeline(const std::string& name);
    PipelineRef getPipelineOrSet(const std::string& name, const Pipeline::CI& pipeline_ci);

  private:
    std::map<std::string, PipelineHnd> pipelines_;
};
}  // namespace mle::renderer::detail
