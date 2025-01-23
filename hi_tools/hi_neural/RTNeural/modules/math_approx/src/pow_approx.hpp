#pragma once

#include "basic_math.hpp"

namespace math_approx
{
namespace pow_detail
{
    // for polynomial derivations, see notebooks/exp_approx.nb

    /** approximation for 2^x, optimized on the range [0, 1] */
    template <typename T, int order, bool C1_continuous>
    constexpr T pow2_approx (T x)
    {
        static_assert (order >= 3 && order <= 7);
        using S = scalar_of_t<T>;

        const auto x_sq = x * x;
        if constexpr (C1_continuous)
        {
            if constexpr (order == 3)
            {
                const auto x_2_3 = (S) 0.227411277760 + (S) 0.0794415416798 * x;
                const auto x_0_1 = (S) 1 + (S) 0.693147180560 * x;
                return x_0_1 + x_2_3 * x_sq;
            }
            else if constexpr (order == 4)
            {
                const auto x_3_4 = (S) 0.0521277476109 + (S) 0.0136568970345 * x;
                const auto x_1_2 = (S) 0.693147180560 + (S) 0.241068174795 * x;
                const auto x_1_2_3_4 = x_1_2 + x_3_4 * x_sq;
                return (S) 1 + x_1_2_3_4 * x;
            }
            else if constexpr (order == 5)
            {
                const auto x_4_5 = (S) 0.00899838527231 + (S) 0.00188723482038 * x;
                const auto x_2_3 = (S) 0.240184132673 + (S) 0.0557830666741 * x;
                const auto x_2_3_4_5 = x_2_3 + x_4_5 * x_sq;
                const auto x_0_1 = (S) 1 + (S) 0.693147180560 * x;
                return x_0_1 + x_2_3_4_5 * x_sq;
            }
            else if constexpr (order == 6)
            {
                const auto x_5_6 = (S) 0.00124453797252 + (S) 0.000217714753229 * x;
                const auto x_3_4 = (S) 0.0554875633068 + (S) 0.00967475272129 * x;
                const auto x_1_2 = (S) 0.693147180560 + (S) 0.240228250686 * x;
                const auto x_3_4_5_6 = x_3_4 + x_5_6 * x_sq;
                const auto x_1_2_3_4_5_6 = x_1_2 + x_3_4_5_6 * x_sq;
                return (S) 1 + x_1_2_3_4_5_6 * x;
            }
            else if constexpr (order == 7)
            {
                // doesn't seem to help at single-precision
                const auto x_6_7 = (S) 0.000133154170702612 + (S) 0.0000245778949916153 * x;
                const auto x_4_5 = (S) 0.00960612128901630 + (S) 0.00135551454943593 * x;
                const auto x_2_3 = (S) 0.240226202240181 + (S) 0.0555072492957270 * x;
                const auto x_0_1 = (S) 1 + (S) 0.693147180559945 * x;
                const auto x_4_5_6_7 = x_4_5 + x_6_7 * x_sq;
                const auto x_0_1_2_3 = x_0_1 + x_2_3 * x_sq;
                return x_0_1_2_3 + x_4_5_6_7 * x_sq * x_sq;
            }
            else
            {
                return {};
            }
        }
        else
        {
            if constexpr (order == 3)
            {
                const auto x_2_3 = (S) 0.226307586882 + (S) 0.0782680256330 * x;
                const auto x_0_1 = (S) 1 + (S) 0.695424387485 * x;
                return x_0_1 + x_2_3 * x_sq;
            }
            else if constexpr (order == 4)
            {
                const auto x_3_4 = (S) 0.0520324008177 + (S) 0.0135557244044 * x;
                const auto x_1_2 = (S) 0.693032120001 + (S) 0.241379754777 * x;
                const auto x_1_2_3_4 = x_1_2 + x_3_4 * x_sq;
                return (S) 1 + x_1_2_3_4 * x;
            }
            else if constexpr (order == 5)
            {
                const auto x_4_5 = (S) 0.00899009909264 + (S) 0.00187839071291 * x;
                const auto x_2_3 = (S) 0.240156326598 + (S) 0.0558229130202 * x;
                const auto x_2_3_4_5 = x_2_3 + x_4_5 * x_sq;
                const auto x_0_1 = (S) 1 + (S) 0.693152270576 * x;
                return x_0_1 + x_2_3_4_5 * x_sq;
            }
            else if constexpr (order == 6)
            {
                const auto x_5_6 = (S) 0.00124359387839 + (S) 0.000217187820427 * x;
                const auto x_3_4 = (S) 0.0554833098983 + (S) 0.00967911763840 * x;
                const auto x_1_2 = (S) 0.693147003658 + (S) 0.240229787107 * x;
                const auto x_3_4_5_6 = x_3_4 + x_5_6 * x_sq;
                const auto x_1_2_3_4_5_6 = x_1_2 + x_3_4_5_6 * x_sq;
                return (S) 1 + x_1_2_3_4_5_6 * x;
            }
            else if constexpr (order == 7)
            {
                // doesn't seem to help at single-precision
                const auto x_6_7 = (S) 0.000136898688977877 + (S) 0.0000234440812713967 * x;
                const auto x_4_5 = (S) 0.00960825566419915 + (S) 0.00135107295099880 * x;
                const auto x_2_3 = (S) 0.240226092549669 + (S) 0.0555070350342468 * x;
                const auto x_0_1 = (S) 1 + (S) 0.693147201030637 * x;
                const auto x_4_5_6_7 = x_4_5 + x_6_7 * x_sq;
                const auto x_0_1_2_3 = x_0_1 + x_2_3 * x_sq;
                return x_0_1_2_3 + x_4_5_6_7 * x_sq * x_sq;
            }
            else
            {
                return {};
            }
        }
    }

    template <typename T>
    struct BaseE
    {
        static constexpr auto log2_base = (T) 1.4426950408889634074;
    };

    template <typename T>
    struct Base2
    {
        static constexpr auto log2_base = (T) 1;
    };

    template <typename T>
    struct Base10
    {
        static constexpr auto log2_base = (T) 3.3219280948873623479;
    };
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing" // these methods require some type-punning
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

/** approximation for pow(Base, x) (32-bit) */
template <typename Base, int order, bool C1_continuous>
constexpr float pow (float x)
{
    x = std::max (-126.0f, Base::log2_base * x);

    const auto xi = (int32_t) x;
    const auto l = x < 0.0f ? xi - 1 : xi;
    const auto f = x - (float) l;
    const auto vi = (l + 127) << 23;

    return bit_cast<float> (vi) * pow_detail::pow2_approx<float, order, C1_continuous> (f);
}

/** approximation for pow(Base, x) (64-bit) */
template <typename Base, int order, bool C1_continuous>
constexpr double pow (double x)
{
    x = std::max (-1022.0, Base::log2_base * x);

    const auto xi = (int64_t) x;
    const auto l = x < 0.0 ? xi - 1 : xi;
    const auto d = x - (double) l;
    const auto vi = (l + 1023) << 52;

    return bit_cast<double> (vi) * pow_detail::pow2_approx<double, order, C1_continuous> (d);
}

#if defined(XSIMD_HPP)
/** approximation for pow(Base, x) (32-bit SIMD) */
template <typename Base, int order, bool C1_continuous>
xsimd::batch<float> pow (xsimd::batch<float> x)
{
    x = xsimd::max (xsimd::broadcast (-126.0f), Base::log2_base * x);

    const auto xi = xsimd::to_int (x);
    const auto l = xsimd::select (xsimd::batch_bool_cast<int32_t> (x < 0.0f), xi - 1, xi);
    const auto f = x - xsimd::to_float (l);
    const auto vi = (l + 127) << 23;

    return xsimd::bit_cast<xsimd::batch<float>> (vi) * pow_detail::pow2_approx<xsimd::batch<float>, order, C1_continuous> (f);
}

/** approximation for pow(Base, x) (64-bit SIMD) */
template <typename Base, int order, bool C1_continuous>
xsimd::batch<double> pow (xsimd::batch<double> x)
{
    x = xsimd::max (-1022.0, Base::log2_base * x);

    const auto xi = xsimd::to_int (x);
    const auto l = xsimd::select (xsimd::batch_bool_cast<int64_t> (x < 0.0), xi - 1, xi);
    const auto d = x - xsimd::to_float (l);
    const auto vi = (l + 1023) << 52;

    return xsimd::bit_cast<xsimd::batch<double>> (vi) * pow_detail::pow2_approx<xsimd::batch<double>, order, C1_continuous> (d);
}
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop // end ignore strict-aliasing warnings
#endif

/** Approximation of exp(x), using exp(x) = 2^floor(x * log2(e)) * 2^frac(x * log2(e)) */
template <int order, bool C1_continuous = false, typename T>
constexpr T exp (T x)
{
    return pow<pow_detail::BaseE<scalar_of_t<T>>, order, C1_continuous> (x);
}

/** Approximation of exp2(x), using exp(x) = 2^floor(x) * 2^frac(x) */
template <int order, bool C1_continuous = false, typename T>
constexpr T exp2 (T x)
{
    return pow<pow_detail::Base2<scalar_of_t<T>>, order, C1_continuous> (x);
}

/** Approximation of exp(x), using exp10(x) = 2^floor(x * log2(10)) * 2^frac(x * log2(10)) */
template <int order, bool C1_continuous = false, typename T>
constexpr T exp10 (T x)
{
    return pow<pow_detail::Base10<scalar_of_t<T>>, order, C1_continuous> (x);
}

/** Approximation of exp(1) - 1, using math_approx::exp(x) */
template <int order, bool C1_continuous = false, typename T>
constexpr T expm1 (T x)
{
    return pow<pow_detail::BaseE<scalar_of_t<T>>, order, C1_continuous> (x) - (T) 1;
}
}
