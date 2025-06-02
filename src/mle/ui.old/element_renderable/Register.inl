#pragma once

#include "mle/common/Logger.h"
#include "mle/ui/element_renderable/FlexContainer.h"
#include "mle/ui/element_renderable/ListContainer.h"
#include "mle/ui/element_renderable/Root.h"
#include "mle/ui/element_renderable/Sprite.h"
#include "mle/ui/element_renderable/Text.h"
#include "mle/ui/element_renderable/shader/Lua.h"
#include "mle/ui/element_renderable/shader/ProgressBar.h"
#include "shader/Border.h"

namespace mle::ui::element_renderable {
namespace {
inline void registerRenderableTypes() {
    MLE_D("Element extra register builtin");
    Element::registerRenderableType("flex", Element::Renderable::makeNew<FlexContainer>);
    Element::registerRenderableType("list", Element::Renderable::makeNew<ListContainer>);
    Element::registerRenderableType("root", Element::Renderable::makeNew<Root>);
    Element::registerRenderableType("sprite", Element::Renderable::makeNew<Sprite>);
    Element::registerRenderableType("text", Element::Renderable::makeNew<Text>);

    Element::registerRenderableType("border", Element::Renderable::makeNew<Border>);
    Element::registerRenderableType("progress_bar", Element::Renderable::makeNew<ProgressBar>);
    Element::registerRenderableType("shader", Element::Renderable::makeNew<Lua>);
}

inline void registerLuaTypes() {
    auto r_ut = lua::getUserType<Element::Renderable>("ui_Element_Renderable");
    r_ut["asText"] = &Element::Renderable::as<Text>;
    r_ut["asSprite"] = &Element::Renderable::as<Sprite>;
    r_ut["asFlexContainer"] = &Element::Renderable::as<FlexContainer>;
    r_ut["asListContainer"] = &Element::Renderable::as<ListContainer>;
    r_ut["asShader"] = &Element::Renderable::as<Lua>;

    Text::registerLuaTypes();
    Sprite::registerLuaTypes();
    FlexContainer::registerLuaTypes();
    ListContainer::registerLuaTypes();
    Lua::registerLuaTypes();
}
}  // namespace
}  // namespace mle::ui::element_renderable
