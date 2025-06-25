#pragma once

#include "mle/common/Color.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/lua/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/Utils.h"
#include "mle/ui/element/Renderable.h"
#include "mle/ui/element/Types.h"

// I will use lkh as short for Lua Key Handler

namespace mle::ui::element {  // NOLINT
namespace comp {
using CallbackFn = std::function<void(entt::entity)>;
struct Parent {
    entt::entity parent = entt::null;
    static Container& container(entt::entity self);
};
struct Name {
    std::string name;
};
struct RootImage {
    renderer::ImageHnd image_handle{};
    Color clear_color{};

    void apply(entt::entity self, const sol::object& o);
};

struct Background {
    Color color{};

    void render(const RenderContext& ctx) const;

    static renderer::PipelineRef getPipeline();
};
struct Blur {
    renderer::ImageHnd image;

    void render(const RenderContext& ctx);

    static std::pair<renderer::PipelineRef, vk::DescriptorSetLayout> getPipeline();
};
struct Table {
    sol::table v;

    void apply(entt::entity self, const sol::table& tbl);
};
}  // namespace comp
}  // namespace mle::ui::element
