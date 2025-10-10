#include <gtest/gtest.h>

#include "mle/renderer/Buffer.h"
#include "mle/renderer/CommandManager.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"

using namespace mle;

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
    int* mapped = static_cast<int*>(buf->map());
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
    EXPECT_EQ(int_map[0], 1);  // NOLINT
    EXPECT_EQ(int_map[1], 2);  // NOLINT
    EXPECT_EQ(int_map[2], 3);  // NOLINT
    EXPECT_EQ(int_map[3], 4);  // NOLINT
}

TEST(Buffer, OwnershipTransfer) {
    Buffer::CI ci{128, vk::BufferUsageFlagBits::eTransferSrc, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    buf->transferOwnershipOTSWait(GCmdType::TRANSFER);
    buf->transferOwnershipOTSWait(GCmdType::GRAPHICS);
    buf->transferOwnershipOTSWait(GCmdType::COMPUTE);
    EXPECT_EQ(buf->getSize(), 128);
}

TEST(Buffer, DescriptorInfo) {
    Buffer::CI ci{512, vk::BufferUsageFlagBits::eUniformBuffer, false, Buffer::CI::AllocationType::STAGING};
    auto buf = Buffer::createHnd(ci);
    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
    auto info = buf->makeDescriptorInfo(cmd, 128, 0);
    EXPECT_EQ(info.range, 128);
    EXPECT_EQ(info.offset, 0);
    EXPECT_EQ(info.buffer, buf->get());
}
