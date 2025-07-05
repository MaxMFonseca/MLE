#include "CubeMap.h"

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/core/Core.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Utils.h"
#include "mle/renderer/detail/VkContext.h"

namespace mle::renderer {
void CubeMap::init(const std::string& name) {
    MLE_D("Reading cubemap textures for: {}", name);

    std::array<Image::RawData, 6> files;

    files[0] = Image::readFile("res/textures/" + name + "/f.png");
    files[1] = Image::readFile("res/textures/" + name + "/b.png");
    files[2] = Image::readFile("res/textures/" + name + "/u.png");
    files[3] = Image::readFile("res/textures/" + name + "/d.png");
    files[4] = Image::readFile("res/textures/" + name + "/r.png");
    files[5] = Image::readFile("res/textures/" + name + "/l.png");

    MLE_ASSERT_LOG(files.at(0).extent == files.at(1).extent && files.at(0).extent == files.at(2).extent && files.at(0).extent == files.at(3).extent &&
                       files.at(0).extent == files.at(4).extent && files.at(0).extent == files.at(5).extent,
                   "All cubemap faces must have the same size.");
    MLE_ASSERT_LOG(files.at(0).channels == files.at(1).channels && files.at(0).channels == files.at(2).channels &&
                       files.at(0).channels == files.at(3).channels && files.at(0).channels == files.at(4).channels &&
                       files.at(0).channels == files.at(5).channels,
                   "All cubemap faces must have the same channel count.");
    MLE_ASSERT_LOG(files.at(0).channels == 4, "For now I will only support 4 channel images for cubemaps, but I intend to change this in the future.");

    u64 image_size = as<u64>(files.at(0).extent.x * files.at(0).extent.y * files.at(0).channels);

    Buffer::CI staging_ci = {};
    staging_ci.size = image_size * 6;
    staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_ci.allocation_type = Buffer::CI::AllocationType::STAGING;
    auto staging_buffer = Buffer::createHnd(staging_ci);
    for (usize i = 0; i < 6; ++i) {
        staging_buffer->update(files.at(i).pixels.data(), image_size, image_size * i);
    }

    vk::ImageCreateInfo image_ci = {};
    image_ci.imageType = vk::ImageType::e2D;
    image_ci.format = getDefaultColorFormat();
    image_ci.extent.width = files.at(0).extent.x;
    image_ci.extent.height = files.at(0).extent.y;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 6;
    image_ci.samples = vk::SampleCountFlagBits::e1;
    image_ci.tiling = vk::ImageTiling::eOptimal;
    image_ci.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    image_ci.initialLayout = vk::ImageLayout::eUndefined;
    image_ci.sharingMode = vk::SharingMode::eExclusive;
    image_ci.flags = vk::ImageCreateFlagBits::eCubeCompatible;

    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_ci.priority = 1.0F;

    VkImage vk_image = VK_NULL_HANDLE;
    auto create_result = vmaCreateImage(detail::getVma(), rAs<VkImageCreateInfo*>(&image_ci), &alloc_ci, &vk_image, &allocation_, &allocation_info_);
    if (create_result != VK_SUCCESS) {
        core::unrecoverable("Failed to create image: {}", as<vk::Result>(create_result));
    }
    o_ = vk_image;

    auto tcmd = getOTSCmd(CmdType::TRANSFER);
    auto gcmd = getOTSCmd(CmdType::GRAPHICS);

    vk::ImageMemoryBarrier2 barrier = {};
    barrier.image = o_;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

    barrier.srcStageMask = {};
    barrier.srcAccessMask = {};
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

    vk::DependencyInfo dependency_info = {};
    dependency_info.setImageMemoryBarriers(barrier);

    tcmd.pipelineBarrier2(dependency_info);

    for (usize i = 0; i < 6; i++) {
        vk::BufferImageCopy region = {};
        region.bufferOffset = image_size * i;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = vk::Extent3D{image_ci.extent.width, image_ci.extent.height, 1};

        tcmd.copyBufferToImage(staging_buffer->get(), o_, vk::ImageLayout::eTransferDstOptimal, region);
    }

    auto src_family = detail::getVk().getQueueIndex(CmdType::TRANSFER);
    auto dst_family = detail::getVk().getQueueIndex(CmdType::GRAPHICS);

    vk::ImageMemoryBarrier2 barrierft = {};
    barrierft.image = o_;
    barrierft.srcQueueFamilyIndex = src_family;
    barrierft.dstQueueFamilyIndex = dst_family;
    barrierft.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrierft.subresourceRange.baseMipLevel = 0;
    barrierft.subresourceRange.levelCount = 1;
    barrierft.subresourceRange.baseArrayLayer = 0;
    barrierft.subresourceRange.layerCount = 6;
    barrierft.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrierft.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrierft.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    barrierft.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;

    vk::DependencyInfo dep_info = {};
    dep_info.setImageMemoryBarriers(barrier);

    tcmd.pipelineBarrier2(dep_info);
    gcmd.pipelineBarrier2(dep_info);

    vk::ImageMemoryBarrier2 barrierlt = {};
    barrierlt.image = o_;
    barrierlt.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrierlt.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrierlt.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrierlt.subresourceRange.baseMipLevel = 0;
    barrierlt.subresourceRange.levelCount = 1;
    barrierlt.subresourceRange.baseArrayLayer = 0;
    barrierlt.subresourceRange.layerCount = 6;
    barrierlt.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrierlt.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrierlt.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    barrierlt.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;

    vk::DependencyInfo dep_info2 = {};
    dep_info2.setImageMemoryBarriers(barrierlt);

    gcmd.pipelineBarrier2(dep_info2);

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

    renderer::submitOTSAsync(CmdType::GRAPHICS, submit_info, [this, name, buffer = std::move(staging_buffer), semaphore]() mutable {
        MLE_D("CubeMap: {} is ready", name);  // NOLINT
        detail::getDevice().destroy(semaphore);
        ready_ = true;
    });

    vk::ImageViewCreateInfo image_view_ci = {};
    image_view_ci.image = o_;
    image_view_ci.viewType = vk::ImageViewType::eCube;
    image_view_ci.format = image_ci.format;
    image_view_ci.components.r = vk::ComponentSwizzle::eIdentity;
    image_view_ci.components.g = vk::ComponentSwizzle::eIdentity;
    image_view_ci.components.b = vk::ComponentSwizzle::eIdentity;
    image_view_ci.components.a = vk::ComponentSwizzle::eIdentity;
    image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 6;

    view_ = unwrap(detail::getDevice().createImageView(image_view_ci));

    MLE_T("Finished reading cubemap: {}", name);
}

CubeMap::~CubeMap() {
    shutdown();
};

void CubeMap::shutdown() {
    if (view_) {
        detail::getDevice().destroy(view_);
    }
    if (allocation_) {
        vmaDestroyImage(detail::getVma(), o_, allocation_);
    }
};

namespace {
constexpr std::array<u32, 36> CUBE_INDICES{
    0, 1, 2, 2, 1, 3,  // back
    5, 4, 7, 7, 4, 6,  // front
    4, 0, 6, 6, 0, 2,  // left
    1, 5, 3, 3, 5, 7,  // right
    2, 3, 6, 6, 3, 7,  // top
    4, 5, 0, 0, 5, 1   // bottom
};
}  // namespace

int CubeMap::getIndexCount() {
    return CUBE_INDICES.size();
}

BufferRef CubeMap::getIndexBuffer() {
    static BufferHnd buffer = nullptr;
    if (!buffer) {
        Buffer::CI index_buffer_ci = {};
        index_buffer_ci.size = sizeof(CUBE_INDICES);
        index_buffer_ci.usage = vk::BufferUsageFlagBits::eIndexBuffer;
        index_buffer_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
        buffer = Buffer::createHnd(index_buffer_ci);
        buffer->update(CUBE_INDICES.data());
        renderer::addOnShutdown([&]() { buffer.reset(); });
    }
    return buffer.get();
}

vk::DescriptorSetLayout CubeMap::getDescriptorSetLayout() {
    static vk::DescriptorSetLayout dsl = nullptr;
    if (!dsl) {
        vk::DescriptorSetLayoutBinding binding = {};
        binding.setBinding(0);
        binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        binding.setDescriptorCount(1);
        binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);
        auto sampler = renderer::getDefaultSampler();
        binding.setImmutableSamplers(sampler);

        vk::DescriptorSetLayoutCreateInfo dsl_ci = {};
        dsl_ci.setBindings(binding);
        dsl_ci.setFlags(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR);

        dsl = unwrap(detail::getDevice().createDescriptorSetLayout(dsl_ci));
        renderer::addOnShutdown([&]() { detail::getDevice().destroy(dsl); });
    }
    return dsl;
}

PipelineRef CubeMap::getPipeline() {
    static PipelineHnd pipeline = nullptr;
    if (!pipeline) {
        Pipeline::CI pipeline_ci;
        pipeline_ci.vertex_shader = getShader("mle/scene/skybox.vert");
        pipeline_ci.fragment_shader = getShader("mle/scene/skybox.frag");
        pipeline_ci.depth = true;
        pipeline_ci.depth_write = true;
        pipeline_ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
        pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(1, false);
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.descriptor_set_layouts.emplace_back(getDescriptorSetLayout());
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline = Pipeline::createHnd(pipeline_ci);
        renderer::addOnShutdown([&]() { pipeline.reset(); });
    }
    return pipeline.get();
}
}  // namespace mle::renderer
