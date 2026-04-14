#include <gtest/gtest.h>

#include <cstddef>

#include "mle/renderer/Buffer.h"
#include "mle/renderer/CommandManager.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"

using namespace mle;  // NOLINT

TEST(Buffer, CreateAndDestroy) {
    Buffer::CI ci{1024, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::GPU_ONLY};
    auto buf = Buffer::createHnd(ci);
    EXPECT_NE(buf->get(), vk::Buffer{});
    EXPECT_EQ(buf->getSize(), 1024);
}

TEST(Buffer, MapAndWrite) {
    Buffer::CI ci{128, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    int data = 42;
    buf->write(&data, sizeof(data), 0);
    int* mapped = as<int*>(buf->map());
    EXPECT_EQ(*mapped, 42);
}

TEST(Buffer, WriteStaged) {
    Buffer::CI ci{256, vk::BufferUsageFlagBits::eTransferDst, false, Buffer::CI::AllocationType::GPU_ONLY_HOST_READ};
    auto buf = Buffer::createHnd(ci);
    std::array<int, 4> arr = {1, 2, 3, 4};
    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);
    auto staging = buf->writeStaged(cmd, arr.data(), arr.size() * sizeof(int));
    cmd_mgr.submitOTSWait(std::move(cmd));
    void* buf_map = buf->map();
    int* int_map = as<int*>(buf_map);
    std::span<int> int_span(int_map, 4);
    EXPECT_TRUE(std::ranges::equal(int_span, arr));
}

TEST(Buffer, OwnershipTransfer) {
    Buffer::CI ci{128, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    buf->ownershipReleaseOTSAcquireOTSWait(GCmdType::TRANSFER);
    buf->ownershipReleaseOTSAcquireOTSWait(GCmdType::GRAPHICS);
    buf->ownershipReleaseOTSAcquireOTSWait(GCmdType::COMPUTE);
    EXPECT_EQ(buf->getSize(), 128);
}

TEST(Buffer, DescriptorInfo) {
    Buffer::CI ci{512, vk::BufferUsageFlagBits::eUniformBuffer, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
    auto info = buf->makeDescriptorInfo(cmd, 128, 0);
    cmd_mgr.submitOTSWait(std::move(cmd));
    EXPECT_EQ(info.range, 128);
    EXPECT_EQ(info.offset, 0);
    EXPECT_EQ(info.buffer, buf->get());
}

TEST(Buffer, CopyBetweenBuffers) {
    Buffer::CI ci_src{64, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    Buffer::CI ci_dst{64, vk::BufferUsageFlagBits::eTransferDst, false, Buffer::CI::AllocationType::GPU_ONLY_HOST_READ};
    auto src = Buffer::createHnd(ci_src);
    auto dst = Buffer::createHnd(ci_dst);

    std::array<int, 4> arr = {10, 20, 30, 40};
    src->write(arr.data(), arr.size() * sizeof(int));
    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);
    dst->copy(cmd, src.get(), arr.size() * sizeof(int));
    cmd_mgr.submitOTSWait(std::move(cmd));
    int* int_map = as<int*>(dst->map());
    std::span<int> int_span(int_map, 4);
    EXPECT_TRUE(std::ranges::equal(int_span, arr));
}

TEST(Buffer, LargeBufferAllocation) {
    auto buf_size = 1024U * 1024;
    Buffer::CI ci{buf_size, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    std::vector<u8> data(buf_size, 0xAB);
    buf->write(data.data(), buf_size);
    u8* mapped = as<u8*>(buf->map());
    std::span<u8> mapped_span(mapped, buf_size);
    EXPECT_TRUE(std::ranges::equal(mapped_span, data));
}

TEST(Buffer, PersistentMapping) {
    Buffer::CI ci{128, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    int* mapped = as<int*>(buf->map());
    mapped[0] = 123;            // NOLINT
    mapped[1] = 456;            // NOLINT
    EXPECT_EQ(mapped[0], 123);  // NOLINT
    EXPECT_EQ(mapped[1], 456);  // NOLINT
}

TEST(Buffer, MultipleWrites) {
    Buffer::CI ci{32, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    int a = 111, b = 222;
    buf->write(&a, sizeof(a), 0);
    buf->write(&b, sizeof(b), sizeof(a));
    int* mapped = as<int*>(buf->map());
    EXPECT_EQ(mapped[0], 111);  // NOLINT
    EXPECT_EQ(mapped[1], 222);  // NOLINT
}
