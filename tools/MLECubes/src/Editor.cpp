#include "Editor.h"

#include "mle/ui/UI.h"

namespace mle_cubes {
void Editor::init() {
    editor_view_created_id_ = mle::ui::addListener("editor_view_created", [this]() { editorViewCreated(); });
}

void Editor::editorViewCreated() {
    MLE_ASSERT(editor_view_created_id_ != mle::max<mle::u64>());
    mle::ui::removeListener("editor_view_created", editor_view_created_id_);
    editor_view_created_id_ = mle::max<mle::u64>();

    MLE_VC("HERE");

    auto e = mle::ui::getElement("EditorView");
    auto& c = mle::ui::getRegistry().emplace<mle::ui::element::comp::Background>(e);
    c.color = mle::Color::fromString("RED");
};
}  // namespace mle_cubes
