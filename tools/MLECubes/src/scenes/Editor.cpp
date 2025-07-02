#include "Editor.h"

#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"

namespace mle_cubes {
void Editor::init() {
    MLE_C("MLECubes App initializing");

    mle::ui::setNextRoot(mle::lua::require("i/ui/editor/layout"));

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
