#pragma once

#include <expected>
#include <string>
#include <utility>

#include "mle/core/Result.h"
#include "mle/utils/Types.h"

/// Disable copy constructor and assignment operator for a class.
#define MLE_NO_COPY(Class)        \
    Class(const Class&) = delete; \
    Class& operator=(const Class&) = delete;

/// Disable move constructor and assignment operator for a class.
#define MLE_NO_MOVE(Class)   \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;

/// Disable both copy and move constructors and assignment operators for a class.
#define MLE_NO_COPY_MOVE(Class) \
    MLE_NO_COPY(Class)          \
    MLE_NO_MOVE(Class)

/// No operation macro
#define MLE_NOOP ((void)0)

/// Defines a singleton pattern for a class.
#define MLE_SINGLETON(Class)                 \
  public:                                    \
    static Class& i() {                      \
        static auto* instance = new Class(); \
        return *instance;                    \
    }                                        \
    MLE_NO_COPY_MOVE(Class)                  \
    ~Class() = default;                      \
                                             \
  private:                                   \
    Class() = default;

namespace mle {
/**
 * @brief Performs a `static_cast` from one type to another.
 *
 * This utility provides a clearer and more concise syntax for static casting,
 * similar to the `as` keyword in other languages (e.g., Rust, C#).
 * I wish C++ had a built-in `as` keyword for this purpose.
 * Imagine... (a as f32) instead of static_cast<f32>(a).
 *
 * @tparam T The target type to cast to.
 * @tparam U The source type.
 * @param value The value to cast.
 * @return The value statically casted to type `T`.
 */
template <typename T, typename U>
constexpr T as(U&& value) {
    return static_cast<T>(std::forward<U>(value));
}

/// Alias for reinterpret_cast.
template <typename T, typename U>
constexpr T rAs(U value) {
    return reinterpret_cast<T>(std::forward<U>(value));
}

/**
 * @brief Converts a raw void pointer to a reference of type `T&`.
 *
 * @tparam T The target reference type.
 * @param value A `void*` pointing to an object of type `T`.
 * @return A reference to the reinterpreted object.
 */
template <typename T>
constexpr T& rVoidAsRef(void* value) {
    return *rAs<T*>(value);
}

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

}  // namespace mle
