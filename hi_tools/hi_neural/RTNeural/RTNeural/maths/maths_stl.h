#pragma once

#include <cmath>

namespace RTNEURAL_NAMESPACE
{
struct DefaultMathsProvider
{
    template <typename T>
    static T tanh(T x)
    {
        return std::tanh(x);
    }

    template <typename T>
    static T sigmoid(T x)
    {
        return (T)1 / ((T)1 + std::exp(-x));
    }

    template <typename T>
    static T exp(T x)
    {
        return std::exp(x);
    }
};
}
