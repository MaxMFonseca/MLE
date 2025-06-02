#pragma once

#include <map>
#include <memory>

#include "mle/common/Color.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/renderer/Types.h"
#include "sol/table.hpp"
#include "spdlog/fmt/bundled/core.h"

namespace mle::ui {
class Element;
using ElementHnd = std::unique_ptr<Element>;
using ElementRef = Element*;

class Font;
using FontHnd = std::unique_ptr<Font>;
using FontRef = Font*;

class Controller;
using ControllerHnd = std::unique_ptr<Controller>;
using ControllerRef = Controller*;

namespace element_renderable {
class Shader;
using ShaderHnd = std::unique_ptr<Shader>;
using ShaderRef = Shader*;
using ShaderConstRef = const Shader*;
}  // namespace element_renderable

struct CreateInfo {
    sol::table table;
    ControllerHnd init_controller;
};
using CI = CreateInfo;

struct RectInstance {
    vec2f pos;
    vec2f size;
    Color color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct SpriteInstance {
    vec2f pos;
    vec2f size;
    vec4f color = {1.0F, 1.0F, 1.0F, 1.0F};
    vec2f texture_offset;
    vec2f texture_size;
    u32 texture_idx;
};

struct ImageInstance {
    vec2f pos;
    vec2f size;
    vec2f texture_offset;
    vec2f texture_size;
    Color color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct TextInstance {
    vec2f pos;
    vec2f size;
    vec4f color = {1.0F, 1.0F, 1.0F, 1.0F};
    vec4f outline_color = {0.0F, 0.0F, 0.0F, 1.0F};
    vec2f texture_offset;
    vec2f texture_size;
    u32 texture_idx;
    float outline_thickness = 0.0F;
};

struct RenderableLayerData {
    std::vector<RectInstance> rects;
    std::vector<SpriteInstance> sprites;
    std::vector<TextInstance> texts;
    std::vector<std::pair<ImageInstance, renderer::ImageRef>> images;
    std::vector<element_renderable::ShaderConstRef> shaders;
};
using RenderableData = std::map<int, RenderableLayerData>;
}  // namespace mle::ui

namespace fmt {
template <>
struct formatter<mle::ui::RectInstance> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::RectInstance& rect, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos: {}, size: {}, color: {}, layer: {}]", rect.pos, rect.size, rect.color);
    }
};

template <>
struct formatter<mle::ui::SpriteInstance> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::SpriteInstance& sprite, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos: {}, size: {}, color: {}, texture_offset: {}, texture_size: {}, texture_idx: {}, layer: {}]", sprite.pos, sprite.size,
                         sprite.color, sprite.texture_offset, sprite.texture_size, sprite.texture_idx);
    }
};

template <>
struct formatter<mle::ui::ImageInstance> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::ImageInstance& img, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos: {}, size: {}, texture_offset: {}, texture_size: {}, layer: {}]", img.pos, img.size, img.texture_offset,
                         img.texture_size);
    }
};

template <>
struct formatter<mle::ui::TextInstance> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::TextInstance& text, FormatContext& ctx) const {
        return format_to(
            ctx.out(),
            "[pos: {}, size: {}, color: {}, outline_color: {}, texture_offset: {}, texture_size: {}, texture_idx: {}, outline_thickness: {}, layer: {}]",
            text.pos, text.size, text.color, text.outline_color, text.texture_offset, text.texture_size, text.texture_idx, text.outline_thickness);
    }
};
}  // namespace fmt
