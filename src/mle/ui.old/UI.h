#pragma once

#include <sol/sol.hpp>

#include "Types.h"
#include "mle/common/Color.h"
#include "mle/lua/Lua.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Types.h"
#include "mle/window/Types.h"

namespace mle::ui {
void init(CI ci);
void shutdown();
void switchController();
void update();
void render();

void registerLuaTypes();

vec2i getRootSize();
f32 getRootAR();
renderer::Texture getTexture(const std::string& name);
FontRef getFont(const std::string& name = "");

void setNextController(ControllerHnd&& controller);
void setRootElement(const sol::table& table);

void listenEvent(const std::string& name, ElementRef element);
void unlistenEvent(const std::string& name, ElementRef element);

void addToDeletionQueue(std::function<void(void)>&& fn);

renderer::PipelineRef getSpritePipeline();
renderer::PipelineRef getRectPipeline();
renderer::PipelineRef getTextPipeline();
renderer::PipelineRef getImagePipeline();
vk::WriteDescriptorSet getSpritePipelineD0Write();
vk::WriteDescriptorSet getTextPipelineD0Write();
vk::Sampler getNearestSampler();
renderer::WriteDescriptorSet makeDescriptorWriteForTexture(usize binding, const renderer::Texture& texture);

void _queueUIEvent(std::string&& s, sol::object&& event = {});  // NOLINT
void queueUIEvent(std::string&& s);
template <typename T>
void queueUIEvent(std::string&& s, T&& t) {
    _queueUIEvent(std::move(s), lua::createObject(std::forward<T>(t)));
}
}  // namespace mle::ui
