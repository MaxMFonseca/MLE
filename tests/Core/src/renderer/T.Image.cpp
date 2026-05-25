
#include <gtest/gtest.h>

#include <cstddef>

#include "mle/renderer/Buffer.h"
#include "mle/renderer/CommandManager.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"

using namespace mle;  // NOLINT

TEST(Image, CreateAndDestroy) {
    Image::CI ci{};
    ci.extent = {64, 64};
    ci.format = ImageFormat::TEXTURE_4U;
    ci.extra_usage = vk::ImageUsageFlagBits::eTransferDst;
    auto img = Image::createHnd(ci);
    EXPECT_NE(img->get(), vk::Image{});
    EXPECT_EQ(img->getExtent(), vec2u(64, 64));
}

TEST(Image, DefaultView) {
    Image::CI ci{};
    ci.extent = {32, 32};
    ci.format = ImageFormat::TEXTURE_4U;
    auto img = Image::createHnd(ci);
    auto view = img->getDefaultView();
    EXPECT_NE(view, vk::ImageView{});
}

TEST(Image, CreateView) {
    Image::CI ci{};
    ci.extent = {16, 16};
    ci.format = ImageFormat::TEXTURE_4U;
    auto img = Image::createHnd(ci);
    ImageViewCreateInfo view_ci;
    view_ci.r = vk::ComponentSwizzle::eR;
    auto view = img->createView(view_ci);
    EXPECT_NE(view, vk::ImageView{});
}

TEST(Image, ChannelCount) {
    Image::CI ci{};
    ci.extent = {8, 8};
    ci.format = ImageFormat::TEXTURE_4U;
    auto img = Image::createHnd(ci);
    EXPECT_EQ(img->getChannelCount(), 4);
}

TEST(Image, HdrColorChannelCount) {
    Image::CI ci{};
    ci.extent = {8, 8};
    ci.format = ImageFormat::HDR_COLOR;
    auto img = Image::createHnd(ci);
    EXPECT_EQ(img->getChannelCount(), 4);
}

TEST(Image, AspectRatio) {
    Image::CI ci{};
    ci.extent = {100, 50};
    ci.format = ImageFormat::TEXTURE_4U;
    auto img = Image::createHnd(ci);
    EXPECT_FLOAT_EQ(img->aspect(), 2.0F);
}

TEST(Image, UploadRawData) {
    Image::CI ci{};
    ci.extent = {4, 4};
    ci.format = ImageFormat::TEXTURE_4U;
    ci.extra_usage = vk::ImageUsageFlagBits::eTransferDst;
    auto img = Image::createHnd(ci);

    Image::RawData raw;
    raw.extent = {4, 4};
    raw.channels = 4;
    raw.pixels.resize(4UL * 4 * 4);
    for (usize i = 0; i < raw.pixels.size(); ++i) {
        raw.pixels[i] = as<u8>(i);
    }

    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);
    auto staging = img->copyRaw(cmd, raw);
    cmd_mgr.submitOTSWait(std::move(cmd));

    auto span = std::span<u8>(static_cast<u8*>(staging->map()), raw.pixels.size());
    EXPECT_TRUE(std::ranges::equal(span, raw.pixels));
}

TEST(Image, CopyImage) {
    Image::CI ci_src{};
    ci_src.extent = {8, 8};
    ci_src.format = ImageFormat::TEXTURE_4U;
    ci_src.extra_usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    auto src_img = Image::createHnd(ci_src);

    Image::CI ci_dst = ci_src;
    auto dst_img = Image::createHnd(ci_dst);

    Image::RawData raw;
    raw.extent = {8, 8};
    raw.channels = 4;
    raw.pixels.resize(8UL * 8 * 4);
    for (usize i = 0; i < raw.pixels.size(); ++i) {
        raw.pixels[i] = as<u8>(i);
    }

    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);
    auto staging = src_img->copyRaw(cmd, raw);
    dst_img->copyImage(cmd, *src_img);
    cmd_mgr.submitOTSWait(std::move(cmd));

    auto readback = dst_img->copyToBufferOTS();
    u8* mapped = static_cast<u8*>(readback->map());
    std::span<u8> mapped_span(mapped, raw.pixels.size());
    EXPECT_TRUE(std::ranges::equal(mapped_span, raw.pixels));
}

TEST(Image, BlitImage) {
    Image::CI ci_src{};
    ci_src.extent = {8, 8};
    ci_src.format = ImageFormat::TEXTURE_4U;
    ci_src.extra_usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    auto src_img = Image::createHnd(ci_src);

    Image::CI ci_dst = ci_src;
    ci_dst.extent = {4, 4};
    auto dst_img = Image::createHnd(ci_dst);

    Image::RawData raw;
    raw.extent = {8, 8};
    raw.channels = 4;
    raw.pixels.resize(8UL * 8 * 4);
    for (usize i = 0; i < raw.pixels.size(); ++i) {
        raw.pixels[i] = static_cast<u8>(i);
    }

    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
    auto staging = src_img->copyRaw(cmd, raw);

    Recti src_rect{{0, 0}, {8, 8}};
    Recti dst_rect{{0, 0}, {4, 4}};
    dst_img->blitImage(cmd, *src_img, src_rect, dst_rect);
    cmd_mgr.submitOTSWait(std::move(cmd));

    // Dont know how to check this properly, just make sure we get some data back
    auto readback = dst_img->copyToBufferOTS();
    u8* mapped = as<u8*>(readback->map());
    std::span<u8> mapped_span(mapped, as<usize>(dst_img->getExtent().x * dst_img->getExtent().y * dst_img->getChannelCount()));
    std::array<u8, 4UL * 4 * 4> not_expected{0};
    EXPECT_FALSE(std::ranges::equal(not_expected, raw.pixels));
}

namespace {
void loadAndCompareTest120Aux(Image::RawData& loaded) {
    Image::CI ci{};
    ci.extent = loaded.extent;
    ci.format = ImageFormat::TEXTURE_4U;
    ci.extra_usage = vk::ImageUsageFlagBits::eTransferDst;
    auto img = Image::createHnd(ci);

    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);
    auto staging = img->copyRaw(cmd, loaded);
    cmd_mgr.submitOTSWait(std::move(cmd));

    auto readback = img->copyToBufferOTS();
    u8* mapped = as<u8*>(readback->map());
    std::span<u8> mapped_span(mapped, loaded.pixels.size());

    EXPECT_EQ(loaded.pixels.size(), mapped_span.size());
    EXPECT_TRUE(std::ranges::equal(mapped_span, loaded.pixels));

    EXPECT_EQ(mapped_span[(0 * 4) + 0], 0);
    EXPECT_EQ(mapped_span[(0 * 4) + 1], 120);
    EXPECT_EQ(mapped_span[(0 * 4) + 2], 240);
    EXPECT_EQ(mapped_span[(0 * 4) + 3], 255);

    EXPECT_EQ(mapped_span[(15 * 4) + 0], 0);
    EXPECT_EQ(mapped_span[(150 * 4) + 1], 120);
    EXPECT_EQ(mapped_span[(300 * 4) + 2], 240);
    EXPECT_EQ(mapped_span[(315 * 4) + 3], 255);
}
}  // namespace

TEST(Image, LoadAndCompare120) {
    auto loaded = Image::readFile("res/textures/i/test120.png");
    EXPECT_TRUE(loaded.has_value());
    if (!loaded.has_value()) {
        return;
    }
    loadAndCompareTest120Aux(loaded.value());
}

TEST(Image, LoadAndCompare120c3c) {
    auto loaded = Image::readFile("res/textures/i/test120c3c.png", 4);
    EXPECT_TRUE(loaded.has_value());
    if (!loaded.has_value()) {
        return;
    }
    loadAndCompareTest120Aux(loaded.value());
}
