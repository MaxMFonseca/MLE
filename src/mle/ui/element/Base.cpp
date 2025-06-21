#include "Base.h"

#include "Container.h"
#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/LuaKeyHandlers.h"
#include "mle/ui/element/Renderable.h"
#include "sol/forward.hpp"

namespace mle::ui::element {
void applyTable(entt::entity e, const sol::table& table, entt::entity parent) {
    auto& reg = getRegistry();
    if (parent != entt::null) {
        reg.emplace_or_replace<comp::Parent>(e, parent);
    }

    std::string name;
    if (lua::tryGetIdx(table, 1, name)) {
        reg.emplace_or_replace<comp::Name>(e, std::move(name));
    }

    auto* bounds_c = reg.try_get<comp::Bounds>(e);
    if (!bounds_c) {
        bounds_c = &reg.emplace<comp::Bounds>(e);
    }
    const auto bounds_r = table["bounds"];
    if (bounds_r.valid()) {
        bounds_c->immutable = true;
        bounds_c->bounds = lua::as<Recti>(bounds_r);
    }

    for (auto& [key, value] : table) {
        std::string key_str;
        if (!lua::tryAs(key, key_str)) {
            continue;
        }
        auto fn_r = getLuaKeyHandlerFn(key_str);
        if (!fn_r) {
            continue;
        }
        fn_r.value()(e, value);
    }
}

bool isFitX(entt::entity e) {
    if (getRegistry().get<comp::Bounds>(e).immutable) {
        return false;
    }
    const auto* target_size = getRegistry().try_get<comp::TargetSize>(e);
    if (target_size) {
        return target_size->x.type == comp::TargetBound::Type::FIT;
    }
    return true;
}

bool isFitY(entt::entity e) {
    if (getRegistry().get<comp::Bounds>(e).immutable) {
        return false;
    }
    const auto* target_size = getRegistry().try_get<comp::TargetSize>(e);
    if (target_size) {
        return target_size->y.type == comp::TargetBound::Type::FIT;
    }
    return true;
}

bool isFit(entt::entity e) {
    if (getRegistry().get<comp::Bounds>(e).immutable) {
        return false;
    }
    const auto* target_size = getRegistry().try_get<comp::TargetSize>(e);
    if (target_size) {
        return target_size->x.type == comp::TargetBound::Type::FIT || target_size->y.type == comp::TargetBound::Type::FIT;
    }
    return true;
}

// entt::entity findFirstFixedSizedX(entt::entity e) {  // NOLINT
//     if (has<comp::StaticBounds>(e)) {
//         return e;
//     }
//     if (auto* target_size = getRegistry().try_get<comp::TargetSize>(e)) {
//         if (!target_size->isFitX()) {
//             return e;
//         }
//     }
//     return findFirstFixedSizedX(getRegistry().get<comp::Parent>(e).parent);
// }
//
// entt::entity findFirstFixedSizedY(entt::entity e) {  // NOLINT
//     if (has<comp::StaticBounds>(e)) {
//         return e;
//     }
//     if (auto* target_size = getRegistry().try_get<comp::TargetSize>(e)) {
//         if (!target_size->isFitY()) {
//             return e;
//         }
//     }
//     return findFirstFixedSizedY(getRegistry().get<comp::Parent>(e).parent);
// }
//

namespace comp {
namespace {
TargetBound::Type stringToType(const std::string& str) {
    using Type = TargetBound::Type;
    if (str == "px") {
        return Type::PX;
    }
    if (str == "w" || str == "fit") {
        return Type::FIT;
    }
    if (str == "%") {
        return Type::PARENT;
    }
    if (str == "s") {
        return Type::SELF;
    }
    if (str == "sw") {
        return Type::SELF_W;
    }
    if (str == "sh") {
        return Type::SELF_H;
    }
    if (str == "ppx") {
        return Type::PARENT_PX;
    }
    if (str == "r%") {
        return Type::ROOT;
    }
    if (str == "rpx") {
        return Type::ROOT_PX;
    }
    if (str == "f" || str == "flex") {
        return Type::FLEX_SHARE;
    }
    if (str == "pw") {
        return Type::PARENT_W;
    }
    if (str == "ph") {
        return Type::PARENT_H;
    }
    return Type::DEFAULT;
}
}  // namespace

void TargetBound::fromString(const std::string& str) {
    auto split = splitNumberAndSuffix<f32>(str);
    MLE_ASSERT_LOG(split.has_value(), "Invalid target bound format: {}", str);
    val = split.value().first;
    if (val == 0.0F && str[0] != '0') {
        val = 1;
    }
    const auto& suffix = split.value().second;
    type = stringToType(suffix);
    if (typeIsPercentage(type)) {
        val /= 100.0F;
    }
}

void TargetBound::fromLua(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        val = obj.as<f32>();
        return;
    }
    if (obj.is<std::string>()) {
        fromString(obj.as<std::string>());
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        auto val_r = table[1];
        MLE_ASSERT(lua::valid<f32>(val_r));
        val = val_r.get<f32>();
        auto type_r = table[2];
        if (type_r.valid() && type_r.is<std::string>()) {
            type = stringToType(type_r.get<std::string>());
        }
        if (typeIsPercentage(type)) {
            val /= 100.0F;
        }
        return;
    }
    MLE_UNREACHABLE_LOG("Unexpected obj type for TargetBound: {}", obj.get_type());
}

Container& Parent::container(entt::entity self) {
    auto& reg = getRegistry();
    return reg.get<Container>(reg.get<Parent>(self).parent);
}

renderer::PipelineRef Background::getPipeline() {
    static renderer::PipelineHnd pipeline;
    if (!pipeline) {
        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("ui/rect.vert", true);
        ci.fragment_shader = renderer::getShader("ui/rect.frag", true);
        ci.color_attachment_formats = {renderer::getDefaultColorFormat()};
        ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
        ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline = renderer::Pipeline::createHnd(ci);
        renderer::addOnShutdown([&]() { pipeline.reset(); });
    }
    return pipeline.get();
}

void TargetPadding::addToRect(entt::entity self, Recti& rect) {
    auto& reg = getRegistry();
    auto* padding = reg.try_get<TargetPadding>(self);
    if (!padding) {
        return;
    }

    auto width = as<f32>(rect.size.x);
    auto height = as<f32>(rect.size.y);

    if (padding->l.val != 0.0F) {
        int val = 0;
        switch (padding->l.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(padding->l.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(padding->l.val * width);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", padding->l.type);
        }
        rect.pos.x += val;
        rect.size.x -= val;
    }
    if (padding->r.val != 0.0F) {
        int val = 0;
        switch (padding->r.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(padding->r.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(padding->r.val * width);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", padding->r.type);
        }
        rect.size.x -= val;
    }
    if (padding->t.val != 0.0F) {
        int val = 0;
        switch (padding->t.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(padding->t.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(padding->t.val * height);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", padding->t.type);
        }
        rect.pos.y += val;
        rect.size.y -= val;
    }
    if (padding->b.val != 0.0F) {
        int val = 0;
        switch (padding->b.type) {
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::PX: {
                val = as<int>(padding->b.val);
            } break;
            case TargetBound::Type::SELF: {
                val = as<int>(padding->b.val * height);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", padding->b.type);
        }
        rect.size.y -= val;
    }
}
void Background::render(const RenderContext& ctx) const {
    ctx.thread->setPipeline(Background::getPipeline());

    struct {
        vec4f color{};
    } pc;

    pc.color = color;

    ctx.thread->pushConstants(&pc);
    ctx.thread->draw(1, 4);
}

void Blur::render(const RenderContext& ctx) const {
    ctx.thread->endRendering();

    auto& reg = getRegistry();
    auto& bounds = reg.get<comp::Bounds>(ctx.self);

    renderer::Image::CI copy_ci;
    copy_ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    copy_ci.extent = {bounds.bounds.size.x, bounds.bounds.size.y};
    copy_ci.format = ctx.root_image->getFormat();
    auto image_copy = renderer::Image::createHnd(copy_ci);

    image_copy->updateCopy(ctx.thread->cmd(), ctx.root_image, ctx.viewport.asI32());

    image_copy->transitionState(ctx.thread->cmd(), renderer::Image::State::SHADER_READ);

    ctx.thread->beginRendering({}, false);

    vk::DescriptorImageInfo image_info;
    image_info.imageLayout = image_copy->getCurrentLayout();
    image_info.imageView = image_copy->getDefaultView();
    vk::WriteDescriptorSet write0;
    write0.descriptorCount = 1;
    write0.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write0.dstBinding = 0;
    write0.setImageInfo(image_info);

    ctx.thread->setPipeline(getPipeline().first);

    ctx.thread->pushDescriptor(0, {write0});

    ctx.thread->setViewport(ctx.viewport);

    ctx.thread->draw(1, 4);

    renderer::deleteAfterFrame(std::move(image_copy));
}

std::pair<renderer::PipelineRef, vk::DescriptorSetLayout> Blur::getPipeline() {
    static renderer::PipelineHnd pipeline;
    static vk::DescriptorSetLayout dsl;
    static vk::Sampler sampler;

    if (!pipeline) {
        MLE_T("Creating blur pipeline");

        MLE_T("  sampler");
        vk::SamplerCreateInfo sampler_ci;
        sampler_ci.magFilter = vk::Filter::eLinear;
        sampler_ci.minFilter = vk::Filter::eLinear;
        sampler_ci.addressModeU = vk::SamplerAddressMode::eMirroredRepeat;
        sampler_ci.addressModeV = vk::SamplerAddressMode::eMirroredRepeat;
        sampler_ci.addressModeW = vk::SamplerAddressMode::eMirroredRepeat;
        sampler_ci.anisotropyEnable = VK_FALSE;
        sampler_ci.maxAnisotropy = 1.0F;
        sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = vk::CompareOp::eAlways;
        sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler_ci.minLod = 0.0F;
        sampler_ci.maxLod = 0.0F;
        sampler_ci.mipLodBias = 0.0F;

        // TODO: move this out
        sampler = renderer::unwrap(renderer::detail::getDevice().createSampler(sampler_ci));

        MLE_T("  descriptor set layout");
        vk::DescriptorSetLayoutBinding binding;
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding.pImmutableSamplers = &sampler;

        vk::DescriptorSetLayoutCreateInfo dsl_ci;
        dsl_ci.bindingCount = 1;
        dsl_ci.flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;
        dsl_ci.pBindings = &binding;

        dsl = renderer::unwrap(renderer::detail::getDevice().createDescriptorSetLayout(dsl_ci));

        MLE_T("  pipeline");
        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("ui/quad.vert", true);
        ci.fragment_shader = renderer::getShader("ui/blur.frag", true);
        ci.color_attachment_formats = {renderer::getDefaultColorFormat()};
        ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
        ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        ci.descriptor_set_layouts.emplace_back(dsl);
        pipeline = renderer::Pipeline::createHnd(ci);

        renderer::addOnShutdown([&]() {
            pipeline.reset();
            renderer::destroy(dsl);
            renderer::destroy(sampler);
        });
    }

    return std::make_pair(pipeline.get(), dsl);
}
}  // namespace comp
}  // namespace mle::ui::element
