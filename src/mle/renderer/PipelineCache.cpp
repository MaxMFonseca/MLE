#include "PipelineCache.h"

namespace mle {
void PipelineCache::shutdown() {
    MLE_I("Shutting down PipelineCache");
    pipelines_.clear();
}

const Pipeline& PipelineCache::setPipeline(const std::string& name, const Pipeline::CI& pipeline_ci) {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_I("Setting pipeline: {}", name);
    auto er = pipelines_.emplace(name, Pipeline::createHnd(pipeline_ci));
    return *er.first->second;
}

const Pipeline& PipelineCache::getPipeline(const std::string& name) {
    MLE_ASSERT_LOG(!name.empty(), "Pipeline name cannot be empty");
    MLE_T("Getting pipeline: {}", name);
    auto it = pipelines_.find(name);
    if (it != pipelines_.end()) {
        return *it->second;
    }
    MLE_UNREACHABLE_LOG("Pipeline not found: {}", name);
}
}  // namespace mle
