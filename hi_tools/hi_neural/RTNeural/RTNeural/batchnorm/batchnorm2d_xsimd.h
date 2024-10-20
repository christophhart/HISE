#ifndef RTNEURAL_BATCHNORM2D_XSIMD_H
#define RTNEURAL_BATCHNORM2D_XSIMD_H

#include "../Layer.h"
#include "../config.h"
#include "../../modules/xsimd/xsimd.hpp"

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
            const auto* inCol = input + i * num_filters;
            auto* outCol = out + i * num_filters;
            xsimd::transform(inCol, inCol + num_filters, running_mean.begin(), outCol,
                [](auto const& a, auto const& b)
                { return a - b; });
            xsimd::transform(outCol, outCol + num_filters, multiplier.begin(), outCol,
                [](auto const& a, auto const& b)
                { return a * b; });
            xsimd::transform(outCol, outCol + num_filters, beta.begin(), outCol,
                [](auto const& a, auto const& b)
                { return a + b; });
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

    using vec_type = std::vector<T, xsimd::aligned_allocator<T>>;

    vec_type gamma;
    vec_type beta;

    vec_type running_mean;
    vec_type running_var;

    vec_type multiplier;

    T epsilon = (T)0;
};

/** Static batch normalization layer. */
template <typename T, int num_filters_t, int num_features_t, bool affine = true>
class BatchNorm2DT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_num_filters = ceil_div(num_filters_t, v_size);
    static constexpr auto v_io_size = v_num_filters * num_features_t;

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
    forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < num_features; i++)
        {
            for(int j = 0; j < v_num_filters; ++j)
            {
                outs[i * v_num_filters + j] = (ins[i * v_num_filters + j] - running_mean[j]) * multiplier[j] + beta[j];
            }
        }
    }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<!isAffine, void>::type
    forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < num_features; i++)
        {
            for(int j = 0; j < v_num_filters; ++j)
            {
                outs[i * v_num_filters + j] = (ins[i * v_num_filters + j] - running_mean[j]) * multiplier[j];
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

    v_type outs[v_io_size];

private:
    void updateMultiplier();

    v_type gamma[v_num_filters];
    v_type beta[v_num_filters];

    v_type running_mean[v_num_filters];
    v_type running_var[v_num_filters];

    v_type multiplier[v_num_filters];

    T epsilon = (T)0;
};
}

#endif // RTNEURAL_BATCHNORM2D_XSIMD_H
