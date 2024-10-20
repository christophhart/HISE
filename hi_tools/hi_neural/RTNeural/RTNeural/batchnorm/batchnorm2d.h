#ifndef BATCHNORM2D_H_INCLUDED
#define BATCHNORM2D_H_INCLUDED

#if RTNEURAL_USE_EIGEN
#include "batchnorm2d_eigen.h"
#include "batchnorm2d_eigen.tpp"
#elif RTNEURAL_USE_XSIMD
#include "batchnorm2d_xsimd.h"
#include "batchnorm2d_xsimd.tpp"
#else
#include "../Layer.h"
#include "../config.h"

namespace RTNEURAL_NAMESPACE
{
/** Dynamic batch normalization layer. */
template <typename T>
class BatchNorm2DLayer final : public Layer<T>
{
public:
    BatchNorm2DLayer(int num_filters, int num_features);

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "batchnorm2d"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        for(int i = 0; i < num_features; i++)
        {
            for(int j = 0; j < num_filters; ++j)
            {
                out[i * num_filters + j] = (input[i * num_filters + j] - running_mean[j]) * multiplier[j] + beta[j];
            }
        }
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

    const int num_filters;
    const int num_features;

    std::vector<T> gamma;
    std::vector<T> beta;

    std::vector<T> running_mean;
    std::vector<T> running_var;

    std::vector<T> multiplier;

    T epsilon = (T)0;
};

/** Static batch normalization layer. */
template <typename T, int num_filters_t, int num_features_t, bool affine = true>
class BatchNorm2DT
{
public:
    static constexpr auto in_size = num_filters_t * num_features_t;
    static constexpr auto out_size = num_filters_t * num_features_t;
    static constexpr auto num_filters = num_filters_t;
    static constexpr auto num_features = num_features_t;
    static constexpr bool is_affine = affine;

    BatchNorm2DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "batchnorm2d"; }

    /** Returns false since batch-norm is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<isAffine, void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        for(int i = 0; i < num_features; i++)
        {
            for(int j = 0; j < num_filters; ++j)
            {
                outs[i * num_filters + j] = (ins[i * num_filters + j] - running_mean[j]) * multiplier[j] + beta[j];
            }
        }
    }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<!isAffine, void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        for(int i = 0; i < num_features; i++)
        {
            for(int j = 0; j < num_filters; ++j)
            {
                outs[i * num_filters + j] = (ins[i * num_filters + j] - running_mean[j]) * multiplier[j];
            }
        }
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

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T gamma[num_filters_t];
    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T beta[num_filters_t];

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T running_mean[num_filters_t];
    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T running_var[num_filters_t];

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) T multiplier[num_filters_t];

    T epsilon = (T)0;
};
}

#endif // RTNEURAL_USE_STL

#endif // BATCHNORM2D_H_INCLUDED
