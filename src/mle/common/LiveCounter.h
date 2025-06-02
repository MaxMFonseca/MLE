/**
 * @file
 * @brief Debug-only base class for tracking live instances of a type.
 */

#pragma once

#include <set>

#include "Logger.h"
#include "mle/common/Utils.h"

namespace mle {

#ifdef MLE_IS_DEBUG_BUILD
/**
 * Adds debug-only instance tracking to a class.
 *
 * Each instantiation of this class tracks its unique ID in a static set.
 * This can be used to detect leaks or debug live instances at runtime.
 *
 * @tparam T The class type to be debug-tracked.
 */
template <typename T>
class LiveCounter {  // NOLINT
  public:
    /// Constructs and registers a new debug instance with a unique ID.
    LiveCounter() :
        debuggable_id_(genID()) {
        MLE_D("Debuggable id {}", debuggable_id_);
        getActiveInstances().insert(debuggable_id_);
    }

    /// Deregisters the debug instance on destruction.
    ~LiveCounter() {
        MLE_T("Destroying debuggable id {}", debuggable_id_);
        getActiveInstances().erase(debuggable_id_);
    }

    /// Logs all currently active instance IDs of the class.
    static void listActiveInstances(std::string type_name) {
        MLE_D("Count of active instances of {}: {}", type_name, getActiveInstances().size());
        for (auto id : getActiveInstances()) {
            MLE_D("  Instance ID: {}", id);
        }
    }

    /// Get a list of all currently active instance IDs.
    static auto& getActiveInstances() {
        static std::set<ID> active_instances;  ///< Set of all currently active instances.
        return active_instances;
    }

  private:
    ID debuggable_id_;  ///< Unique ID for this debug-tracked instance.
};

#else
/**
 * @brief No-op implementation of LiveCounter for release builds.
 */
template <typename T>
class LiveCounter {  // NOLINT
  public:
    /// Constructs a debug instance (no-op in release builds).
    LiveCounter() = default;

    /// Deregisters the debug instance on destruction (no-op in release builds).
    ~LiveCounter() = default;

    /// Logs active instances (no-op in release builds).
    static void listActiveInstances(std::string) {}
};
#endif
}  // namespace mle
