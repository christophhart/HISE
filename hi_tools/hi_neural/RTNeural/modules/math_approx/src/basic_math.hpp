#pragma once

// If MATH_APPROX_XSIMD_TARGET is not defined
// the user can still use XSIMD by manually including
// it before including the math_approx header.
#if MATH_APPROX_XSIMD_TARGET
#include <xsimd/xsimd.hpp>
#endif

#if ! defined(XSIMD_HPP)
#include <cmath>
#endif

#include <algorithm>
#include <bit>

namespace math_approx
{
template <typename T>
struct scalar_of
{
    using type = T;
};

/**
 * When T is a scalar floating-point type, scalar_of_t<T> is T.
 * When T is a SIMD floating-point type, scalar_of_t<T> is the corresponding scalar type.
 */
template <typename T>
using scalar_of_t = typename scalar_of<T>::type;

/** Inverse square root */
template <typename T>
T rsqrt (T x)
{
    // @TODO: figure out a way that we can make this method constexpr

    // sqrtss followed by divss... this seems to measure a bit faster than the rsqrtss plus NR iteration below
    return (T) 1 / std::sqrt (x);

    // fast inverse square root (using rsqrtss hardware instruction), plus one Newton-Raphson iteration
    //    auto r = xsimd::rsqrt (xsimd::broadcast (x)).get (0);
    //    x *= r;
    //    x *= r;
    //    x += -3.0f;
    //    r *= -0.5f;
    //    return x * r;
}

/** Function interface for the ternary operator. */
template <typename T>
T select (bool q, T t, T f)
{
    return q ? t : f;
}

#if defined(XSIMD_HPP)
template <typename T>
struct scalar_of<xsimd::batch<T>>
{
    using type = T;
};

/** Inverse square root */
template <typename T>
xsimd::batch<T> rsqrt (xsimd::batch<T> x)
{
    using S = scalar_of_t<T>;
    auto r = xsimd::rsqrt (x);
    x *= r;
    x *= r;
    x += (S) -3;
    r *= (S) -0.5;
    return x * r;
}

/** Function interface for the ternary operator. */
template <typename T>
xsimd::batch<T> select (xsimd::batch_bool<T> q, xsimd::batch<T> t, xsimd::batch<T> f)
{
    return xsimd::select (q, t, f);
}
#endif

#if ! __cpp_lib_bit_cast
// bit_cast requirement.
template <typename From, typename To>
using is_bitwise_castable = std::integral_constant<bool,
                                                   (sizeof (From) == sizeof (To)) && std::is_trivially_copyable<From>::value && std::is_trivially_copyable<To>::value>;

// compiler support is needed for bitwise copy with constexpr.
template <typename To, typename From>
inline typename std::enable_if<is_bitwise_castable<From, To>::value, To>::type bit_cast (const From& from) noexcept
{
    union U
    {
        U() {};
        char storage[sizeof (To)] {};
        typename std::remove_const<To>::type dest;
    } u; // instead of To dest; because To doesn't require DefaultConstructible.
    std::memcpy (&u.dest, &from, sizeof from);
    return u.dest;
}
#else
using std::bit_cast;
#endif
} // namespace math_approx
