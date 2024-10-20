#pragma once

#include <cmath>

namespace RTNEURAL_NAMESPACE
{
struct DefaultMathsProvider
{
    template <typename Matrix>
    static auto tanh(const Matrix& x)
    {
        return x.array().tanh();
    }

    template <typename Matrix>
    static auto sigmoid(const Matrix& x)
    {
        using T = typename Matrix::Scalar;
        return (T)1 / (((T)-1 * x.array()).array().exp() + (T)1);
    }

    template <typename Matrix>
    static auto exp(const Matrix& x)
    {
        return x.array().exp();
    }
};
}
