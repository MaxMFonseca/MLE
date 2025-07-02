#pragma once

#include "mle/common/Utils.h"
#include "mle/core/Scene.h"

namespace mle_cubes {
using namespace mle;  // NOLINT
class Editor : public core::Scene {
  public:
    MLE_NO_COPY_MOVE(Editor)

    Editor() = default;
    ~Editor() override = default;

    void init() override;

  private:
    void editorViewCreated();

  private:
    mle::ID editor_view_created_id_ = mle::max<mle::u64>();
};
}  // namespace mle_cubes
