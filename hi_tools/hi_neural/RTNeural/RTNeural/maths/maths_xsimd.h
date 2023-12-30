#pragma once

#include <cmath>

namespace RTNEURAL_NAMESPACE
{
struct DefaultMathsProvider
{
    template <typename T>
    static T tanh(T x)
    {
        using std::tanh;
        using xsimd::tanh;
        return tanh(x);
    }

    template <typename T>
    static T sigmoid(T x)
    {
        using std::exp;
        using xsimd::exp;
        return (T)1 / ((T)1 + exp(-x));
    }

    template <typename T>
    static T exp(T x)
    {
        using std::exp;
        using xsimd::exp;
        return exp(x);
    }
};
}
