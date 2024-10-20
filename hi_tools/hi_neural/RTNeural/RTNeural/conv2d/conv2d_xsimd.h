#ifndef RTNEURAL_CONV2D_XSIMD_H
#define RTNEURAL_CONV2D_XSIMD_H

#include "../Layer.h"
#include "../config.h"
#include "../conv1d_stateless/conv1d_stateless.h"
#include "../../modules/xsimd/xsimd.hpp"

namespace RTNEURAL_NAMESPACE
{
/**
 * Dynamic implementation of a 2-dimensional convolution layer with no activation.
 *
 * @tparam T Type of the layer (float, double, int ...)
 */
template <typename T>
class Conv2D : public Layer<T>
{
public:
    /**
     * @param in_num_filters_in number of input filters (channels)
     * @param in_num_filters_out number of output filters (channels)
     * @param in_num_features_in number of input features
     * @param in_kernel_size_time size of the convolution kernel (time axis)
     * @param in_kernel_size_feature size of the convolution kernel (feature axis)
     * @param in_dilation_rate dilation_rate (time axis)
     * @param in_stride convolution stride (feature axis)
     * @param in_valid_pad whether the padding is "valid" or not ("same" otherwise)
     */
    Conv2D(int in_num_filters_in, int in_num_filters_out, int in_num_features_in, int in_kernel_size_time, int in_kernel_size_feature, int in_dilation_rate, int in_stride, bool in_valid_pad);
    Conv2D(std::initializer_list<int> sizes);
    Conv2D(const Conv2D& other);
    Conv2D& operator=(const Conv2D& other);
    virtual ~Conv2D() = default;

    /** Reset the layer's state */
    RTNEURAL_REALTIME void reset() override
    {
        state_index = 0;

        for(int i = 0; i < receptive_field; i++)
        {
            std::fill(state[i].begin(), state[i].end(), (T)0);
        }
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "conv2d"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* output) noexcept override
    {
        for(int i = 0; i < kernel_size_time; ++i)
        {
            int state_idx_to_use = (state_index + (receptive_field - 1) - i * dilation_rate) % receptive_field;

            conv1dLayers[i].forward(input, state[state_idx_to_use].data());
        }

        for(int i = 0; i < num_features_out; ++i)
        {
            const auto* stateCol = state[state_index].data() + i * num_filters_out;
            auto* outCol = output + i * num_filters_out;
            xsimd::transform(stateCol, stateCol + num_filters_out, bias.begin(), outCol, [](auto a, auto b)
                { return a + b; });
        }

        std::fill(state[state_index].begin(), state[state_index].end(), (T)0);
        state_index = state_index == receptive_field - 1 ? 0 : state_index + 1;
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[num_filters_out][num_filters_in][kernel_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<std::vector<T>>>>& inWeights);

    /**
     * Sets the layer biases.
     *
     * The bias vector must have size bias[num_filters_out]
     */
    RTNEURAL_REALTIME void setBias(const std::vector<T>& inBias);

    /** Returns the size of the convolution kernel (time axis). */
    RTNEURAL_REALTIME int getKernelSizeTime() const noexcept { return kernel_size_time; }

    /** Returns the size of the convolution kernel (feature axis). */
    RTNEURAL_REALTIME int getKernelSizeFeature() const noexcept { return kernel_size_feature; }

    /** Returns the convolution stride (feature axis) */
    RTNEURAL_REALTIME int getStride() const noexcept { return stride; }

    /** Returns the convolution dilation rate (time axis) */
    RTNEURAL_REALTIME int getDilationRate() const noexcept { return dilation_rate; }

    const int num_filters_in;
    const int num_features_in;
    const int num_filters_out;
    const int kernel_size_time;
    const int kernel_size_feature;
    const int dilation_rate;
    const int stride;
    const int num_features_out;
    const int receptive_field;
    const bool valid_pad;

private:
    std::vector<Conv1DStateless<T>> conv1dLayers;

    std::vector<std::vector<T, xsimd::aligned_allocator<T>>> state;

    int state_index = 0;

    std::vector<T, xsimd::aligned_allocator<T>> bias;
};

//====================================================

/**
 * Static implementation of a 2-dimensional convolution layer with no activation.
 *
 * @tparam T Type of the layer (float, double, int ...)
 * @tparam num_filters_in_t number of input filters (channels)
 * @tparam num_filters_out_t number of output filters (channels)
 * @tparam num_features_in_t number of input features
 * @tparam kernel_size_time_t size of the convolution kernel (time axis)
 * @tparam kernel_size_feature_t size of the convolution kernel (feature axis)
 * @tparam dilation_rate_t dilation_rate (time axis)
 * @tparam stride_t convolution stride (feature axis)
 * @tparam valid_pad_t if true: pad is "valid". if false: pad is "same"
 */
template <typename T, int num_filters_in_t, int num_filters_out_t, int num_features_in_t, int kernel_size_time_t,
    int kernel_size_feature_t, int dilation_rate_t, int stride_t, bool valid_pad_t>
class Conv2DT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_num_filters_in = ceil_div(num_filters_in_t, v_size);
    static constexpr auto v_num_filters_out = ceil_div(num_filters_out_t, v_size);
    static constexpr auto v_in_size = v_num_filters_in * num_features_in_t;

public:
    static constexpr int num_features_out = Conv1DStateless<T>::computeNumFeaturesOut(num_features_in_t, kernel_size_feature_t, stride_t, valid_pad_t);
    static constexpr auto in_size = num_filters_in_t * num_features_in_t;
    static constexpr auto out_size = num_filters_out_t * num_features_out;
    static constexpr auto v_out_size = v_num_filters_out * num_features_out;

    using output_type = std::array<v_type, v_out_size>;

    static constexpr int receptive_field = 1 + (kernel_size_time_t - 1) * dilation_rate_t;
    static constexpr int num_filters_in = num_filters_in_t;
    static constexpr int num_features_in = num_features_in_t;
    static constexpr int num_filters_out = num_filters_out_t;
    static constexpr int kernel_size_time = kernel_size_time_t;
    static constexpr int kernel_size_feature = kernel_size_feature_t;
    static constexpr int dilation_rate = dilation_rate_t;
    static constexpr int stride = stride_t;
    static constexpr bool valid_pad = valid_pad_t;

    Conv2DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "conv2d"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Reset the layer's state */
    RTNEURAL_REALTIME void reset()
    {
        state_index = 0;

        for(int i = 0; i < receptive_field; i++)
        {
            std::fill(state[i].begin(), state[i].end(), (T)0);
        }
    }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_in_size]) noexcept
    {
        for(int i = 0; i < kernel_size_time; ++i)
        {
            int state_idx_to_use = (state_index + (receptive_field - 1) - i * dilation_rate) % receptive_field;

            std::fill(std::begin(conv1dLayers[i].outs), std::end(conv1dLayers[i].outs), (T)0);
            conv1dLayers[i].forward(ins);

            for(int j = 0; j < state[state_idx_to_use].size(); ++j)
                state[state_idx_to_use][j] += conv1dLayers[i].outs[j];
        }

        for(int i = 0; i < num_features_out; ++i)
        {
            for(int j = 0; j < v_num_filters_out; ++j)
            {
                outs[i * v_num_filters_out + j] = state[state_index][i * v_num_filters_out + j] + bias[j];
            }
        }

        std::fill(state[state_index].begin(), state[state_index].end(), (T)0);
        state_index = state_index == receptive_field - 1 ? 0 : state_index + 1;
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights [kernel_size_time][num_filters_out][num_filters_in][kernel_size_feature]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<std::vector<T>>>>& inWeights);

    /**
     * Sets the layer biases.
     *
     * The bias vector must have size bias[num_filters_out]
     */
    RTNEURAL_REALTIME void setBias(const std::vector<T>& inBias);

    /** Returns the size of the convolution kernel (time axis). */
    RTNEURAL_REALTIME int getKernelSizeTime() const noexcept { return kernel_size_time_t; }

    /** Returns the size of the convolution kernel (feature axis). */
    RTNEURAL_REALTIME int getKernelSizeFeature() const noexcept { return kernel_size_feature_t; }

    /** Returns the convolution stride */
    RTNEURAL_REALTIME int getStride() const noexcept { return stride_t; }

    /** Returns the convolution dilation rate */
    RTNEURAL_REALTIME int getDilationRate() const noexcept { return dilation_rate_t; }

    v_type outs[v_out_size];

private:
    std::array<Conv1DStatelessT<T, num_filters_in_t, num_features_in_t, num_filters_out_t, kernel_size_feature_t, stride_t, valid_pad_t>,
        kernel_size_time_t>
        conv1dLayers;

    std::array<output_type, receptive_field> state;

    int state_index = 0;

    v_type bias[v_num_filters_out];
};

} // RTNEURAL

#endif // RTNEURAL_CONV2D_XSIMD_H
