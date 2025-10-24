#pragma once

#include <array>
#include <atomic>

#include "mle/utils/Utils.h"

namespace mle {
template <typename T>
class AtomicTripleBuffer {
  public:
    AtomicTripleBuffer() = default;
    ~AtomicTripleBuffer() = default;

    MLE_NO_COPY_MOVE(AtomicTripleBuffer);

    T& producerAcquire() {
        const auto cons_using = consumer_idx_.load(std::memory_order_acquire);
        const auto pub = loadPub(std::memory_order_acquire);
        const auto last_published = pub.idx;

        for (u8 i = 0; i < 3; ++i) {
            if (i != cons_using && i != last_published) {
                prod_staging_idx_ = i;
                return buf_.at(i);
            }
        }
        prod_staging_idx_ = as<u8>((last_published + 1) % 3);
        return buf_[prod_staging_idx_];
    }

    void producerPublish() {
        ++prod_seq_;
        storePub({prod_seq_, prod_staging_idx_}, std::memory_order_release);
    }

    [[nodiscard]] bool consumerHasNew() const {
        const Pub pub = loadPub(std::memory_order_acquire);
        return pub.seq != cons_seen_seq_;
    }

    T& consumerGetLatest() {
        const Pub pub = loadPub(std::memory_order_acquire);

        cons_seen_seq_ = pub.seq;
        cons_current_idx_ = pub.idx;

        consumer_idx_.store(cons_current_idx_, std::memory_order_release);

        return buf_[cons_current_idx_];
    }

    auto& getUnsafe(u8 idx) { return buf_.at(idx); }

  private:
    struct Pub {
        u64 seq{0};
        u8 idx{0xFF};
    };

    u64 pack(Pub p) const { return (p.seq << 8) | p.idx; }
    Pub unpack(u64 packed) const {
        Pub p;
        p.seq = packed >> 8U;
        p.idx = as<u8>(packed & 0xFFU);
        return p;
    }

    Pub loadPub(std::memory_order mo) const { return unpack(published_.load(mo)); }
    void storePub(Pub p, std::memory_order mo) { published_.store(pack(p), mo); }

  private:
    std::array<T, 3> buf_;

    std::atomic<u8> consumer_idx_{0xFF};

    std::atomic<u64> published_{pack(Pub{})};

    // Producer thread-local state
    u8 prod_staging_idx_{0xFF};
    u64 prod_seq_{0};

    // Consumer thread-local state
    u8 cons_current_idx_{0xFF};
    u64 cons_seen_seq_{0};
};
}  // namespace mle
