#pragma once

#include "basic_math.hpp"
#include "log_approx.hpp"

namespace math_approx
{
struct AsinhLog2Provider
{
    // for polynomial derivations, see notebooks/asinh_approx.nb

    /** approximation for log2(x), optimized on the range [1, 2], to be used within an asinh(x) computation */
    template <typename T, int order, bool /*C1_continuous*/>
    static constexpr T log2_approx (T x)
    {
        static_assert (order >= 3 && order <= 5);
        using S = scalar_of_t<T>;

        const auto x_sq = x * x;
        if constexpr (order == 3)
        {
            const auto x_2_3 = (S) -1.21535595794871 + (S) 0.194363894384581 * x;
            const auto x_0_1 = (S) -2.26452854958994 + (S) 3.28552061315407 * x;
            return x_0_1 + x_2_3 * x_sq;
        }
        else if constexpr (order == 4)
        {
            const auto x_3_4 = (S) 0.770443387059628 + (S) -0.102652345633016 * x;
            const auto x_1_2 = (S) 4.33013912645867 + (S) -2.39448588379361 * x;
            const auto x_1_2_3_4 = x_1_2 + x_3_4 * x_sq;
            return (S) -2.60344428409168 + x_1_2_3_4 * x;
        }
        else if constexpr (order == 5)
        {
            const auto x_4_5 = (S) -0.511946284688366 + (S) 0.0578217518982235 * x;
            const auto x_2_3 = (S) -3.94632584968643 + (S) 1.90796087279737 * x;
            const auto x_0_1 = (S) -2.87748189127908 + (S) 5.36997140095829 * x;
            const auto x_2_3_4_5 = x_2_3 + x_4_5 * x_sq;
            return x_0_1 + x_2_3_4_5 * x_sq;
        }
        else
        {
            return {};
        }
    }
};

/**
 * Approximation of asinh(x) in the full range, using identity
 * asinh(x) = log(x + sqrt(x^2 + 1)).
 *
 * Orders 6 and 7 use an additional Newton-Raphson iteration,
 * but for most cases the accuracy improvement is not worth
 * the additional cost (when compared to the performance and
 * accuracy achieved by the STL implementation).
 */
template <int order, typename T>
constexpr T asinh (T x)
{
    using S = scalar_of_t<T>;
    using std::abs, std::sqrt;
#if defined(XSIMD_HPP)
    using xsimd::abs, xsimd::sqrt;
#endif

    const auto sign = select (x > (S) 0, (T) (S) 1, select (x < (S) 0, (T) (S) -1, (T) (S) 0));
    x = abs (x);

    const auto log_arg = x + sqrt (x * x + (S) 1);
    auto y = log<pow_detail::BaseE<scalar_of_t<T>>, std::min (order, 5), false, AsinhLog2Provider> (log_arg);

    if constexpr (order > 5)
    {
        const auto exp_y = math_approx::exp<order - 1> (y);
        y -= (exp_y - log_arg) / exp_y;
    }

    return sign * y;
}

/**
 * Approximation of acosh(x) in the full range, using identity
 * acosh(x) = log(x + sqrt(x^2 - 1)).
 */
template <int order, typename T>
constexpr T acosh (T x)
{
    using S = scalar_of_t<T>;
    using std::sqrt;
#if defined(XSIMD_HPP)
    using xsimd::sqrt;
#endif

    const auto z1 = x + sqrt (x * x - (S) 1);
    return log<order> (z1);
}

/**
 * Approximation of atanh(x), using identity
 * atanh(x) = (1/2) log((x + 1) / (x - 1)).
 */
template <int order, typename T>
constexpr T atanh (T x)
{
    using S = scalar_of_t<T>;
    return (S) 0.5 * log<order> (((S) 1 + x) / ((S) 1 - x));
}
} // namespace math_approx
