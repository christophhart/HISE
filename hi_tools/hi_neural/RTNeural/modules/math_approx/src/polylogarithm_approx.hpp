#pragma once

#include "basic_math.hpp"

namespace math_approx
{
/**
 * Approximation of the "dilogarithm" function for inputs
 * in the range [0, 1/2]. This method does not do any
 * bounds-checking.
 *
 * Orders higher than 3 are generally not recommended for
 * single-precision floating-point types, since they don't
 * improve the accuracy very much.
 *
 * For derivations, see notebooks/li2_approx.nb
 */
template <int order, typename T>
constexpr T li2_0_half (T x)
{
    static_assert (order >= 1 && order <= 6);
    using S = scalar_of_t<T>;

    if constexpr (order == 1)
    {
        const auto n_0 = (S) 0.996460629617;
        const auto d_0_1 = (S) 1 + (S) -0.288575624121 * x;
        return x * n_0 / d_0_1;
    }
    else if constexpr (order == 2)
    {
        const auto n_0_1 = (S) 0.999994847641 + (S) -0.546961998015 * x;
        const auto d_1_2 = (S) -0.797206910618 + (S) 0.0899936224040 * x;
        const auto d_0_1_2 = (S) 1 + d_1_2 * x;
        return x * n_0_1 / d_0_1_2;
    }
    else if constexpr (order == 3)
    {
        const auto x_sq = x * x;
        const auto n_0_2 = (S) 0.999999991192 + (S) 0.231155739205 * x_sq;
        const auto n_0_1_2 = n_0_2 + (S) -1.07612533343 * x;
        const auto d_2_3 = (S) 0.451592861555 + (S) -0.0281544399023 * x;
        const auto d_0_1 = (S) 1 + (S) -1.32612627824 * x;
        const auto d_0_1_2_3 = d_0_1 + d_2_3 * x_sq;
        return x * n_0_1_2 / d_0_1_2_3;
    }
    else if constexpr (order == 4)
    {
        const auto x_sq = x * x;
        const auto n_2_3 = (S) 0.74425269014090502911555775982556365472 + (S) -0.08749607277005140673532964399704145939 * x;
        const auto n_0_1 = (S) 0.99999999998544094594795118478024862055 + (S) -1.6098648159028159794757437744309391591 * x;
        const auto n_0_1_2_3 = n_0_1 + n_2_3 * x_sq;
        const auto d_3_4 = (S) -0.21787247785577362691148412819704459614 + (S) 0.00870385570778120787932426702624346169 * x;
        const auto d_1_2 = (S) -1.85986481869406218896935179306183665107 + (S) 1.09810787318601772062220747277929300408 * x;
        const auto d_1_2_3_4 = d_1_2 + d_3_4 * x_sq;
        const auto d_0_1_2_3_4 = (S) 1 + d_1_2_3_4 * x;
        return x * n_0_1_2_3 / d_0_1_2_3_4;
    }
    else if constexpr (order == 5)
    {
        const auto x_sq = x * x;

        const auto n_3_4 = (S) -0.41945653857264507277532555842378439927 + (S) 0.03140351694981020435408321943912212079 * x;
        const auto n_1_2 = (S) -2.14843104749890205674150618938194330623 + (S) 1.54956546570292751217524363072830456069 * x;
        const auto n_1_2_3_4 = n_1_2 + n_3_4 * x_sq;
        const auto n_0_1_2_3_4 = (S) 0.99999999999997312289180148636206726177 + n_1_2_3_4 * x;

        const auto d_4_5 = (S) 0.09609912057603552016206051904306797162 + (S) -0.00269129500193871901659324657805482418 * x;
        const auto d_2_3 = (S) 2.03806211686824385201410542913121040892 + (S) -0.72497973694183708484311198715866984035 * x;
        const auto d_0_1 = (S) 1 + (S) -2.398431047506893407956406025441134862 * x;
        const auto d_2_3_4_5 = d_2_3 + d_4_5 * x_sq;
        const auto d_0_1_2_3_4_5 = d_0_1 + d_2_3_4_5 * x_sq;

        return x * n_0_1_2_3_4 / d_0_1_2_3_4_5;
    }
    else if constexpr (order == 6)
    {
        const auto x_sq = x * x;

        const auto n_4_5 = (S) 0.20885966267164674441979654645138181067 + (S) -0.01085968986663512120143497781484214416 * x;
        const auto n_2_3 = (S) 2.64771686149306717256638234054408732899 + (S) -1.15385196641292513334184445301529897694 * x;
        const auto n_0_1 = (S) 0.99999999999999995022522902211061062582 + (S) -2.6883902117841251600624689886592808124 * x;
        const auto n_2_3_4_5 = n_2_3 + n_4_5 * x_sq;
        const auto n_0_1_2_3_4_5 = n_0_1 + n_2_3_4_5 * x_sq;

        const auto d_5_6 = (S) -0.03980108270103465616851961097089502921 + (S) 0.00082742905522813187941384917520432493 * x;
        const auto d_3_4 = (S) -1.70766499097900947314107956633154245176 + (S) 0.41595826557420951684124942212799147948 * x;
        const auto d_1_2 = (S) -2.93839021178414636324893816529360171731 + (S) 3.27120330332951521662427278605230451458 * x;
        const auto d_3_4_5_6 = d_3_4 + d_5_6 * x_sq;
        const auto d_0_1_2 = (S) 1 + d_1_2 * x;
        const auto d_0_1_2_3_4_5_6 = d_0_1_2 + d_3_4_5_6 * x_sq * x;

        return x * n_0_1_2_3_4_5 / d_0_1_2_3_4_5_6;
    }
    else
    {
        return {};
    }
}

/**
 * Approximation of the "dilogarithm" function for all inputs.
 *
 * Orders higher than 3 are generally not recommended for
 * single-precision floating-point types, since they don't
 * improve the accuracy very much.
 */
template <int order, int log_order = std::min (order + 2, 6), bool log_C1 = (log_order >= 5), typename T>
constexpr T li2 (T x)
{
    const auto x_r = (T) 1 / x;
    const auto x_r1 = (T) 1 / (x - (T) 1);

    constexpr auto pisq_o_6 = (T) M_PI * (T) M_PI / (T) 6;
    constexpr auto pisq_o_3 = (T) M_PI * (T) M_PI / (T) 3;

    T y, r;
    bool sign = true;
    if (x < (T) -1)
    {
        y = -x_r1;
        const auto l = log<log_order, log_C1> ((T) 1 - x);
        r = -pisq_o_6 + l * ((T) 0.5 * l - log<log_order, log_C1> (-x));
    }
    else if (x < (T) 0)
    {
        y = x * x_r1;
        const auto l = log<log_order, log_C1> ((T) 1 - x);
        r = (T) -0.5 * l * l;
        sign = false;
    }
    else if (x < (T) 0.5)
    {
        y = x;
        r = {};
    }
    else if (x < (T) 1)
    {
        y = (T) 1 - x;
        r = pisq_o_6 - log<log_order, log_C1> (x) * log<log_order, log_C1> (y);
        sign = false;
    }
    else if (x < (T) 2)
    {
        y = (T) 1 - x_r;
        const auto l = log<log_order, log_C1> (x);
        r = pisq_o_6 - l * (log<log_order, log_C1> (y) + (T) 0.5 * l);
    }
    else
    {
        y = x_r;
        const auto l = log<log_order, log_C1> (x);
        r = pisq_o_3 - (T) 0.5 * l * l;
        sign = false;
    }

    const auto li2_reduce = li2_0_half<order> (y);
    return r + select (sign, li2_reduce, -li2_reduce);
}

#if defined(XSIMD_HPP)
/**
 * Approximation of the "dilogarithm" function for all inputs.
 *
 * Orders higher than 3 are generally not recommended for
 * single-precision floating-point types, since they don't
 * improve the accuracy very much.
 */
template <int order, int log_order = std::min (order + 2, 6), bool log_C1 = (log_order >= 5), typename T>
xsimd::batch<T> li2 (const xsimd::batch<T>& x)
{
    // x < -1:
    // - log(-x) -> [1, inf]
    // - log(1-x) -> [2, inf]
    // x < 0:
    // - NOP
    // - log(1-x) -> [1, 2]
    // x < 1/2:
    // - NOP
    // - NOP
    // x < 1:
    // - log(x) -> [1/2, 1]
    // - log(1-x) -> [0, 1/2]
    // x < 2:
    // - log(x) -> [1, 2]
    // - log(1-1/x) -> [0, 1/2]
    // x >= 2:
    // - log(x) -> [2, inf]
    // - NOP

    const auto x_r = (T) 1 / x;
    const auto x_r1 = (T) 1 / (x - (T) 1);
    const auto log_arg1 = select (x < (T) -1, -x, select (x < (T) 0.5, xsimd::broadcast ((T) 1), x));
    const auto log_arg2 = select (x < (T) 1, (T) 1 - x, (T) 1 - x_r);

    const auto log1 = log<log_order, log_C1> (log_arg1);
    const auto log2 = log<log_order, log_C1> (log_arg2);

    // clang-format off
    const auto y = select (x < (T) -1, (T) -1 * x_r1,
                   select (x < (T) 0, x * x_r1,
                   select (x < (T) 0.5, x,
                   select (x < (T) 1, (T) 1 - x,
                   select (x < (T) 2, (T) 1 - x_r,
                       x_r)))));
    const auto sign = x < (T) -1 || (x >= (T) 0 && x < (T) 0.5) || (x >= (T) 1 && x < (T) 2);

    static constexpr auto pisq_o_6 = (T) M_PI * (T) M_PI / (T) 6;
    static constexpr auto pisq_o_3 = (T) M_PI * (T) M_PI / (T) 3;
    const auto log1_log2 = log1 * log2;
    const auto half_log1_sq = (T) 0.5 * log1 * log1;
    const auto half_log2_sq = (T) 0.5 * log2 * log2;
    const auto r = select (x < (T) -1, -pisq_o_6 + half_log2_sq - log1_log2,
                   select (x < (T) 0, -half_log2_sq,
                   select (x < (T) 0.5, xsimd::broadcast ((T) 0),
                   select (x < (T) 1, pisq_o_6 - log1_log2,
                   select (x < (T) 2, pisq_o_6 - log1_log2 - half_log1_sq,
                       pisq_o_3 - half_log1_sq)))));
    //clang-format on

    const auto li2_reduce = li2_0_half<order> (y);
    return r + select (sign, li2_reduce, -li2_reduce);
}
#endif
} // namespace math_approx
