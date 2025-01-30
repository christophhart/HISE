#pragma once

#include "conv1d.h"

namespace RTNEURAL_NAMESPACE
{
/**
 * Dynamic implementation of a 1-dimensional convolutional layer
 * with strides.
 *
 * Internally, this is just a wrapper around the Conv1D layer.
 */
template <typename T>
class StridedConv1D final : public Layer<T>
{
public:
    /**
     * Constructs a strided convolution layer for the given dimensions.
     *
     * @param in_size: the input size for the layer
     * @param out_size: the output size for the layer
     * @param kernel_size: the size of the convolution kernel
     * @param dilation: the dilation rate to use for dilated convolution
     * @param stride: the stride of the convolution
     */
    StridedConv1D(int in_size, int out_size, int kernel_size, int dilation, int stride, int groups = 1)
        : Layer<T>(in_size, out_size)
        , internal(in_size, out_size, kernel_size, dilation, groups)
        , stride(stride)
    {
        skip_output.resize(out_size, T {});
    }

    StridedConv1D(std::initializer_list<int> sizes)
        : StridedConv1D<T>(*sizes.begin(), *(sizes.begin() + 1), *(sizes.begin() + 2),
            *(sizes.begin() + 3), *(sizes.begin() + 4), *(sizes.begin() + 5))
    {
    }

    StridedConv1D(const StridedConv1D& other) = default;
    StridedConv1D& operator=(const StridedConv1D& other) = default;

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() override
    {
        strides_counter = 0;
        std::fill(std::begin(skip_output), std::end(skip_output), T {});
        internal.reset();
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "strided_conv1d"; }

    /** Performs a stride step for this layer. */
    RTNEURAL_REALTIME inline void skip(const T* input)
    {
        internal.skip(input);
    }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* h) noexcept override
    {
        if(strides_counter == 0)
        {
            internal.forward(input, h);
            std::copy(h, h + Layer<T>::out_size, std::begin(skip_output));
        }
        else
        {
            internal.skip(input);
            std::copy(std::begin(skip_output), std::end(skip_output), h);
        }

        strides_counter = (strides_counter == stride - 1) ? 0 : strides_counter + 1;
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[out_size][in_size][kernel_size * dilation]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& weights)
    {
        internal.setWeights(weights);
    }

    /**
     * Sets the layer biases.
     *
     * The bias vector must have size bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const std::vector<T>& biasVals)
    {
        internal.setBias(biasVals);
    }

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return internal.getKernelSize(); }

    /** Returns the convolution dilation rate. */
    RTNEURAL_REALTIME int getDilationRate() const noexcept { return internal.getDilationRate(); }

    /** Returns the number of "groups" in the convolution. */
    int getGroups() const noexcept { return internal.getGroups(); }

private:
    Conv1D<T> internal;

    const int stride;
    int strides_counter = 0;
    std::vector<T> skip_output {};
};

//====================================================
/**
 * Static implementation of a 1-dimensional convolution layer
 * with strides.
 *
 * Internally, this is just a wrapper around the Conv1DT layer.
 *
 * @param in_sizet: the input size for the layer
 * @param out_sizet: the output size for the layer
 * @param kernel_size: the size of the convolution kernel
 * @param dilation_rate: the dilation rate to use for dilated convolution
 * @param stride: the stride of the convolution
 * @param groups: controls connections between inputs and outputs
 * @param dynamic_state: use dynamically allocated layer state
 */
template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int stride, int groups = 1, bool dynamic_state = false>
class StridedConv1DT
{
    Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state> internal;

    int strides_counter = 0;

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;
    static constexpr auto filters_per_group = in_size / groups;
    static constexpr auto channels_per_group = out_size / groups;

    StridedConv1DT()
        : outs(internal.outs)
    {
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "strided_conv1d"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset()
    {
        internal.reset();
    }

    /** Performs a stride step for this layer. */
    template <typename Inputs>
    RTNEURAL_REALTIME inline void skip(const Inputs& ins) noexcept
    {
        internal.skip(ins);
    }

    /** Performs forward propagation for this layer. */
    template <typename Inputs>
    RTNEURAL_REALTIME inline void forward(const Inputs& ins) noexcept
    {
        if(strides_counter == 0)
            internal.forward(ins);
        else
            internal.skip(ins);

        strides_counter = (strides_counter == stride - 1) ? 0 : strides_counter + 1;
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[out_size][group_count][kernel_size * dilation]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& weights)
    {
        internal.setWeights(weights);
    }

    /**
     * Sets the layer biases.
     *
     * The bias vector must have size bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const std::vector<T>& biasVals)
    {
        internal.setBias(biasVals);
    }

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size; }

    /** Returns the convolution dilation rate. */
    RTNEURAL_REALTIME int getDilationRate() const noexcept { return dilation_rate; }

    /** Returns the number of "groups" in the convolution. */
    int getGroups() const noexcept { return groups; }

    /** Reference to the internal layer weights. */
    decltype(internal.outs)& outs;
};
}
