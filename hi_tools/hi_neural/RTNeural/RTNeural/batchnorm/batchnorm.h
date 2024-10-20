#ifndef BATCHNORM_H_INCLUDED
#define BATCHNORM_H_INCLUDED

#if RTNEURAL_USE_EIGEN
#include "batchnorm_eigen.h"
#include "batchnorm_eigen.tpp"
#elif RTNEURAL_USE_XSIMD
#include "batchnorm_xsimd.h"
#include "batchnorm_xsimd.tpp"
#else
#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include <vector>

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
        for(int i = 0; i < Layer<T>::out_size; ++i)
            out[i] = multiplier[i] * (input[i] - running_mean[i]) + beta[i];
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

    std::vector<T> gamma;
    std::vector<T> beta;

    std::vector<T> running_mean;
    std::vector<T> running_var;

    std::vector<T> multiplier;

    T epsilon = (T)0;
};

/** Static batch normalization layer. */
template <typename T, int size, bool affine = true>
class BatchNorm1DT
{
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
    forward(const T (&ins)[in_size]) noexcept
    {
        for(int i = 0; i < size; ++i)
            outs[i] = multiplier[i] * (ins[i] - running_mean[i]) + beta[i];
    }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<!isAffine, void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        for(int i = 0; i < size; ++i)
            outs[i] = multiplier[i] * (ins[i] - running_mean[i]);
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

    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

private:
    void updateMultiplier();

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T gamma[out_size];
    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T beta[out_size];

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T running_mean[out_size];
    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T running_var[out_size];

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T multiplier[out_size];

    T epsilon = (T)0;
};
}

#endif // RTNEURAL_STL

#endif // BATCHNORM_H_INCLUDED
