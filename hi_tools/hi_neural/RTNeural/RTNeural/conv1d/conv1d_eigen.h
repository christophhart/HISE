#ifndef CONV1DEIGEN_H_INCLUDED
#define CONV1DEIGEN_H_INCLUDED

#include "../Layer.h"
#include "../config.h"
#include <Eigen/Dense>

namespace RTNEURAL_NAMESPACE
{

/**
 * Dynamic implementation of a 1-dimensional convolution layer
 * with no activation.
 *
 * This implementation was designed to be used for "temporal
 * convolution", so the layer has a "state" made up of past inputs
 * to the layer. To ensure that the state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T>
class Conv1D : public Layer<T>
{
public:
    /**
     * Constructs a convolution layer for the given dimensions.
     *
     * @param in_size: the input size for the layer
     * @param out_size: the output size for the layer
     * @param kernel_size: the size of the convolution kernel
     * @param dilation: the dilation rate to use for dilated convolution
     */
    Conv1D(int in_size, int out_size, int kernel_size, int dilation, int groups = 1);
    Conv1D(std::initializer_list<int> sizes);
    Conv1D(const Conv1D& other);
    Conv1D& operator=(const Conv1D& other);
    virtual ~Conv1D();

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() override;

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "conv1d"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* h) noexcept override
    {
        // insert input into a circular buffer
        state.col(state_ptr) = Eigen::Map<const Eigen::Vector<T, Eigen::Dynamic>,
            RTNeuralEigenAlignment>(input, Layer<T>::in_size);

        // set state pointers to the particular columns of the buffer
        setStatePointers();

        if (groups == 1)
        {
            // copy selected columns to a helper variable
            for(int k = 0; k < kernel_size; ++k)
                state_cols.col(k) = state.col(state_ptrs(k));

            // perform a multichannel convolution
            for(int i = 0; i < Layer<T>::out_size; ++i)
                h[i] = state_cols.cwiseProduct(kernelWeights[i]).sum() + bias(i);
        }
        else
        {
            // perform a multichannel convolution
            for(int i = 0; i < Layer<T>::out_size; ++i)
            {
                // copy selected columns to a helper variable
                const auto ii = ((i / channels_per_group) * filters_per_group);
                for(int k = 0; k < kernel_size; ++k)
                    state_cols.col(k) = state.col(state_ptrs(k))(Eigen::seqN(ii, filters_per_group));

                h[i] = state_cols.cwiseProduct(kernelWeights[i]).sum() + bias(i);
            }
        }

        state_ptr = (state_ptr == state_size - 1 ? 0 : state_ptr + 1); // iterate state pointer forwards
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[out_size][in_size][kernel_size * dilation]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& weights);

    /**
     * Sets the layer biases.
     *
     * The bias vector must have size bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const std::vector<T>& biasVals);

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size; }

    /** Returns the convolution dilation rate. */
    RTNEURAL_REALTIME int getDilationRate() const noexcept { return dilation_rate; }

    /** Returns the number of "groups" in the convolution. */
    int getGroups() const noexcept { return groups; }

private:
    const int dilation_rate;
    const int kernel_size;
    const int state_size;
    const int groups;
    const int filters_per_group;
    const int channels_per_group;

    std::vector<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> kernelWeights;
    Eigen::Vector<T, Eigen::Dynamic> bias;

    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> state;
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> state_cols;
    Eigen::VectorXi state_ptrs;
    int state_ptr = 0;

    /** Sets pointers to state array columns. */
    inline void setStatePointers()
    {
        for(int k = 0; k < kernel_size; ++k)
            state_ptrs[k] = (state_ptr + state_size - k * dilation_rate) % state_size;
    }
};

//====================================================
/**
 * Static implementation of a 1-dimensional convolution layer
 * with no activation.
 *
 * This implementation was designed to be used for "temporal
 * convolution", so the layer has a "state" made up of past inputs
 * to the layer. To ensure that the state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 *
 * @param in_sizet: the input size for the layer
 * @param out_sizet: the output size for the layer
 * @param kernel_size: the size of the convolution kernel
 * @param dilation_rate: the dilation rate to use for dilated convolution
 * @param dynamic_state: use dynamically allocated layer state
 */
template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups = 1, bool dynamic_state = false>
class Conv1DT
{
    using vec_type = Eigen::Vector<T, out_sizet>;

    static_assert((in_sizet % groups == 0) && (out_sizet % groups == 0), "in_sizet and out_sizet must be divisible by groups!");

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;
    static constexpr auto dilation = dilation_rate;
    static constexpr auto kernel_length = kernel_size;
    static constexpr auto filters_per_group = in_size / groups;
    static constexpr auto channels_per_group = out_size / groups;

    static constexpr auto state_size = (kernel_size - 1) * dilation_rate + 1;
    using state_type = Eigen::Matrix<T, in_sizet, dynamic_state ? Eigen::Dynamic : state_size>;
    using weights_type = Eigen::Matrix<T, filters_per_group, kernel_size>;
    using state_ptrs_type = Eigen::Vector<int, kernel_size>;

    Conv1DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "conv1d"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    template<int _groups = groups, std::enable_if_t<_groups == 1, bool> = true>
    RTNEURAL_REALTIME inline void forward(const Eigen::Matrix<T, in_size, 1>& ins) noexcept
    {
        // insert input into a circular buffer
        state.col(state_ptr) = ins;

        // set state pointers to particular columns of the buffer
        setStatePointers();

        // copy selected columns to a helper variable
        for(int k = 0; k < kernel_length; ++k)
            state_cols.col(k) = state.col(state_ptrs(k));

        // perform a multichannel convolution
        for(int i = 0; i < out_size; ++i)
            outs(i) = state_cols.cwiseProduct(weights[i]).sum() + bias(i);

        state_ptr = (state_ptr == state_size - 1 ? 0 : state_ptr + 1); // iterate state pointer forwards
    }

    /** Performs forward propagation for this layer (groups > 1). */
    template<int _groups = groups, std::enable_if_t<_groups != 1, bool> = true>
    RTNEURAL_REALTIME inline void forward(const Eigen::Matrix<T, in_size, 1>& ins) noexcept
    {
        // insert input into a circular buffer
        state.col(state_ptr) = ins;

        // set state pointers to particular columns of the buffer
        setStatePointers();

        // perform a multichannel convolution
        for(int i = 0; i < out_size; ++i)
        {
            // copy selected columns to a helper variable
            const auto ii = ((i / channels_per_group) * filters_per_group);
            for(int k = 0; k < kernel_length; ++k)
                state_cols.col(k) = state.col(state_ptrs(k))(Eigen::seqN(ii, filters_per_group));

            outs(i) = state_cols.cwiseProduct(weights[i]).sum() + bias(i);
        }

        state_ptr = (state_ptr == state_size - 1 ? 0 : state_ptr + 1); // iterate state pointer forwards
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[out_size][in_size][kernel_size * dilation]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& weights);

    /**
     * Sets the layer biases.
     *
     * The bias vector must have size bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const std::vector<T>& biasVals);

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size; }

    /** Returns the convolution dilation rate. */
    RTNEURAL_REALTIME int getDilationRate() const noexcept { return dilation_rate; }

    /** Returns the number of "groups" in the convolution. */
    int getGroups() const noexcept { return groups; }

    Eigen::Map<vec_type, RTNeuralEigenAlignment> outs;

private:
    void resize_state()
    {
        state.resize(in_sizet, state_size);
    }

    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    state_type state;
    weights_type state_cols;

    int state_ptr = 0;
    state_ptrs_type state_ptrs;

    weights_type weights[out_size];
    vec_type bias;

    /** Sets pointers to state array columns. */
    inline void setStatePointers()
    {
        for(int k = 0; k < kernel_size; ++k)
            state_ptrs[k] = (state_ptr + state_size - k * dilation_rate) % state_size;
    }
};

} // RTNEURAL_NAMESPACE

#endif // CONV1DEIGEN_H_INCLUDED
