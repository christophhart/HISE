#pragma once

#include "basic_math.hpp"

namespace math_approx
{
namespace inv_trig_detail
{
    // for polynomial derivations, see notebooks/asin_acos_approx.nb

    template <int order, typename T>
    constexpr T asin_kernel (T x)
    {
        using S = scalar_of_t<T>;
        static_assert (order >= 1 && order <= 4);

        if constexpr (order == 1)
        {
            return (S) 0.16443531037029196495 + x * (S) 0.097419577664394046979;
        }
        else if constexpr (order == 2)
        {
            return (S) 0.16687742065041710759 + x * ((S) 0.070980446338571381859 + x * (S) 0.066682760821292624831);
        }
        else if constexpr (order == 3)
        {
            return (S) 0.16665080061757006624 + x * ((S) 0.075508850204912977833 + x * ((S) 0.039376231206556484843 + x * (S) 0.051275338699694958389));
        }
        else if constexpr (order == 4)
        {
            return (S) 0.16666803275183153521 + x * ((S) 0.074936964020844071266 + x * ((S) 0.045640288439217274741 + x * ((S) 0.023435504410713306478 + x * (S) 0.043323710842752508055)));
        }
        else
        {
            return {};
        }
    }

    template <int order, typename T>
    constexpr T acos_kernel (T x)
    {
        using S = scalar_of_t<T>;
        static_assert (order >= 1 && order <= 5);

        if constexpr (order == 1)
        {
            return (S) 0.061454830783555181029 + x * (S) 0.50934149601134137697;
        }
        else if constexpr (order == 2)
        {
            return (S) 0.18188825560430002537 + x * ((S) -0.092825628092384385170 + x * (S) 0.48173369928298098719);
        }
        else if constexpr (order == 3)
        {
            return (S) 0.16480511788348814473 + x * ((S) 0.11286070199090997290 + x * ((S) -0.18795205899643871450 + x * (S) 0.48108256591693704385));
        }
        else if constexpr (order == 4)
        {
            return (S) 0.16687235373875186628 + x * ((S) 0.068412956842158992310 + x * ((S) 0.11466969910945928879 + x * ((S) -0.27433862418620241774 + x * (S) 0.49517994129072917531)));
        }
        else if constexpr (order == 5)
        {
            return (S) 0.16664924406383360700 + x * ((S) 0.075837825275592588015 + x * ((S) 0.030665158374004904823 + x * ((S) 0.13572846625592635550 + x * ((S) -0.34609357317006372856 + x * (S) 0.50800920599560273061))));
        }
        else
        {
            return {};
        }
    }

    // for polynomial derivations, see notebooks/arctan_approx.nb

    template <int order, typename T>
    constexpr T atan_kernel (T x)
    {
        using S = scalar_of_t<T>;
        static_assert (order >= 4 && order <= 7);

        if constexpr (order == 4)
        {
            const auto x_sq = x * x;
            const auto num = x + x_sq * (S) 0.498001992540;
            const auto den = (S) 1 + x * (S) 0.481844539675 + x_sq * (S) 0.425470835319;
            return num / den;
        }
        else if constexpr (order == 5 || order == 6)
        {
            const auto x_sq = x * x;
            const auto num = (S) 0.177801521472 + x * (S) 0.116983970701;
            const auto den = (S) 1 + x * (S) 0.174763903018 + x_sq * (S) 0.473808187566;
            return (x + x_sq * num) / den;
        }
        else if constexpr (order == 7)
        {
            const auto x_sq = x * x;
            const auto num = (S) 0.274959104817 + (S) 0.351814748865 * x + (S) -0.0395798531406 * x_sq;
            const auto den = (S) 1 + x * ((S) 0.275079063405 + x * ((S) 0.683311392128 + x * (S) 0.0624877111229));
            return (x + x_sq * num) / den;
        }
        else
        {
            return {};
        }
    }
} // namespace inv_trig_detail

/**
 * Approximation of asin(x) using asin(x) ≈ p(x^2) * x^3 + x for x in [0, 0.5],
 * and asin(x) ≈ pi/2 - p((1-x)/2) * ((1-x)/2)^3/2 + ((1-x)/2)^1/2 for x in [0.5, 1],
 * where p(x) is a polynomial fit to achieve the minimum absolute error.
 */
template <int order, typename T>
T asin (T x)
{
    using S = scalar_of_t<T>;

    using std::abs, std::sqrt;
#if defined(XSIMD_HPP)
    using xsimd::abs, xsimd::sqrt;
#endif

    const auto abs_x = abs (x);

    const auto reflect = abs_x > (S) 0.5;
    auto z0 = select (reflect, (S) 0.5 * ((S) 1 - abs_x), abs_x * abs_x);

    auto x2 = select (reflect, sqrt (z0), abs_x);
    auto z1 = inv_trig_detail::asin_kernel<order> (z0);

    auto z2 = z1 * (z0 * x2) + x2;
    auto res = select (reflect, (S) M_PI_2 - (z2 + z2), z2);
    return select (x > (S) 0, res, -res);
}

/**
 * Approximation of acos(x) using the same approach as asin(x),
 * but with a different polynomial fit.
 */
template <int order, typename T>
T acos (T x)
{
    using S = scalar_of_t<T>;

    using std::abs, std::sqrt;
#if defined(XSIMD_HPP)
    using xsimd::abs, xsimd::sqrt;
#endif

    const auto abs_x = abs (x);

    const auto reflect = abs_x > (S) 0.5;
    auto z0 = select (reflect, (S) 0.5 * ((S) 1 - abs_x), abs_x * abs_x);

    auto x2 = select (reflect, sqrt (z0), abs_x);
    auto z1 = inv_trig_detail::acos_kernel<order> (z0);

    auto z2 = z1 * (z0 * x2) + x2;
    auto res = select (reflect, (S) M_PI_2 - (z2 + z2), z2);
    return (S) M_PI_2 - select (x > (S) 0, res, -res);
}

/**
 * Approximation of atan(x) using a polynomial approximation of arctan(x) on [0, 1],
 * and arctan(x) = pi/2 - arctan(1/x) for x > 1.
 */
template <int order, typename T>
T atan (T x)
{
    using S = scalar_of_t<T>;

    using std::abs, std::sqrt;
#if defined(XSIMD_HPP)
    using xsimd::abs, xsimd::sqrt;
#endif

    const auto abs_x = abs (x);
    const auto reflect = abs_x > (S) 1;

    const auto z = select (reflect, (S) 1 / abs_x, abs_x);
    const auto atan_01 = inv_trig_detail::atan_kernel<order> (z);

    const auto res = select (reflect, (S) M_PI_2 - atan_01, atan_01);
    return select (x > (S) 0, res, -res);
}
} // namespace math_approx
