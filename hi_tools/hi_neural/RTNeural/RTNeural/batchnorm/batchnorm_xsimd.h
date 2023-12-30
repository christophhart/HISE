#ifndef BATCHNORMXSIMD_H_INCLUDED
#define BATCHNORMXSIMD_H_INCLUDED

#include "../Layer.h"
#include "../config.h"
#include "../../modules/xsimd/xsimd.hpp"

namespace RTNEURAL_NAMESPACE
{
/** Dynamic batch normalization layer. */
template <typename T>
class BatchNorm1DLayer final : public Layer<T>
{
public:
    explicit BatchNorm1DLayer(int size);

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "batchnorm"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        xsimd::transform(input, input + Layer<T>::in_size, running_mean.begin(), out,
            [](auto const& a, auto const& b)
            { return a - b; });
        xsimd::transform(out, out + Layer<T>::in_size, multiplier.begin(), out,
            [](auto const& a, auto const& b)
            { return a * b; });
        xsimd::transform(out, out + Layer<T>::in_size, beta.begin(), out,
            [](auto const& a, auto const& b)
            { return a + b; });
    }

    /** Sets the layer "gamma" values. */
    RTNEURAL_REALTIME void setGamma(const std::vector<T>& gammaVals);

    /** Sets the layer "beta" values. */
    RTNEURAL_REALTIME void setBeta(const std::vector<T>& betaVals);

    /** Sets the layer's trained running mean. */
    RTNEURAL_REALTIME void setRunningMean(const std::vector<T>& runningMean);

    /** Set's the layer's trained running variance. */
    RTNEURAL_REALTIME void setRunningVariance(const std::vector<T>& runningVar);

    /** Set's the layer "epsilon" value. */
    RTNEURAL_REALTIME void setEpsilon(T epsilon);

private:
    void updateMultiplier();

    using vec_type = std::vector<T, xsimd::aligned_allocator<T>>;

    vec_type gamma;
    vec_type beta;

    vec_type running_mean;
    vec_type running_var;

    vec_type multiplier;

    T epsilon = (T)0;
};

/** Static batch normalization layer. */
template <typename T, int size, bool affine = true>
class BatchNorm1DT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_out_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;
    static constexpr bool is_affine = affine;

    BatchNorm1DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "batchnorm"; }

    /** Returns false since batch-norm is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<isAffine, void>::type
    forward(const v_type (&ins)[v_out_size]) noexcept
    {
        for(int k = 0; k < v_out_size; ++k)
            outs[k] = multiplier[k] * (ins[k] - running_mean[k]) + beta[k];
    }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<!isAffine, void>::type
    forward(const v_type (&ins)[v_out_size]) noexcept
    {
        for(int k = 0; k < v_out_size; ++k)
            outs[k] = multiplier[k] * (ins[k] - running_mean[k]);
    }

    /** Sets the layer "gamma" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<isAffine, void>::type setGamma(const std::vector<T>& gammaVals);

    /** Sets the layer "gamma" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<!isAffine, void>::type setGamma(const std::vector<T>&) { }

    /** Sets the layer "beta" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<isAffine, void>::type setBeta(const std::vector<T>& betaVals);

    /** Sets the layer "beta" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<!isAffine, void>::type setBeta(const std::vector<T>&) { }

    /** Sets the layer's trained running mean. */
    RTNEURAL_REALTIME void setRunningMean(const std::vector<T>& runningMean);

    /** Set's the layer's trained running variance. */
    RTNEURAL_REALTIME void setRunningVariance(const std::vector<T>& runningVar);

    /** Set's the layer "epsilon" value. */
    RTNEURAL_REALTIME void setEpsilon(T epsilon);

    v_type outs[v_out_size];

private:
    void updateMultiplier();

    v_type gamma[v_out_size];
    v_type beta[v_out_size];

    v_type running_mean[v_out_size];
    v_type running_var[v_out_size];

    v_type multiplier[v_out_size];

    T epsilon = (T)0;
};
}

#endif // BATCHNORMXSIMD_H_INCLUDED
