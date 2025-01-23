#pragma once

#include "basic_math.hpp"

namespace math_approx
{
namespace trig_detail
{
    template <typename T>
    constexpr T truncate (T x)
    {
        return static_cast<T> (static_cast<int> (x));
    }

#if defined(XSIMD_HPP)
    template <typename T>
    xsimd::batch<T> truncate (xsimd::batch<T> x)
    {
        return xsimd::to_float (xsimd::to_int (x));
    }
#endif

    /** Fast method to wrap a value into the range [-pi, pi] */
    template <typename T>
    constexpr T fast_mod_mpi_pi (T x)
    {
        using S = scalar_of_t<T>;
        constexpr auto pi = static_cast<S> (M_PI);
        constexpr auto two_pi = static_cast<S> (2.0 * M_PI);
        constexpr auto recip_two_pi = static_cast<S> (1) / two_pi;

        x += pi;
        const auto mod = x - two_pi * truncate (x * recip_two_pi);
        return select (x >= (T) 0, mod, mod + two_pi) - pi;
    }

    /** Fast method to wrap a value into the range [-pi/2, pi/2] */
    template <typename T>
    constexpr T fast_mod_mhalfpi_halfpi (T x)
    {
        using S = scalar_of_t<T>;
        constexpr auto half_pi = static_cast<S> (M_PI) * (S) 0.5;
        constexpr auto pi = static_cast<S> (M_PI);
        constexpr auto recip_pi = (S) 1 / pi;

        x += half_pi;
        const auto mod = x - pi * truncate (x * recip_pi);
        return select (x >= (T) 0, mod, mod + pi) - half_pi;
    }

    // Polynomials were derived using the method presented in
    // https://mooooo.ooo/chebyshev-sine-approximation/
    // and then adapted for various (odd) orders.

    template <typename T>
    constexpr T sin_poly_9 (T x, T x_sq)
    {
        using S = scalar_of_t<T>;
        const auto x_7_9 = (S) -2.49397084313e-6 + (S) 2.00382818811e-8 * x_sq;
        const auto x_5_7_9 = (S) 0.000173405228576 + x_7_9 * x_sq;
        const auto x_3_5_7_9 = (S) -0.00662075636230 + x_5_7_9 * x_sq;
        const auto x_1_3_5_7_9 = (S) 0.101321159036 + x_3_5_7_9 * x_sq;
        return x * x_1_3_5_7_9;
    }

    template <typename T>
    constexpr T sin_poly_7 (T x, T x_sq)
    {
        using S = scalar_of_t<T>;
        const auto x_5_7 = (S) 0.000170965340046 + (S) -2.09843101304e-6 * x_sq;
        const auto x_3_5_7 = (S) -0.00661594021539 + x_5_7 * x_sq;
        const auto x_1_3_5_7 = (S) 0.101319673615 + x_3_5_7 * x_sq;
        return x * x_1_3_5_7;
    }

    template <typename T>
    constexpr T sin_poly_5 (T x, T x_sq)
    {
        using S = scalar_of_t<T>;
        const auto x_3_5 = (S) -0.00650096169550 + (S) 0.000139899314103 * x_sq;
        const auto x_1_3_5 = (S) 0.101256629587 + x_3_5 * x_sq;
        return x * x_1_3_5;
    }
} // namespace sin_detail

/** Polynomial approximation of sin(x) on the range [-pi, pi] */
template <int order, typename T>
constexpr T sin_mpi_pi (T x)
{
    static_assert (order % 2 == 1 && order <= 9 && order >= 5, "Order must be an odd number within [5, 9]");

    using S = scalar_of_t<T>;
    constexpr auto pi = static_cast<S> (M_PI);
    constexpr auto pi_sq = pi * pi;
    const auto x_sq = x * x;

    T x_poly {};
    if constexpr (order == 9)
        x_poly = trig_detail::sin_poly_9 (x, x_sq);
    else if constexpr (order == 7)
        x_poly = trig_detail::sin_poly_7 (x, x_sq);
    else if constexpr (order == 5)
        x_poly = trig_detail::sin_poly_5 (x, x_sq);

    return (pi_sq - x_sq) * x_poly;
}

/** Full range approximation of sin(x) */
template <int order, typename T>
constexpr T sin (T x)
{
    return sin_mpi_pi<order, T> (trig_detail::fast_mod_mpi_pi (x));
}

/**
 * Polynomial approximation of cos(x) on the range [-pi, pi],
 * using a range-shifted approximation of sin(x).
 */
template <int order, typename T>
constexpr T cos_mpi_pi (T x)
{
    static_assert (order % 2 == 1 && order <= 9 && order >= 5, "Order must be an odd number within [5, 9]");

    using S = scalar_of_t<T>;
    constexpr auto pi = static_cast<S> (M_PI);
    constexpr auto pi_sq = pi * pi;
    constexpr auto pi_o_2 = pi * (S) 0.5;

    using std::abs;
#if defined(XSIMD_HPP)
    using xsimd::abs;
#endif
    x = abs (x);

    const auto hpmx = pi_o_2 - x;
    const auto hpmx_sq = hpmx * hpmx;

    T x_poly {};
    if constexpr (order == 9)
        x_poly = trig_detail::sin_poly_9 (hpmx, hpmx_sq);
    else if constexpr (order == 7)
        x_poly = trig_detail::sin_poly_7 (hpmx, hpmx_sq);
    else if constexpr (order == 5)
        x_poly = trig_detail::sin_poly_5 (hpmx, hpmx_sq);

    return (pi_sq - hpmx_sq) * x_poly;
}

/** Full range approximation of cos(x) */
template <int order, typename T>
constexpr T cos (T x)
{
    return cos_mpi_pi<order, T> (trig_detail::fast_mod_mpi_pi (x));
}

/** Polynomial approximation of tan(x) on the range [-pi/4, pi/4] */
template <int order, typename T>
constexpr T tan_mquarterpi_quarterpi (T x)
{
    static_assert (order % 2 == 1 && order >= 3 && order <= 15, "Order must be an odd number within [3, 15]");

    // for polynomial derivation, see notebooks/tan_approx.nb

    using S = scalar_of_t<T>;
    const auto x_sq = x * x;
    if constexpr (order == 3)
    {
        const auto x_1_3 = (S) 1 + (S) 0.442959265447 * x_sq;
        return x * x_1_3;
    }
    else if constexpr (order == 5)
    {
        const auto x_3_5 = (S) 0.317574684334 + (S) 0.203265826702 * x_sq;
        const auto x_1_3_5 = (S) 1 + x_3_5 * x_sq;
        return x * x_1_3_5;
    }
    else if constexpr (order == 7)
    {
        const auto x_5_7 = (S) 0.116406244996 + (S) 0.0944480566104 * x_sq;
        const auto x_1_3 = (S) 1 + (S) 0.335216153138 * x_sq;
        const auto x_1_3_5_7 = x_1_3 + x_5_7 * x_sq * x_sq;
        return x * x_1_3_5_7;
    }
    else if constexpr (order == 9)
    {
        const auto x_7_9 = (S) 0.0405232529373 + (S) 0.0439292071029 * x_sq;
        const auto x_3_5 = (S) 0.333131667276 + (S) 0.136333765649 * x_sq;
        const auto x_3_5_7_9 = x_3_5 + x_7_9 * x_sq * x_sq;
        return x * ((S) 1 + x_3_5_7_9 * x_sq);
    }
    else if constexpr (order == 11)
    {
        const auto x_q = x_sq * x_sq;
        const auto x_9_11 = (S) 0.0126603694551 + (S) 0.0203633469693 * x_sq;
        const auto x_5_7 = (S) 0.132897195017 + (S) 0.0570525279731 * x_sq;
        const auto x_1_3 = (S) 1 + (S) 0.333353019629 * x_sq;
        const auto x_5_7_9_11 = x_5_7 + x_9_11 * x_q;
        const auto x_1_3_5_7_9_11 = x_1_3 + x_5_7_9_11 * x_q;
        return x * x_1_3_5_7_9_11;
    }
    else if constexpr (order == 13)
    {
        const auto x_q = x_sq * x_sq;
        const auto x_6 = x_q * x_sq;
        const auto x_11_13 = (S) 0.00343732283737 + (S) 0.00921082294855 * x_sq;
        const auto x_7_9 = (S) 0.0534743904687 + (S) 0.0242183751709 * x_sq;
        const auto x_3_5 = (S) 0.333331890901 + (S) 0.133379954680 * x_sq;
        const auto x_7_9_11_13 = x_7_9 + x_11_13 * x_q;
        const auto x_1_3_5 = (S) 1 + x_3_5 * x_sq;
        return x * (x_1_3_5 + x_7_9_11_13 * x_6);
    }
    else if constexpr (order == 15)
    {
        // doesn't seem to help much at single-precision, but here it is:
        const auto x_q = x_sq * x_sq;
        const auto x_8 = x_q * x_q;
        const auto x_13_15 = (S) 0.000292958045126 + (S) 0.00427933470414 * x_sq;
        const auto x_9_11 = (S) 0.0213477960960 + (S) 0.0106702896251 * x_sq;
        const auto x_5_7 = (S) 0.133327796402 + (S) 0.0540469276103* x_sq;
        const auto x_1_3 = (S) 1 + (S) 0.333333463757 * x_sq;
        const auto x_9_11_13_15 = x_9_11 + x_13_15 * x_q;
        const auto x_1_3_5_7 = x_1_3 + x_5_7 * x_q;
        const auto x_1_3_5_7_9_11_13_15 = x_1_3_5_7 + x_9_11_13_15 * x_8;
        return x * x_1_3_5_7_9_11_13_15;
    }
    else
    {
        return {};
    }
}

/**
 * Approximation of tan(x) on the range [-pi/2, pi/2],
 * using the tangent half-angle formula.
 *
 * Accuracy may suffer as x approaches ±pi/2.
 */
template <int order, typename T>
constexpr T tan_mhalfpi_halfpi (T x)
{
    using S = scalar_of_t<T>;
    const auto h_x = tan_mquarterpi_quarterpi<order> ((S) 0.5 * x);
    return (S) 2 * h_x / ((S) 1 - h_x * h_x);
}

/**
 * Full-range approximation of tan(x)
 *
 * Accuracy may suffer as x approaches values for which tan(x) approaches ±Inf.
 */
template <int order, typename T>
constexpr T tan (T x)
{
    return tan_mhalfpi_halfpi<order> (trig_detail::fast_mod_mhalfpi_halfpi (x));
}
} // namespace math_approx
