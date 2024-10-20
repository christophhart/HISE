#ifndef RTNEURAL_CONV1D_STATELESS_XSIMD_H
#define RTNEURAL_CONV1D_STATELESS_XSIMD_H

#include "../Layer.h"
#include "../config.h"
#include "../../modules/xsimd/xsimd.hpp"

namespace RTNEURAL_NAMESPACE
{
/**
 * Dynamic implementation of a 1-dimensional stateless convolution layer with no activation.
 * This implementation was designed to be used for a single frame of features, fully available at each forward call.
 * So the layer has a NO internal "state"
 *
 * @tparam T Type of the layer (float, double, int ...)
 */
template <typename T>
class Conv1DStateless : public Layer<T>
{
public:
    Conv1DStateless(int in_num_filters_in, int in_num_features_in, int in_num_filters_out, int in_kernel_size, int in_stride, bool in_valid_pad);
    Conv1DStateless(std::initializer_list<int> sizes);
    Conv1DStateless(const Conv1DStateless& other);
    Conv1DStateless& operator=(const Conv1DStateless& other);
    virtual ~Conv1DStateless() = default;

    static constexpr int computeNumFeaturesOut(int num_features_in, int kernel_size, int stride, int valid_pad)
    {
        // Based on tensorflow docs: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
        // Custom implementation of ceil since std::ceil is not constexpr.

        if(valid_pad)
        {
            float f = static_cast<float>(num_features_in - kernel_size + 1) / static_cast<float>(stride);
            int i = static_cast<int>(f);
            return f > static_cast<float>(i) ? i + 1 : i;
        }

        float f = static_cast<float>(num_features_in) / static_cast<float>(stride);
        int i = static_cast<int>(f);
        return f > static_cast<float>(i) ? i + 1 : i;
    }

    static constexpr int computePadLeft(int num_features_in, int kernel_size, int stride, bool valid_pad)
    {
        // Based on tensorflow: Based on tensorflow: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2ow: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
        return valid_pad ? 0 : std::max(num_features_in % stride == 0 ? kernel_size - stride : kernel_size - num_features_in % stride, 0) / 2;
    }

    static constexpr int computePadRight(int num_features_in, int kernel_size, int stride, bool valid_pad)
    {
        // Based on tensorflow: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
        if(!valid_pad)
        {
            int total_pad = std::max(num_features_in % stride == 0 ? kernel_size - stride : kernel_size - num_features_in % stride, 0);
            return total_pad - total_pad / 2;
        }

        return 0;
    }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() override { }

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "conv1d_stateless"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* output) noexcept override
    {
        if(valid_pad)
        {
            for(int out_row_idx = 0; out_row_idx < num_filters_out; ++out_row_idx)
            {
                for(int out_col_idx = 0; out_col_idx < num_features_out; ++out_col_idx)
                {
                    T sum {};
                    for(int in_col_idx = out_col_idx * stride; in_col_idx < out_col_idx * stride + kernel_size; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx - out_col_idx * stride;
                        xsimd::transform(kernelWeights[out_row_idx][kernel_col_idx].begin(),
                            kernelWeights[out_row_idx][kernel_col_idx].end(),
                            input + in_col_idx * num_filters_in,
                            scratch.begin(),
                            [](auto a, auto b)
                            { return a * b; });
                        sum += xsimd::reduce(scratch.begin(), scratch.end(), T {});
                    }
                    output[out_col_idx * num_filters_out + out_row_idx] += sum;
                }
            }
        }
        else
        {
            for(int out_row_idx = 0; out_row_idx < num_filters_out; ++out_row_idx)
            {
                int out_col_idx = 0;

                for(; out_col_idx * stride < pad_left; ++out_col_idx)
                {
                    T sum {};
                    const int eff_kernel_size = kernel_size - pad_left + out_col_idx * stride;
                    for(int in_col_idx = 0; in_col_idx < eff_kernel_size; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx + (kernel_size - eff_kernel_size);
                        xsimd::transform(kernelWeights[out_row_idx][kernel_col_idx].begin(),
                            kernelWeights[out_row_idx][kernel_col_idx].end(),
                            input + in_col_idx * num_filters_in,
                            scratch.begin(),
                            [](auto a, auto b)
                            { return a * b; });
                        sum += xsimd::reduce(scratch.begin(), scratch.end(), T {});
                    }
                    output[out_col_idx * num_filters_out + out_row_idx] += sum;
                }

                for(; out_col_idx * stride - pad_left + kernel_size < num_features_in; ++out_col_idx)
                {
                    T sum {};
                    for(int in_col_idx = out_col_idx * stride - pad_left; in_col_idx < out_col_idx * stride - pad_left + kernel_size; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx - (out_col_idx * stride - pad_left);
                        xsimd::transform(kernelWeights[out_row_idx][kernel_col_idx].begin(),
                            kernelWeights[out_row_idx][kernel_col_idx].end(),
                            input + in_col_idx * num_filters_in,
                            scratch.begin(),
                            [](auto a, auto b)
                            { return a * b; });
                        sum += xsimd::reduce(scratch.begin(), scratch.end(), T {});
                    }
                    output[out_col_idx * num_filters_out + out_row_idx] += sum;
                }

                for(; out_col_idx * stride - pad_left + kernel_size <= num_features_in + pad_right; ++out_col_idx)
                {
                    T sum {};
                    const int eff_kernel_size = num_features_in - (out_col_idx * stride - pad_left);
                    for(int in_col_idx = (num_features_in - eff_kernel_size); in_col_idx < num_features_in; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx - (num_features_in - eff_kernel_size);
                        xsimd::transform(kernelWeights[out_row_idx][kernel_col_idx].begin(),
                            kernelWeights[out_row_idx][kernel_col_idx].end(),
                            input + in_col_idx * num_filters_in,
                            scratch.begin(),
                            [](auto a, auto b)
                            { return a * b; });
                        sum += xsimd::reduce(scratch.begin(), scratch.end(), T {});
                    }
                    output[out_col_idx * num_filters_out + out_row_idx] += sum;
                }
            }
        }
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[num_filters_out][num_filters_in][kernel_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& inWeights);

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size; }

    /** Returns the stride. */
    RTNEURAL_REALTIME int getStride() const noexcept { return stride; }

private:
    const int num_filters_in;
    const int num_features_in;
    const int num_filters_out;
    const int kernel_size;
    const int stride;
    const int num_features_out;
    const bool valid_pad;
    const int pad_left;
    const int pad_right;

    using Matrix = std::vector<std::vector<T, xsimd::aligned_allocator<T>>>;
    std::vector<Matrix> kernelWeights;

    std::vector<T, xsimd::aligned_allocator<T>> scratch;
};

//====================================================

/**
 * Static implementation of a 1-dimensional stateless convolution layer with no activation.
 * This implementation was designed to be used for a single frame of features, fully available at each forward call.
 * So the layer has a NO internal "state"
 *
 * @tparam T Type of the layer (float, double, int ...)
 * @tparam num_filters_in_t number of input filters
 * @tparam num_features_in_t number of input features
 * @tparam num_filters_out_t number of output filters
 * @tparam kernel_size_t size of the convolution kernel
 * @tparam stride_t convolution stride
 * @tparam valid_pad_t if true: pad is "valid", if false: pad is "same"
 */
template <typename T, int num_filters_in_t, int num_features_in_t, int num_filters_out_t, int kernel_size_t,
    int stride_t, bool valid_pad_t>
class Conv1DStatelessT
{
    static constexpr int num_features_out = Conv1DStateless<T>::computeNumFeaturesOut(num_features_in_t, kernel_size_t, stride_t, valid_pad_t);
    static constexpr int pad_left = Conv1DStateless<T>::computePadLeft(num_features_in_t, kernel_size_t, stride_t, valid_pad_t);
    static constexpr int pad_right = Conv1DStateless<T>::computePadRight(num_features_in_t, kernel_size_t, stride_t, valid_pad_t);

    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_num_filters_in = ceil_div(num_filters_in_t, v_size);
    static constexpr auto v_num_filters_out = ceil_div(num_filters_out_t, v_size);
    static constexpr auto v_in_size = v_num_filters_in * num_features_in_t;
    static constexpr auto v_out_size = v_num_filters_out * num_features_out;

    using weights_type = std::array<std::array<v_type, v_num_filters_in>, kernel_size_t>;

public:
    Conv1DStatelessT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "conv1d_stateless"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Empty function, this layer has no state */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer if pad is "valid". */
    template <bool isValid = valid_pad_t>
    RTNEURAL_REALTIME inline typename std::enable_if<isValid, void>::type
    forward(const v_type (&inMatrix)[v_in_size]) noexcept
    {
        // @TODO: can we vectorize in the other direction if num_filters == 1?
        for(int out_col_idx = 0; out_col_idx < num_features_out; ++out_col_idx)
        {
            for(int out_row_idx = 0; out_row_idx < v_num_filters_out; ++out_row_idx)
            {
                alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_temp[v_size] {};
                for(int i = 0; i < v_size; ++i)
                {
                    v_type sum {};
                    for(int in_col_idx = out_col_idx * stride_t; in_col_idx < out_col_idx * stride_t + kernel_size_t; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx - out_col_idx * stride_t;
                        for(int in_row_idx = 0; in_row_idx < v_num_filters_in; ++in_row_idx)
                            sum += kernelWeights[out_row_idx * v_size + i][kernel_col_idx][in_row_idx] * (inMatrix[in_col_idx * v_num_filters_in + in_row_idx]);
                    }
                    out_temp[i] = xsimd::reduce_add(sum);
                }
                outs[out_col_idx * v_num_filters_out + out_row_idx] += xsimd::load_aligned(out_temp);
            }
        }
    }

    /** Performs forward propagation for this layer if pad is "same" */
    template <bool isValid = valid_pad_t>
    RTNEURAL_REALTIME inline typename std::enable_if<!isValid, void>::type
    forward(const v_type (&inMatrix)[v_in_size]) noexcept
    {
        int out_col_idx = 0;

        for(; out_col_idx * stride_t < pad_left; ++out_col_idx)
        {
            for(int out_row_idx = 0; out_row_idx < v_num_filters_out; ++out_row_idx)
            {
                alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_temp[v_size] {};
                for(int i = 0; i < v_size; ++i)
                {
                    v_type sum {};
                    const int eff_kernel_size = kernel_size_t - pad_left + out_col_idx * stride_t;
                    for(int in_col_idx = 0; in_col_idx < eff_kernel_size; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx + (kernel_size_t - eff_kernel_size);
                        for(int in_row_idx = 0; in_row_idx < v_num_filters_in; ++in_row_idx)
                            sum += kernelWeights[out_row_idx * v_size + i][kernel_col_idx][in_row_idx] * (inMatrix[in_col_idx * v_num_filters_in + in_row_idx]);
                    }
                    out_temp[i] = xsimd::reduce_add(sum);
                }
                outs[out_col_idx * v_num_filters_out + out_row_idx] += xsimd::load_aligned(out_temp);
            }
        }

        for(; out_col_idx * stride_t - pad_left + kernel_size_t < num_features_in_t; ++out_col_idx)
        {
            for(int out_row_idx = 0; out_row_idx < v_num_filters_out; ++out_row_idx)
            {
                alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_temp[v_size] {};
                for(int i = 0; i < v_size; ++i)
                {
                    v_type sum {};
                    for(int in_col_idx = out_col_idx * stride_t - pad_left; in_col_idx < out_col_idx * stride_t - pad_left + kernel_size_t; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx - (out_col_idx * stride_t - pad_left);
                        for(int in_row_idx = 0; in_row_idx < v_num_filters_in; ++in_row_idx)
                            sum += kernelWeights[out_row_idx * v_size + i][kernel_col_idx][in_row_idx] * (inMatrix[in_col_idx * v_num_filters_in + in_row_idx]);
                    }
                    out_temp[i] = xsimd::reduce_add(sum);
                }
                outs[out_col_idx * v_num_filters_out + out_row_idx] += xsimd::load_aligned(out_temp);
            }
        }

        for(; out_col_idx * stride_t - pad_left + kernel_size_t <= num_features_in_t + pad_right; ++out_col_idx)
        {
            for(int out_row_idx = 0; out_row_idx < v_num_filters_out; ++out_row_idx)
            {
                alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_temp[v_size] {};
                for(int i = 0; i < v_size; ++i)
                {
                    v_type sum {};
                    const int eff_kernel_size = num_features_in_t - (out_col_idx * stride_t - pad_left);
                    for(int in_col_idx = (num_features_in_t - eff_kernel_size); in_col_idx < num_features_in_t; ++in_col_idx)
                    {
                        const auto kernel_col_idx = in_col_idx - (num_features_in_t - eff_kernel_size);
                        for(int in_row_idx = 0; in_row_idx < v_num_filters_in; ++in_row_idx)
                            sum += kernelWeights[out_row_idx * v_size + i][kernel_col_idx][in_row_idx] * (inMatrix[in_col_idx * v_num_filters_in + in_row_idx]);
                    }
                    out_temp[i] = xsimd::reduce_add(sum);
                }
                outs[out_col_idx * v_num_filters_out + out_row_idx] += xsimd::load_aligned(out_temp);
            }
        }
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[num_filters_out][num_filters_in][kernel_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& inWeights);

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size_t; }

    /** Returns the convolution dilation rate. */
    RTNEURAL_REALTIME int getStride() const noexcept { return stride_t; }

    v_type outs[v_out_size];

private:
    weights_type kernelWeights[num_filters_out_t];
};

} // RTNEURAL

#endif // RTNEURAL_CONV1D_STATELESS_XSIMD_H
