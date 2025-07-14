#include "Editor.h"

#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"

namespace mle_cubes {
void Editor::init() {
    MLE_C("MLECubes App initializing");

    mle::ui::setNextRoot("i/ui/editor/layout");

    mle::ui::listenNamedElementCreated("EditorView", [](entt::entity e) { editorViewCreated(e); });
}

void Editor::editorViewCreated(entt::entity e) {
    auto& c = mle::ui::getRegistry().emplace<mle::ui::element::comp::Background>(e);
    c.color = mle::Color::fromString("RED");
};
}  // namespace mle_cubes
