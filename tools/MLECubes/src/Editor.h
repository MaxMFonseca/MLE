#pragma once

#include "mle/common/Utils.h"

namespace mle_cubes {
class Editor {
  public:
    MLE_NO_COPY_MOVE(Editor)

    Editor() = default;
    ~Editor() = default;

    void init();

    void render();

  private:
    void editorViewCreated();

  private:
    mle::ID editor_view_created_id_ = mle::max<mle::u64>();
};
}  // namespace mle_cubes
