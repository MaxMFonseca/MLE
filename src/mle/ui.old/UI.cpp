#include "UI.h"

#include <memory>
#include <vulkan/vulkan_structs.hpp>

#include "Controller.h"
#include "element_renderable/Register.inl"
#include "mle/common/Types.h"
#include "mle/common/UnicodeBuffer.h"
#include "mle/common/Utils.h"
#include "mle/lua/Lua.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Events.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/TextureAtlas.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Element.h"
#include "mle/ui/Font.h"
#include "mle/ui/element_renderable/Root.h"
#include "mle/window/Window.h"

namespace mle::ui {
namespace {
struct Data {
    renderer::SwapchainRecreatedListener swapchain_recreated_listener;
    window::WindowResizeListener window_resize_listener;

    ControllerHnd controller{};
    ControllerHnd next_controller{};

    renderer::PipelineRef rect_pipeline_{};
    renderer::PipelineRef sprite_pipeline_{};
    renderer::PipelineRef text_pipeline_{};
    renderer::PipelineRef image_pipeline_{};

    renderer::WriteDescriptorSet sprite_pipeline_d0_write_{};
    renderer::WriteDescriptorSet text_pipeline_d0_write_;

    renderer::TextureAtlasHnd textures{};
    FontManager font_manager;

    vk::Sampler nearest_sampler_;
    vk::Sampler linear_sampler_;

    ElementHnd root_element;
    bool root_dirty = true;
    vec2i root_real_size{};
    f32 root_real_aspect_ratio = 0.0F;

    std::vector<std::pair<std::string, sol::object>> events{};
    std::unordered_map<std::string, std::vector<ElementRef>> event_handlers{};

    std::vector<std::function<void(void)>> update_deletion_queue;
};
std::unique_ptr<Data> d_;  // NOLINT

void loadFont(const sol::table& table) {
    d_->font_manager.addFont(table);
}

void doEvents() {
    for (const auto& [name, event] : d_->events) {
        auto handlers_it = d_->event_handlers.find(name);
        if (handlers_it == d_->event_handlers.end()) {
            MLE_T("No handlers for event '{}'", name);
            continue;
        }
        auto& handlers = handlers_it->second;
        MLE_T("Executing {} handlers for event '{}'", handlers.size(), name);
        for (auto& handler : handlers) {
            handler->callEvent(name, event);
        }
    }
    d_->events.clear();
}

void initRectPipeline() {
    auto pgr = renderer::getPipeline("uiBackgroundRect");
    d_->rect_pipeline_ = pgr.pipeline;
    auto& p = *d_->rect_pipeline_;
    if (pgr.is_new) {
        p.setVertexShader(addShadersBasePath("mle/ui/rect_instanced.vert"));
        p.setFragmentShader(addShadersBasePath("mle/ui/rect_instanced.frag"));
        p.setTopology(vk::PrimitiveTopology::eTriangleStrip);
        p.setPolygonMode(vk::PolygonMode::eFill);
        p.setBlendAttachments(renderer::makeDefaultBlendAttachmentStates(1));
        p.setColorAttachmentFormats({renderer::getDefaultColorFormat()});
    }
}

void initSpritePipeline() {
    auto pgr = renderer::getPipeline("uiSprite");
    d_->sprite_pipeline_ = pgr.pipeline;
    auto& p = *d_->sprite_pipeline_;
    if (pgr.is_new) {
        p.setVertexShader(addShadersBasePath("mle/ui/sprite_texture_array_instanced.vert"));
        p.setFragmentShader(addShadersBasePath("mle/ui/sprite_texture_array_instanced.frag"));
        p.setTopology(vk::PrimitiveTopology::eTriangleStrip);
        p.setPolygonMode(vk::PolygonMode::eFill);
        p.setBlendAttachments(renderer::makeDefaultBlendAttachmentStates(1));
        p.setColorAttachmentFormats({renderer::getDefaultColorFormat()});
    }
}

void initImagePipeline() {
    auto pgr = renderer::getPipeline("uiImage");
    d_->image_pipeline_ = pgr.pipeline;
    auto& p = *d_->image_pipeline_;
    if (pgr.is_new) {
        p.setVertexShader(addShadersBasePath("mle/ui/sprite_instanced.vert"));
        p.setFragmentShader(addShadersBasePath("mle/ui/sprite_instanced.frag"));
        p.setTopology(vk::PrimitiveTopology::eTriangleStrip);
        p.setPolygonMode(vk::PolygonMode::eFill);
        p.setBlendAttachments(renderer::makeDefaultBlendAttachmentStates(1));
        p.setColorAttachmentFormats({renderer::getDefaultColorFormat()});
    }
}

void initTextPipeline() {
    auto pgr = renderer::getPipeline("uiText");
    d_->text_pipeline_ = pgr.pipeline;
    auto& p = *d_->text_pipeline_;
    if (pgr.is_new) {
        p.setVertexShader(addShadersBasePath("mle/ui/sdf_text_instanced.vert"));
        p.setFragmentShader(addShadersBasePath("mle/ui/sdf_text_instanced.frag"));
        p.setTopology(vk::PrimitiveTopology::eTriangleStrip);
        p.setPolygonMode(vk::PolygonMode::eFill);
        p.setBlendAttachments(renderer::makeDefaultBlendAttachmentStates(1));
        p.setColorAttachmentFormats({renderer::getDefaultColorFormat()});
    }
}

void elfWindowResize(UserPtr /*unused*/, const window::events::WindowResize& /*event*/) {
    MLE_D("Window resized");
    d_->root_dirty = true;
}

void createDefaultSampler() {
    MLE_T("Creating default samplers");
    vk::SamplerCreateInfo sampler_ci;
    sampler_ci.magFilter = vk::Filter::eLinear;
    sampler_ci.minFilter = vk::Filter::eLinear;
    sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler_ci.addressModeU = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeV = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeW = vk::SamplerAddressMode::eRepeat;
    sampler_ci.mipLodBias = 0.0F;
    sampler_ci.anisotropyEnable = vk::False;
    sampler_ci.maxAnisotropy = 1.0F;
    sampler_ci.compareEnable = vk::False;
    sampler_ci.compareOp = vk::CompareOp::eAlways;
    sampler_ci.minLod = 0.0F;
    sampler_ci.maxLod = VK_LOD_CLAMP_NONE;
    sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    sampler_ci.unnormalizedCoordinates = vk::False;
    d_->linear_sampler_ = renderer::getVk().getDevice().createSampler(sampler_ci);
    sampler_ci.magFilter = vk::Filter::eNearest;
    sampler_ci.minFilter = vk::Filter::eNearest;
    d_->nearest_sampler_ = renderer::getVk().getDevice().createSampler(sampler_ci);
}

void texturesImageCountChanged() {
    MLE_D("Textures descriptor set changed");
    d_->sprite_pipeline_d0_write_ = d_->textures->makeWriteDescriptorSet(0, d_->nearest_sampler_);
    d_->sprite_pipeline_->setOverrideDescriptorSetLayoutCount(0, d_->sprite_pipeline_d0_write_.image_infos.size());
}

void textImageCountChanged() {
    MLE_D("Text descriptor set changed");
    d_->text_pipeline_d0_write_ = d_->font_manager.getAtlas()->makeWriteDescriptorSet(0, d_->linear_sampler_);
    d_->text_pipeline_->setOverrideDescriptorSetLayoutCount(0, d_->text_pipeline_d0_write_.image_infos.size());
}
}  // namespace

void init(CI ci) {
    MLE_ASSERT(!d_);
    d_ = std::make_unique<Data>();

    renderer::TextureAtlas::GroupCreateInfo default_group_ci;
    default_group_ci.atlas_padding = {2, 2};
    default_group_ci.atlas_extent = {4096, 4096};
    default_group_ci.format = vk::Format::eR8G8B8A8Srgb;
    d_->textures = std::make_unique<renderer::TextureAtlas>(default_group_ci);

    d_->window_resize_listener = window::getED().makeEventListener<window::events::WindowResize>(nullptr, &elfWindowResize);

    createDefaultSampler();

    initRectPipeline();
    initSpritePipeline();
    initImagePipeline();
    initTextPipeline();

    element_renderable::registerRenderableTypes();

    Font::CI digital_disco_ci;
    digital_disco_ci.atlas_chars_per_dim = {10, 10};
    digital_disco_ci.height = 64;
    digital_disco_ci.padding = {2, 2};
    digital_disco_ci.pre_load_string.pushRange(0x21, 0x7E);
    digital_disco_ci.font_name = "DigitalDisco";
    d_->font_manager.addFont(digital_disco_ci, addFontsBasePath("mle/DigitalDisco.ttf"));

    MLE_ASSERT_LOG(ci.init_controller, "init_controller must be set on core::CI");
    d_->next_controller = std::move(ci.init_controller);
    MLE_I("UI module initialized");
}

void shutdown() {
    MLE_I("Shutting down UI module");
    MLE_ASSERT(d_);

    d_->root_element->log();
    d_->root_element.reset();
    d_->controller.reset();

    if (d_->nearest_sampler_) {
        renderer::getVk().getDevice().destroy(d_->nearest_sampler_);
    }
    if (d_->linear_sampler_) {
        renderer::getVk().getDevice().destroy(d_->linear_sampler_);
    }
    d_.reset();
    MLE_I("Finished shutting down UI module");
}

void registerLuaTypes() {
    auto ui_table = lua::createTable("ui");
    ui_table["loadFont"] = &loadFont;
    ui_table["queueEvent"] = &_queueUIEvent;

    Element::registerLuaTypes();

    element_renderable::registerLuaTypes();
}

void update() {
    d_->root_element->checkHover(window::getUIM().getCursorPosNormalized());

    doEvents();

    d_->root_element->update();

    d_->controller->update();

    if (d_->next_controller) {
        switchController();
    }

    if (d_->root_dirty) {
        MLE_I("Root element dirty. Recalculating bounds.");
        d_->root_real_size = window::getSize();
        d_->root_real_aspect_ratio = window::getAspectRatio();

        Element::NewBounds new_bounds{.on_root = {0, 0, 1, 1},
                                      .on_root_clamped = {0, 0, 1, 1},
                                      .aspect_ratio = d_->root_real_aspect_ratio,
                                      .content = {0, 0, 1, 1},
                                      .padding = {0, 0, 1, 1},
                                      .margin = {0, 0, 1, 1}};

        d_->root_element->updateInternalBounds(new_bounds);

        d_->root_element->log();

        d_->root_dirty = false;
    } else {
        d_->root_element->updateInternalBounds();
    }

    for (auto& fn : d_->update_deletion_queue) {
        fn();
    }
    d_->update_deletion_queue.clear();
}

void switchController() {
    MLE_I("Switching controllers");

    renderer::waitIdle();

    if (d_->controller) {
        MLE_T("Unregistering current controller");
        d_->controller->unregisterLua();
        MLE_T("Deleting current controller");
        d_->controller.reset();
    }

    MLE_T("Deleting root element");
    d_->root_element.reset();

    MLE_T("Clenaing events");
    d_->events.clear();
    d_->event_handlers.clear();

    d_->controller = std::move(d_->next_controller);
    MLE_T("Registering new controller");
    d_->controller->registerLua();
    MLE_T("Initializing new controller");
    d_->controller->init();
    MLE_ASSERT_LOG(d_->root_element, "The UI root element must have been created after the controller`s init()");

    d_->next_controller = nullptr;
}

void render() {
    if (auto textures_update_result = d_->textures->update(); textures_update_result == Result::IMAGE_COUNT_CHANGED) {
        texturesImageCountChanged();
    }
    if (auto text_update_result = d_->font_manager.update(); text_update_result == Result::IMAGE_COUNT_CHANGED) {
        textImageCountChanged();
    }

    d_->rect_pipeline_->build();
    d_->sprite_pipeline_->build();
    d_->image_pipeline_->build();
    d_->text_pipeline_->build();

    d_->root_element->render();

    renderer::endFrame(element_renderable::Root::get(d_->root_element.get()).getImage());
}

vec2i getRootSize() {
    return d_->root_real_size;
}

f32 getRootAR() {
    return d_->root_real_aspect_ratio;
}

renderer::Texture getTexture(const std::string& name) {
    return d_->textures->getTexture(name);
}

FontRef getFont(const std::string& name) {
    return d_->font_manager.getFont(name);
}

void _queueUIEvent(std::string&& s, sol::object&& event) {
    d_->events.emplace_back(std::move(s), std::move(event));
}

void queueUIEvent(std::string&& s) {
    _queueUIEvent(std::move(s));
}

void listenEvent(const std::string& name, ElementRef element) {
    auto& handlers = d_->event_handlers[name];
    MLE_ASSERT(std::ranges::find(handlers, element) == handlers.end());
    handlers.emplace_back(element);
}

void unlistenEvent(const std::string& name, ElementRef element) {
    auto& handlers = d_->event_handlers[name];
    std::erase_if(handlers, [&element](const auto& e) { return e == element; });
}

void setNextController(ControllerHnd&& controller) {
    MLE_I("Setting next controller");
    d_->next_controller = std::move(controller);
    MLE_I("Next controller set");
}

void setRootElement(const sol::table& table) {
    auto root_table = lua::createTable(table);
    root_table["renderable"] = "root";
    MLE_T("New root element. Table: {}", root_table);
    d_->root_element = std::make_unique<Element>(root_table);
    d_->root_dirty = true;
}

renderer::PipelineRef getSpritePipeline() {
    return d_->sprite_pipeline_;
}

renderer::PipelineRef getRectPipeline() {
    return d_->rect_pipeline_;
}

renderer::PipelineRef getTextPipeline() {
    return d_->text_pipeline_;
}

renderer::PipelineRef getImagePipeline() {
    return d_->image_pipeline_;
}

vk::WriteDescriptorSet getSpritePipelineD0Write() {
    return d_->sprite_pipeline_d0_write_.write;
}

vk::WriteDescriptorSet getTextPipelineD0Write() {
    return d_->text_pipeline_d0_write_.write;
}

vk::Sampler getNearestSampler() {
    return d_->nearest_sampler_;
}

renderer::WriteDescriptorSet makeDescriptorWriteForTexture(usize binding, const renderer::Texture& texture) {
    return d_->textures->makeWriteDescriptorSet(binding, d_->nearest_sampler_, texture);
}

void addToDeletionQueue(std::function<void(void)>&& fn) {
    d_->update_deletion_queue.emplace_back(std::move(fn));
}
}  // namespace mle::ui
