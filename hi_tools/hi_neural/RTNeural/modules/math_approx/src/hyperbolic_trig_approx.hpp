#pragma once

#include "pow_approx.hpp"

namespace math_approx
{
// ref: https://en.wikipedia.org/wiki/Hyperbolic_functions#Definitions
// sinh = (e^(2x) - 1) / (2e^x), cosh = (e^(2x) + 1) / (2e^x)
// let B = e^x, then sinh = (B^2 - 1) / (2B), cosh = (B^2 + 1) / (2B)
// simplifying, we get: sinh = 0.5 (B - 1/B), cosh = 0.5 (B + 1/B)

/** Approximation of sinh(x), using exp(x) internally */
template <int order, typename T>
constexpr T sinh (T x)
{
    using S = scalar_of_t<T>;
    auto B = exp<order> (x);
    auto Br = (S) 0.5 / B;
    B *= (S) 0.5;
    return B - Br;
}

/** Approximation of cosh(x), using exp(x) internally */
template <int order, typename T>
constexpr T cosh (T x)
{
    using S = scalar_of_t<T>;
    auto B = exp<order> (x);
    auto Br = (S) 0.5 / B;
    B *= (S) 0.5;
    return B + Br;
}

/**
 * Simultaneous pproximation of sinh(x) and cosh(x),
 * using exp(x) internally.
 *
 * For more information see the comments above.
 */
template <int order, typename T>
constexpr auto sinh_cosh (T x)
{
    using S = scalar_of_t<T>;
    auto B = exp<order> (x);
    auto Br = (S) 0.5 / B;
    B *= (S) 0.5;

    auto sinh = B - Br;
    auto cosh = B + Br;

    return std::make_pair (sinh, cosh);
}

namespace tanh_detail
{
    // See notebooks/tanh_approx.nb for the derivation of these polynomials

    template <typename T>
    constexpr T tanh_poly_11 (T x)
    {
        using S = scalar_of_t<T>;
        const auto x_sq = x * x;
        const auto y_9_11 = (S) 2.63661358122e-6 + (S) 3.33765558362e-8 * x_sq;
        const auto y_7_9_11 = (S) 0.000199027336899 + y_9_11 * x_sq;
        const auto y_5_7_9_11 = (S) 0.00833223857843 + y_7_9_11 * x_sq;
        const auto y_3_5_7_9_11 = (S) 0.166667159320 + y_5_7_9_11 * x_sq;
        const auto y_1_3_5_7_9_11 = (S) 1 + y_3_5_7_9_11 * x_sq;
        return x * y_1_3_5_7_9_11;
    }

    template <typename T>
    constexpr T tanh_poly_9 (T x)
    {
        using S = scalar_of_t<T>;
        const auto x_sq = x * x;
        const auto y_7_9 = (S) 0.000192218110330 + (S) 3.54808622170e-6 * x_sq;
        const auto y_5_7_9 = (S) 0.00834777254865 + y_7_9 * x_sq;
        const auto y_3_5_7_9 = (S) 0.166658873283 + y_5_7_9 * x_sq;
        const auto y_1_3_5_7_9 = (S) 1 + y_3_5_7_9 * x_sq;
        return x * y_1_3_5_7_9;
    }

    template <typename T>
    constexpr T tanh_poly_7 (T x)
    {
        using S = scalar_of_t<T>;
        const auto x_sq = x * x;
        const auto y_5_7 = (S) 0.00818199927912 + (S) 0.000243153287690 * x_sq;
        const auto y_3_5_7 = (S) 0.166769941467 + y_5_7 * x_sq;
        const auto y_1_3_5_7 = (S) 1 + y_3_5_7 * x_sq;
        return x * y_1_3_5_7;
    }

    template <typename T>
    constexpr T tanh_poly_5 (T x)
    {
        using S = scalar_of_t<T>;
        const auto x_sq = x * x;
        const auto y_3_5 = (S) 0.165326984031 + (S) 0.00970240200826 * x_sq;
        const auto y_1_3_5 = (S) 1 + y_3_5 * x_sq;
        return x * y_1_3_5;
    }

    template <typename T>
    constexpr T tanh_poly_3 (T x)
    {
        using S = scalar_of_t<T>;
        const auto x_sq = x * x;
        const auto y_1_3 = (S) 1 + (S) 0.183428244899 * x_sq;
        return x * y_1_3;
    }
} // namespace tanh_detail

/**
 * Approximation of tanh(x), using tanh(x) ≈ p(x) / (p(x)^2 + 1),
 * where p(x) is an odd polynomial fit to minimize the maxinimum relative error.
 */
template <int order, typename T>
T tanh (T x)
{
    static_assert (order % 2 == 1 && order <= 11 && order >= 3, "Order must e an odd number within [3, 11]");

    T x_poly {};
    if constexpr (order == 11)
        x_poly = tanh_detail::tanh_poly_11 (x);
    else if constexpr (order == 9)
        x_poly = tanh_detail::tanh_poly_9 (x);
    else if constexpr (order == 7)
        x_poly = tanh_detail::tanh_poly_7 (x);
    else if constexpr (order == 5)
        x_poly = tanh_detail::tanh_poly_5 (x);
    else if constexpr (order == 3)
        x_poly = tanh_detail::tanh_poly_3 (x);

    using S = scalar_of_t<T>;
    return x_poly * rsqrt (x_poly * x_poly + (S) 1);
}
} // namespace math_approx
