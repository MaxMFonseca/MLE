#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "mle/renderer/Renderer.h"
#include "mle/renderer/TextureCache.h"

using namespace mle;  // NOLINT

namespace {
Image::RawData makeRaw(u8 r, u8 g, u8 b, u8 a = 255) {
    Image::RawData data{};
    data.extent = {2, 2};
    data.channels = 4;
    data.srgb = false;
    data.pixels = {
        r, g, b, a, r, g, b, a,
        r, g, b, a, r, g, b, a,
    };
    return data;
}
}  // namespace

TEST(TextureCache, AddTextureWaitReplacesExistingId) {
    auto& cache = Renderer::i().textureCache();
    auto id = entt::hashed_string{"test_texture_cache_replace"};

    auto* first = cache.addTextureWait(id, makeRaw(255, 0, 0));
    ASSERT_NE(first, nullptr);
    EXPECT_TRUE(cache.contains(id));
    ASSERT_TRUE(cache.get(id).has_value());

    auto* second = cache.addTextureWait(id, makeRaw(0, 255, 0));
    ASSERT_NE(second, nullptr);
    EXPECT_TRUE(cache.contains(id));
    ASSERT_TRUE(cache.get(id).has_value());
    EXPECT_EQ(cache.getExtent(id).value(), vec2u(2, 2));
}

TEST(TextureCache, ConcurrentRawUploadsAndCacheQueriesDoNotCrash) {
    auto& cache = Renderer::i().textureCache();
    constexpr int kTextureCount = 32;
    constexpr int kPollCount = 2000;

    std::vector<entt::id_type> ids;
    ids.reserve(kTextureCount);
    for (int i = 0; i < kTextureCount; ++i) {
        auto name = "test_texture_cache_concurrent_" + std::to_string(i);
        auto id = entt::hashed_string{name.c_str()};
        ids.push_back(id);
        cache.addTexture(id, makeRaw(static_cast<u8>(i), 64, 128));
    }

    std::atomic<bool> stop = false;
    std::thread reader([&] {
        for (int poll = 0; poll < kPollCount; ++poll) {
            for (auto id : ids) {
                std::ignore = cache.contains(id);
                std::ignore = cache.get(id);
                std::ignore = cache.getExtent(id);
            }
        }
        stop.store(true, std::memory_order_release);
    });

    while (!stop.load(std::memory_order_acquire)) {
        for (auto id : ids) {
            std::ignore = cache.contains(id);
        }
        std::this_thread::yield();
    }

    reader.join();

    bool all_ready = false;
    for (int attempt = 0; attempt < 200 && !all_ready; ++attempt) {
        all_ready = true;
        for (auto id : ids) {
            if (!cache.get(id).has_value()) {
                all_ready = false;
                break;
            }
        }
        if (!all_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    if (!all_ready) {
        Renderer::i().commandManager().waitDeviceIdle();
        for (int attempt = 0; attempt < 200 && !all_ready; ++attempt) {
            all_ready = true;
            for (auto id : ids) {
                if (!cache.get(id).has_value()) {
                    all_ready = false;
                    break;
                }
            }
            if (!all_ready) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }

    EXPECT_TRUE(all_ready);
}
