#pragma once

#include "basic_math.hpp"
#include "pow_approx.hpp"

namespace math_approx
{
namespace log_detail
{
    struct Log2Provider
    {
        // for polynomial derivations, see notebooks/log_approx.nb

        /** approximation for log2(x), optimized on the range [1, 2] */
        template <typename T, int order, bool C1_continuous>
        static constexpr T log2_approx (T x)
        {
            static_assert (order >= 3 && order <= 6);
            using S = scalar_of_t<T>;

            const auto x_sq = x * x;
            if constexpr (C1_continuous)
            {
                if constexpr (order == 3)
                {
                    const auto x_2_3 = (S) -1.09886528622 + (S) 0.164042561333 * x;
                    const auto x_0_1 = (S) -2.21347520444 + (S) 3.14829792933 * x;
                    return x_0_1 + x_2_3 * x_sq;
                }
                else if constexpr (order == 4)
                {
                    const auto x_3_4 = (S) 0.671618567027 + (S) -0.0845960009489 * x;
                    const auto x_1_2 = (S) 4.16344994072 + (S) -2.19861329856 * x;
                    const auto x_1_2_3_4 = x_1_2 + x_3_4 * x_sq;
                    return (S) -2.55185920824 + x_1_2_3_4 * x;
                }
                else if constexpr (order == 5)
                {
                    const auto x_4_5 = (S) -0.432338320780 + (S) 0.0464481811023 * x;
                    const auto x_2_3 = (S) -3.65368350361 + (S) 1.68976432066 * x;
                    const auto x_0_1 = (S) -2.82807214111 + (S) 5.17788146374 * x;
                    const auto x_2_3_4_5 = x_2_3 + x_4_5 * x_sq;
                    return x_0_1 + x_2_3_4_5 * x_sq;
                }
                else if constexpr (order == 6)
                {
                    const auto x_5_6 = (S) 0.284794437502 + (S) -0.0265448504094 * x;
                    const auto x_3_4 = (S) 3.38542517475 + (S) -1.31007090775 * x;
                    const auto x_1_2 = (S) 6.19242937536 + (S) -5.46521465640 * x;
                    const auto x_3_4_5_6 = x_3_4 + x_5_6 * x_sq;
                    const auto x_1_2_3_4_5_6 = x_1_2 + x_3_4_5_6 * x_sq;
                    return (S) -3.06081857306 + x_1_2_3_4_5_6 * x;
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
                    const auto x_2_3 = (S) -1.05974531422 + (S) 0.159220010975 * x;
                    const auto x_0_1 = (S) -2.16417056258 + (S) 3.06469586582 * x;
                    return x_0_1 + x_2_3 * x_sq;
                }
                else if constexpr (order == 4)
                {
                    const auto x_3_4 = (S) 0.649709537672 + (S) -0.0821303550902 * x;
                    const auto x_1_2 = (S) 4.08637809379 + (S) -2.13412984371 * x;
                    const auto x_1_2_3_4 = x_1_2 + x_3_4 * x_sq;
                    return (S) -2.51982743265 + x_1_2_3_4 * x;
                }
                else if constexpr (order == 5)
                {
                    const auto x_4_5 = (S) -0.419319345483 + (S) 0.0451488402558 * x;
                    const auto x_2_3 = (S) -3.56885211615 + (S) 1.64139451414 * x;
                    const auto x_0_1 = (S) -2.80534277658 + (S) 5.10697088382 * x;
                    const auto x_2_3_4_5 = x_2_3 + x_4_5 * x_sq;
                    return x_0_1 + x_2_3_4_5 * x_sq;
                }
                else if constexpr (order == 6)
                {
                    const auto x_5_6 = (S) 0.276834061071 + (S) -0.0258400886535 * x;
                    const auto x_3_4 = (S) 3.30388341157 + (S) -1.27446900713 * x;
                    const auto x_1_2 = (S) 6.12708086513 + (S) -5.36371998242 * x;
                    const auto x_3_4_5_6 = x_3_4 + x_5_6 * x_sq;
                    const auto x_1_2_3_4_5_6 = x_1_2 + x_3_4_5_6 * x_sq;
                    return (S) -3.04376925958 + x_1_2_3_4_5_6 * x;
                }
                else
                {
                    return {};
                }
            }
        }
    };
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing" // these methods require some type-punning
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

/** approximation for log(Base, x) (32-bit) */
template <typename Base, int order, bool C1_continuous, typename Log2ProviderType = log_detail::Log2Provider>
constexpr float log (float x)
{
    const auto vi = bit_cast<int32_t> (x);
    const auto ex = vi & 0x7f800000;
    const auto e = (ex >> 23) - 127;
    const auto vfi = (vi - ex) | 0x3f800000;
    const auto vf = bit_cast<float> (vfi);

    constexpr auto log2_base_r = 1.0f / Base::log2_base;
    return log2_base_r * ((float) e + Log2ProviderType::template log2_approx<float, order, C1_continuous> (vf));
}

/** approximation for log(x) (64-bit) */
template <typename Base, int order, bool C1_continuous, typename Log2ProviderType = log_detail::Log2Provider>
constexpr double log (double x)
{
    const auto vi = bit_cast<int64_t> (x);
    const auto ex = vi & 0x7ff0000000000000;
    const auto e = (ex >> 52) - 1023;
    const auto vfi = (vi - ex) | 0x3ff0000000000000;
    const auto vf = bit_cast<double> (vfi);

    constexpr auto log2_base_r = 1.0 / Base::log2_base;
    return log2_base_r * ((double) e + Log2ProviderType::template log2_approx<double, order, C1_continuous> (vf));
}

#if defined(XSIMD_HPP)
/** approximation for pow(Base, x) (32-bit SIMD) */
template <typename Base, int order, bool C1_continuous, typename Log2ProviderType = log_detail::Log2Provider>
xsimd::batch<float> log (xsimd::batch<float> x)
{
    const auto vi = xsimd::bit_cast<xsimd::batch<int32_t>> (x);
    const auto ex = vi & 0x7f800000;
    const auto e = (ex >> 23) - 127;
    const auto vfi = (vi - ex) | 0x3f800000;
    const auto vf = xsimd::bit_cast<xsimd::batch<float>> (vfi);

    static constexpr auto log2_base_r = 1.0f / Base::log2_base;
    return log2_base_r * (xsimd::to_float (e) + Log2ProviderType::template log2_approx<xsimd::batch<float>, order, C1_continuous> (vf));
}

/** approximation for pow(Base, x) (64-bit SIMD) */
template <typename Base, int order, bool C1_continuous, typename Log2ProviderType = log_detail::Log2Provider>
xsimd::batch<double> log (xsimd::batch<double> x)
{
    const auto vi = xsimd::bit_cast<xsimd::batch<int64_t>> (x);
    const auto ex = vi & 0x7ff0000000000000;
    const auto e = (ex >> 52) - 1023;
    const auto vfi = (vi - ex) | 0x3ff0000000000000;
    const auto vf = xsimd::bit_cast<xsimd::batch<double>> (vfi);

    static constexpr auto log2_base_r = 1.0 / Base::log2_base;
    return log2_base_r * (xsimd::to_float (e) + Log2ProviderType::template log2_approx<xsimd::batch<double>, order, C1_continuous> (vf));
}
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop // end ignore strict-aliasing warnings
#endif

/**
 * Approximation of log(x), using
 * log(x) = (1 / log2(e)) * (Exponent(x) + log2(1 + Mantissa(x))
 */
template <int order, bool C1_continuous = false, typename T>
constexpr T log (T x)
{
    return log<pow_detail::BaseE<scalar_of_t<T>>, order, C1_continuous> (x);
}

/**
 * Approximation of log2(x), using
 * log2(x) = Exponent(x) + log2(1 + Mantissa(x)
 */
template <int order, bool C1_continuous = false, typename T>
constexpr T log2 (T x)
{
    return log<pow_detail::Base2<scalar_of_t<T>>, order, C1_continuous> (x);
}

/**
 * Approximation of log10(x), using
 * log10(x) = (1 / log2(10)) * (Exponent(x) + log2(1 + Mantissa(x))
 */
template <int order, bool C1_continuous = false, typename T>
constexpr T log10 (T x)
{
    return log<pow_detail::Base10<scalar_of_t<T>>, order, C1_continuous> (x);
}

/** Approximation of log(1 + x), using math_approx::log(x) */
template <int order, bool C1_continuous = false, typename T>
constexpr T log1p (T x)
{
    return log<pow_detail::BaseE<scalar_of_t<T>>, order, C1_continuous> ((T) 1 + x);
}
}
