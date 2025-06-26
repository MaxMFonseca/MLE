#include "TextureCache.h"

#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Assert.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer::detail {
void TextureCache::init() {  // NOLINT
    MLE_I("Initializing texture cache");

    vk::DescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = vk::DescriptorType::eSampledImage;
    binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    binding.descriptorCount = 500000;
    vk::DescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.bindingCount = 1;
    dsl_ci.setBindings(binding);
    dsl_ci.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

    vk::DescriptorBindingFlags flags = vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::eUpdateAfterBind |
                                       vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
    vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_ci;
    binding_flags_ci.bindingCount = 1;
    binding_flags_ci.pBindingFlags = &flags;
    dsl_ci.pNext = &binding_flags_ci;

    descriptor_set_layout_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorSetLayout(dsl_ci));

    vk::DescriptorPoolSize pool_size;
    pool_size.type = vk::DescriptorType::eSampledImage;
    pool_size.descriptorCount = 1024;

    vk::DescriptorPoolCreateInfo pool_ci;
    pool_ci.setPoolSizes(pool_size);
    pool_ci.maxSets = 2;
    pool_ci.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;

    descriptor_pool_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorPool(pool_ci));

    vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_info;
    variable_info.descriptorSetCount = 1;
    variable_info.pDescriptorCounts = &pool_size.descriptorCount;

    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info.pNext = &variable_info;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.setSetLayouts(descriptor_set_layout_);

    dset_ = renderer::unwrap(renderer::detail::getDevice().allocateDescriptorSets(alloc_info)).at(0);
}

void TextureCache::reset() {
    MLE_I("Shutting down texture cache");
    textures_.clear();
    getDevice().destroy(descriptor_pool_);
    getDevice().destroy(descriptor_set_layout_);
    dset_ = nullptr;
    descriptor_pool_ = nullptr;
    descriptor_set_layout_ = nullptr;
}

void TextureCache::update() {  // NOLINT
}

void TextureCache::updateImagesOnFrame() {
    if (update_images_on_frame_.empty()) {
        return;
    }

    auto cmd = renderer::detail::getFramePrimaryCmd();

    std::ranges::sort(update_images_on_frame_, [](const auto& a, const auto& b) { return a.idx < b.idx; });

    u32 idx = max<u32>();
    ImageRef img = nullptr;
    for (auto& i : update_images_on_frame_) {
        if (idx != i.idx) {
            if (img) {
                img->transitionState(cmd, Image::State::SHADER_READ);
            }
            img = getImage(i.idx);
            idx = i.idx;
        }

        MLE_T("Updating image on frame: idx={}, area={}", i.idx, i.area);

        img->update(cmd, i.buffer.get(), i.area);

        renderer::deleteAfterFrame(std::move(i.buffer));
    }
    if (img) {
        img->transitionState(cmd, Image::State::SHADER_READ);
    }

    update_images_on_frame_.clear();
}

void TextureCache::frameBegin() {
    updateImagesOnFrame();
}

ImageRef TextureCache::getImage(u32 idx) {
    MLE_ASSERT_LOG(idx < textures_.size(), "Texture index out of bounds: {}", idx);
    return textures_.at(idx).image.get();
}

Texture TextureCache::add(std::string name) {
    MLE_D("Adding texture: {}", name);

    MLE_ASSERT(!texture_names_.contains(name));

    auto file_data = Image::readFile("res/textures/" + name);

    u32 idx = 0;
    if (!free_indices_.empty()) {
        idx = free_indices_.front();
        free_indices_.pop_front();
    } else {
        idx = static_cast<u32>(textures_.size());
        textures_.emplace_back();
    }

    texture_names_[name] = idx;
    auto& td = textures_[idx];

    Image::CI ci;
    ci.extent = file_data.extent;
    ci.format = Image::getDefaultFormatForChannelCount(file_data.channels);
    ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;

    td = {.image = Image::createHnd(ci), .ready = false};

    auto buf = Image::createStagingBuffer(file_data);

    auto tcmd = renderer::getOTSCmd(CmdType::TRANSFER);
    auto gcmd = renderer::getOTSCmd(CmdType::GRAPHICS);

    td.image->update(tcmd, buf.get());
    td.image->changeOwnerQueue(CmdType::TRANSFER, tcmd, CmdType::GRAPHICS, gcmd);
    td.image->transitionState(gcmd, Image::State::SHADER_READ);

    auto semaphore = unwrap(renderer::detail::getDevice().createSemaphore({}));

    vk::SemaphoreSubmitInfo semaphore_info = {};
    semaphore_info.setSemaphore(semaphore);
    semaphore_info.setStageMask(vk::PipelineStageFlagBits2::eTransfer);
    vk::CommandBufferSubmitInfo cmd_info = {};
    cmd_info.setCommandBuffer(tcmd);
    vk::SubmitInfo2 submit_info = {};
    submit_info.setCommandBufferInfos(cmd_info);
    submit_info.setSignalSemaphoreInfos(semaphore_info);

    renderer::submitOTSAsync(CmdType::TRANSFER, submit_info);

    cmd_info.setCommandBuffer(gcmd);
    submit_info.setSignalSemaphoreInfos({});
    submit_info.setWaitSemaphoreInfos(semaphore_info);

    updating_textures_.emplace_back(UpdatingData{
        .staging_buffer = std::move(buf),
        .semaphore = semaphore,
        .idx = idx,
    });

    renderer::submitOTSAsync(CmdType::GRAPHICS, submit_info, [idx, this]() { finishedUpload(idx); });  // NOLINT

    MLE_D("Texture: {} is: {}", name, idx);

    return {.image = td.image.get(), .idx = idx, .ready = false};
}

Texture TextureCache::add(const std::string& name, ImageHnd&& img) {
    MLE_D("Adding texture from ImageHnd: {}", name);

    MLE_ASSERT(!texture_names_.contains(name));

    u32 idx = 0;
    if (!free_indices_.empty()) {
        idx = free_indices_.front();
        free_indices_.pop_front();
    } else {
        idx = static_cast<u32>(textures_.size());
        textures_.emplace_back();
    }

    texture_names_[name] = idx;
    auto& td = textures_[idx];

    td.image = std::move(img);
    td.ready = true;

    MLE_D("Texture: {} is: {}", name, idx);

    write(idx);

    return {.image = td.image.get(), .idx = idx, .ready = true};
}

void TextureCache::finishedUpload(u32 idx) {
    MLE_D("Finished uploading texture with index: {}", idx);

    auto it = std::ranges::find_if(updating_textures_, [idx](const UpdatingData& data) { return data.idx == idx; });
    MLE_ASSERT_LOG(it != updating_textures_.end(), "Texture with index {} not found in updating textures", idx);

    textures_.at(it->idx).ready = true;

    getDevice().destroy(it->semaphore);

    write(it->idx);

    updating_textures_.erase(it);
}

Texture TextureCache::get(const std::string& name) {
    auto it = texture_names_.find(name);
    if (it != texture_names_.end()) {
        return {.image = textures_[it->second].image.get(), .idx = it->second, .ready = textures_[it->second].ready};
    }

    MLE_D("Texture '{}' not found in cache, attempting to load", name);
    return add(name);
}

void TextureCache::write(u32 idx) {
    auto& texture = textures_.at(idx);

    vk::DescriptorImageInfo image_info;
    image_info.imageView = texture.image->getDefaultView();
    image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::WriteDescriptorSet write;
    write.setDstSet(dset_);
    write.setDstArrayElement(idx);
    write.setDescriptorType(vk::DescriptorType::eSampledImage);
    write.setDstBinding(0);
    write.setImageInfo(image_info);

    getDevice().updateDescriptorSets(write, nullptr);
}

void TextureCache::bindTexturesDSet(RenderingThread& thread) {
    thread.bindDescriptorSet(dset_, 0);
}
}  // namespace mle::renderer::detail
