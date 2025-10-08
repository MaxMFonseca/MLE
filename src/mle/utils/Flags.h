/**
 * @file Flags.h
 * @brief Type-safe wrapper for enum-based bit flags.
 *
 * This header defines the mle::Flags<T> template, which wraps scoped enum classes
 * to provide safe and expressive bitmask operations. It also provides macro helpers
 * for declaring flag sets and formatting them with the fmt library.
 *
 * Example usage:
 * @code
 * MLE_FLAGS_BEGIN(RenderState, u32)
 * MLE_FLAG(RenderState, Visible)
 * MLE_FLAG(RenderState, Transparent)
 * MLE_FLAG(RenderState, CastsShadow)
 * MLE_FLAGS_END(RenderState)
 *
 * MLE_FLAGS_FMT_BEGIN(RenderState)
 * MLE_FLAGS_FMT_CASE(RenderState, Visible)
 * MLE_FLAGS_FMT_CASE(RenderState, Transparent)
 * MLE_FLAGS_FMT_CASE(RenderState, CastsShadow)
 * MLE_FLAGS_FMT_END(RenderState)
 *
 * void draw(RenderStateFlags state) {
 *     if (state.have(RenderStateFlagBits::Visible)) {
 *         // Draw the object
 *     }
 * }
 *
 * RenderStateFlags flags = RenderStateFlagBits::Visible | RenderStateFlagBits::CastsShadow;
 * LOG_I(flags)
 * @endcode
 */

#pragma once

#include <type_traits>

#include "mle/core/Assert.h"

namespace mle {

/**
 * @brief A strongly-typed flag wrapper for scoped enums.
 *
 * Wraps enum class values and provides bitmask operations in a type-safe way.
 *
 * @tparam BitType An enum class type used to represent individual flag bits.
 */
template <typename BitType>
class Flags {
  public:
    /**
     * @brief The underlying integer type used for the bitmask.
     */
    using MaskType = std::underlying_type_t<BitType>;

    /**
     * @brief Default constructor, initializes with no bits set.
     */
    constexpr Flags() noexcept :
        mask_(0) {}

    /**
     * @brief Constructs from a single enum bit.
     *
     * Implicit on purpose to allow easy flag construction using enum constants.
     *
     * @param bit The enum bit to initialize the mask with.
     */
    constexpr Flags(BitType bit) noexcept :  // NOLINT(google-explicit-constructor, hicpp-explicit-conversions)
        mask_(static_cast<MaskType>(bit)) {}

    /**
     * @brief Constructs from a raw bitmask value.
     *
     * @param flags A mask of underlying bits.
     */
    constexpr explicit Flags(MaskType flags) noexcept :
        mask_(flags) {}

    constexpr Flags(const Flags<BitType>& rhs) noexcept = default;
    constexpr Flags(Flags<BitType>&& rhs) noexcept = default;
    constexpr Flags<BitType>& operator=(const Flags<BitType>& rhs) noexcept = default;
    constexpr Flags<BitType>& operator=(Flags<BitType>&& rhs) noexcept = default;

    ~Flags() = default;

    auto operator<=>(Flags<BitType> const&) const = default;
    constexpr bool operator<(Flags<BitType> const& rhs) const noexcept { return mask_ < rhs.mask_; }
    constexpr bool operator<=(Flags<BitType> const& rhs) const noexcept { return mask_ <= rhs.mask_; }
    constexpr bool operator>(Flags<BitType> const& rhs) const noexcept { return mask_ > rhs.mask_; }
    constexpr bool operator>=(Flags<BitType> const& rhs) const noexcept { return mask_ >= rhs.mask_; }
    constexpr bool operator==(Flags<BitType> const& rhs) const noexcept { return mask_ == rhs.mask_; }
    constexpr bool operator!=(Flags<BitType> const& rhs) const noexcept { return mask_ != rhs.mask_; }
    constexpr bool operator!() const noexcept { return !mask_; }
    constexpr Flags<BitType> operator&(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(mask_ & rhs.mask_); }
    constexpr Flags<BitType> operator|(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(mask_ | rhs.mask_); }
    constexpr Flags<BitType> operator^(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(mask_ ^ rhs.mask_); }
    constexpr Flags<BitType>& operator|=(Flags<BitType> const& rhs) noexcept {
        mask_ |= rhs.mask_;
        return *this;
    }
    constexpr Flags<BitType>& operator&=(Flags<BitType> const& rhs) noexcept {
        mask_ &= rhs.mask_;
        return *this;
    }
    constexpr Flags<BitType>& operator^=(Flags<BitType> const& rhs) noexcept {
        mask_ ^= rhs.mask_;
        return *this;
    }
    explicit constexpr operator bool() const noexcept { return !!mask_; }
    explicit constexpr operator MaskType() const noexcept { return mask_; }

    /**
     * @brief Checks if no bits are set.
     *
     * @return true if the mask is zero.
     */
    [[nodiscard]] constexpr bool none() const noexcept { return mask_ == 0; }

    /**
     * @brief Checks if the given bit is set.
     *
     * @param bit The bit to check.
     * @return true if the bit is set.
     */
    constexpr bool have(BitType bit) const noexcept { return static_cast<bool>(mask_ & MaskType(bit)); }

    /**
     * @brief Checks if any of the given bits are set.
     *
     * @param bits The bits to check.
     * @return true if any are set.
     */
    constexpr bool any(MaskType bits) const noexcept { return (mask_ & bits) != 0; }

    /**
     * @brief Checks if all of the given bits are set.
     *
     * @param bits The bits to check.
     * @return true if all are set.
     */
    constexpr bool all(MaskType bits) const noexcept { return (mask_ & bits) == bits; }

    /**
     * @brief Returns the internal mask.
     */
    constexpr MaskType get() const noexcept { return mask_; }

    /**
     * @brief Sets a bit.
     *
     * @param bit The bit to set.
     */
    constexpr void set(BitType bit) noexcept { mask_ |= MaskType(bit); }

    /**
     * @brief Toggles a bit (inverts its current state).
     */
    constexpr void toggle(BitType bit) noexcept { mask_ ^= MaskType(bit); }

    /**
     * @brief Clears a bit.
     *
     * @param bit The bit to clear.
     */
    constexpr void clear(BitType bit) noexcept { mask_ &= ~MaskType(bit); }

    /**
     * @brief Clears all bits.
     */
    constexpr void clear() noexcept { mask_ = 0; }

    /**
     * @brief Conditionally sets or clears a bit.
     *
     * @param bit The bit to modify.
     * @param value True to set, false to clear.
     */
    constexpr void set(BitType bit, bool value) noexcept {
        if (value) {
            set(bit);
        } else {
            clear(bit);
        }
    }

  private:
    MaskType mask_;
};

}  // namespace mle

/**
 * @brief Begins the definition of a strongly-typed flag enum.
 *
 * Must be paired with MLE_FLAGS_END. Stores a base counter internally to assign each flag a unique bit.
 *
 * @param name The prefix name of the flag type (e.g., MyFlag)
 * @param type The underlying integer type (e.g., u32, u64)
 *
 * Example:
 * @code
 * MLE_FLAGS_BEGIN(MyFlag, u32)
 * MLE_FLAG(MyFlag, A)
 * MLE_FLAG(MyFlag, B)
 * MLE_FLAGS_END(MyFlag)
 * @endcode
 */
#define MLE_FLAGS_BEGIN(name, type)                         \
    static constexpr auto name##COUNTER_BASE = __COUNTER__; \
    enum class name##FlagBits : type {  // NOLINT(bugprone-macro-parentheses) not possible
/**
 * @brief Declares a flag with an automatically assigned bit.
 *
 * Must be used inside an MLE_FLAGS_BEGIN/END block.
 *
 * @param name The flag group name (used to compute the counter offset)
 * @param flag_name The name of the individual flag
 */
#define MLE_FLAG(name, flag_name) flag_name = bit<mle::u64>(__COUNTER__ - name##COUNTER_BASE - 1),

/**
 * @brief Ends the flag enum definition and creates a corresponding Flags typedef.
 *
 * Must be paired with MLE_FLAGS_BEGIN.
 * Exposes the flag type as name##Flags, which is a typedef for Flags<name##FlagBits>.
 *
 * @param name The prefix used for the flag type
 */
#define MLE_FLAGS_END(name) \
    }                       \
    ;                       \
    using name##Flags = Flags<name##FlagBits>

/**
 * @brief [INTERNAL] Begins a custom fmt formatter specialization for a single flag bit.
 *
 * This macro should not be used directly. Use MLE_FLAGS_FMT_BEGIN instead.
 *
 * @param name The flag group name
 *
 * @internal
 */
#define MLE_DETAIL_FLAGS_FMT_BEGIN(name)                                       \
    namespace fmt {                                                            \
    template <>                                                                \
    struct formatter<name##FlagBits> : formatter<std::string> {                \
        template <typename FormatContext>                                      \
        constexpr auto format(name##FlagBits flag, FormatContext& ctx) const { \
            switch (flag) {
/**
 * @brief [INTERNAL] Adds a case to the internal flag bit formatter.
 *
 * @param name The flag group name
 * @param flag_name The name of the flag to format
 *
 * @internal
 */
#define MLE_DETAIL_FLAGS_FMT_CASE(name, flag_name) \
    case name##FlagBits::flag_name:                \
        return format_to(ctx.out(), #flag_name);

/**
 * @brief [INTERNAL] Ends the custom formatter specialization for both bit and flag types.
 *
 * @param name The flag group name
 *
 * @internal
 */
#define MLE_DETAIL_FLAGS_FMT_END(name)                                           \
    default:                                                                     \
        MLE_UNREACHABLE;                                                         \
        }                                                                        \
        }                                                                        \
        }                                                                        \
        ;                                                                        \
                                                                                 \
        template <>                                                              \
        struct formatter<name##Flags> : formatter<std::string> {                 \
            template <typename FormatContext>                                    \
            constexpr auto format(name##Flags flags, FormatContext& ctx) const { \
                format_to(ctx.out(), "[ ");                                      \
                mle::u64 i = 1;                                                  \
                while (i <= flags.get()) {                                       \
                    auto flag = static_cast<name##FlagBits>(i);                  \
                    if (flags.have(flag)) {                                      \
                        format_to(ctx.out(), "{} ", flag);                       \
                    }                                                            \
                    i <<= 1U;                                                    \
                }                                                                \
                return format_to(ctx.out(), "]");                                \
            }                                                                    \
        };                                                                       \
        }

/**
 * @brief Begins a flag formatter block for use with fmt.
 */
#define MLE_FLAGS_FMT_BEGIN(...) MLE_DETAIL_FLAGS_FMT_BEGIN(__VA_ARGS__)  // NOLINT(cppcoreguidelines-macro-usage) impossible

/**
 * @brief Defines a flag case inside a formatter block.
 */
#define MLE_FLAGS_FMT_CASE(...) MLE_DETAIL_FLAGS_FMT_CASE(__VA_ARGS__)  // NOLINT(cppcoreguidelines-macro-usage) impossible

/**
 * @brief Ends a flag formatter block.
 */
#define MLE_FLAGS_FMT_END(...) MLE_DETAIL_FLAGS_FMT_END(__VA_ARGS__)  // NOLINT(cppcoreguidelines-macro-usage) impossible
