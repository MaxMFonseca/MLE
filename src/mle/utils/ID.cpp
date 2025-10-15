#include "ID.h"

#include <atomic>

namespace mle {
ID genID() {
    static std::atomic<ID> current_id = 0;
    return current_id.fetch_add(1, std::memory_order_relaxed);
};
}  // namespace mle
