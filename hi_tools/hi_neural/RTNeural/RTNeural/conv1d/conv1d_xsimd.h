#ifndef CONV1DXSIMD_H_INCLUDED
#define CONV1DXSIMD_H_INCLUDED

#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include <numeric>
#include <vector>
#include <iostream>

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
        vCopy(input, state[state_ptr].data(), Layer<T>::in_size);

        // set state pointers to particular columns of the buffer
        setStatePointers();

        if (groups == 1)
        {
            // copy selected columns to a helper variable
            for(int k = 0; k < kernel_size; ++k)
            {
                const auto& col = state[state_ptrs[k]];
                vCopy(col.data(), state_cols[k].data(), Layer<T>::in_size);
            }

            // perform multi-channel convolution
            vCopy(bias.data(), h, Layer<T>::out_size);
            for(int i = 0; i < Layer<T>::out_size; ++i)
            {
                for(int k = 0; k < kernel_size; ++k)
                    h[i] += vMult(weights[i][k].data(), state_cols[k].data(), prod_state.data(), Layer<T>::in_size);
            }
        }
        else
        {
            // perform multi-channel convolution
            vCopy(bias.data(), h, Layer<T>::out_size);
            for(int i = 0; i < Layer<T>::out_size; ++i)
            {
                const auto ii = ((i / channels_per_group) * filters_per_group);
                for(int k = 0; k < kernel_size; ++k)
                {
                    // copy selected columns to a helper variable
                    const auto& column = state[state_ptrs[k]];
                    const auto column_begin = column.begin() + ii;
                    const auto column_end = column_begin + filters_per_group;
                    std::copy(column_begin, column_end, state_cols[k].begin());

                    h[i] += vMult(weights[i][k].data(), state_cols[k].data(), prod_state.data(), filters_per_group);
                }
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
    using vec_type = std::vector<T, xsimd::aligned_allocator<T>>;
    using vec2_type = std::vector<vec_type>;
    using vec3_type = std::vector<vec2_type>;

    const int dilation_rate;
    const int kernel_size;
    const int state_size;
    const int groups;
    const int filters_per_group;
    const int channels_per_group;

    vec3_type weights;
    vec_type bias;

    vec2_type state;
    vec2_type state_cols;

    int state_ptr = 0;
    std::vector<int> state_ptrs;

    vec_type prod_state;

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
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto state_size = (kernel_size - 1) * dilation_rate + 1;
    static constexpr auto v_in_size = ceil_div(in_sizet, v_size);
    static constexpr auto v_out_size = ceil_div(out_sizet, v_size);

    static_assert((in_sizet % groups == 0) && (out_sizet % groups == 0), "in_size and out_size must be divisible by groups!");

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;
    static constexpr auto filters_per_group = in_size / groups;
    static constexpr auto channels_per_group = out_size / groups;
    static constexpr auto v_filters_per_group = ceil_div(filters_per_group, v_size);

    Conv1DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "conv1d"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    template <int G = groups>
    RTNEURAL_REALTIME inline typename std::enable_if<(G > 1), void>::type
    forward(const v_type (&ins)[v_in_size]) noexcept
    {
        // insert input into a circular buffer
        std::copy(std::begin(ins), std::end(ins), state[state_ptr].begin());

        // set state pointers to particular columns of the buffer
        setStatePointers();

        // perform multi-channel convolution
        for(int i = 0; i < v_out_size; ++i)
        {
            alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_sum[v_size] {};
            for(int k = 0; k < v_size && (i * v_size + k) < out_size; ++k)
            {
                assert(i * v_size + k < out_size);
                const auto& subWeights = weights[i * v_size + k];
                v_type accum {};

                const auto ii = (((i * v_size + k) / channels_per_group) * filters_per_group);
                for(int j = 0; j < kernel_size; ++j)
                {
                    // copy selected columns to a helper variable
                    // @TODO: I'm not sure the reinterpret_casts are 100% safe here, but they seem to work in testing!
                    const auto& column = reinterpret_cast<std::array<T, in_size>&> (state[state_ptrs[j]]);
                    const auto column_begin = column.begin() + ii;
                    const auto column_end = column_begin + filters_per_group;
                    std::copy(column_begin, column_end, reinterpret_cast<std::array<T, filters_per_group>&> (state_cols[j]).begin());

                    accum += std::inner_product(
                        subWeights[j].begin(),
                        subWeights[j].end(),
                        state_cols[j].begin(),
                        v_type {});
                }
                out_sum[k] = xsimd::reduce_add(accum);
            }

            outs[i] = xsimd::load_aligned(out_sum) + bias[i];
        }

        state_ptr = (state_ptr == state_size - 1 ? 0 : state_ptr + 1); // iterate state pointer forwards
    }

    /** Performs forward propagation for this layer. */
    template <int DR = dilation_rate, int G = groups>
    RTNEURAL_REALTIME inline typename std::enable_if<(DR > 1 && G == 1), void>::type
    forward(const v_type (&ins)[v_in_size]) noexcept
    {
        // insert input into a circular buffer
        std::copy(std::begin(ins), std::end(ins), state[state_ptr].begin());

        // set state pointers to particular columns of the buffer
        setStatePointers();

        // copy selected columns to a helper variable
        for(int k = 0; k < kernel_size; ++k)
        {
            const auto& col = state[state_ptrs[k]];
            std::copy(col.begin(), col.end(), state_cols[k].begin());
        }

        // perform multi-channel convolution
        for(int i = 0; i < v_out_size; ++i)
        {
            alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_sum[v_size] {};
            for(int k = 0; k < v_size && (i * v_size + k) < out_size; ++k)
            {
                assert(i * v_size + k < out_size);
                const auto& subWeights = weights[i * v_size + k];
                v_type accum {};
                for(int j = 0; j < kernel_size; ++j)
                {
                    accum += std::inner_product(
                        subWeights[j].begin(),
                        subWeights[j].end(),
                        state_cols[j].begin(),
                        v_type {});
                }
                out_sum[k] = xsimd::reduce_add(accum);
            }

            outs[i] = xsimd::load_aligned(out_sum) + bias[i];
        }

        state_ptr = (state_ptr == state_size - 1 ? 0 : state_ptr + 1); // iterate state pointer forwards
    }

    /** Performs forward propagation for this layer. */
    template <int DR = dilation_rate, int KS = kernel_size, int G = groups>
    RTNEURAL_REALTIME inline typename std::enable_if<(DR == 1 && KS > 1 && G == 1), void>::type
    forward(const v_type (&ins)[v_in_size]) noexcept
    {
        // insert input into a circular buffer
        std::copy(std::begin(ins), std::end(ins), state[state_ptr].begin());

        // set state pointers to particular columns of the buffer
        setStatePointers();

        // perform multi-channel convolution
        for(int i = 0; i < v_out_size; ++i)
        {
            alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_sum[v_size] {};
            for(int k = 0; k < v_size && (i * v_size + k) < out_size; ++k)
            {
                const auto& subWeights = weights[i * v_size + k];
                v_type accum {};
                for(int j = 0; j < kernel_size; ++j)
                {
                    accum += std::inner_product(
                        subWeights[j].begin(),
                        subWeights[j].end(),
                        state[(state_ptr + state_size - j) % state_size].begin(),
                        v_type {});
                }
                out_sum[k] = xsimd::reduce_add(accum);
            }

            outs[i] = xsimd::load_aligned(out_sum) + bias[i];
        }

        state_ptr = (state_ptr == state_size - 1 ? 0 : state_ptr + 1); // iterate state pointer forwards
    }

    /** Performs forward propagation for this layer. */
    template <int DR = dilation_rate, int KS = kernel_size, int G = groups>
    RTNEURAL_REALTIME inline typename std::enable_if<DR == 1 && KS == 1 && G == 1, void>::type
    forward(const v_type (&ins)[v_in_size]) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
        {
            alignas(RTNEURAL_DEFAULT_ALIGNMENT) T out_sum[v_size] {};
            for(int k = 0; k < v_size && (i * v_size + k) < out_size; ++k)
            {
                const auto& subWeights = weights[i * v_size + k][0];

                v_type accum {};
                for(int j = 0; j < v_in_size; ++j)
                    accum += subWeights[j] * ins[j];
                out_sum[k] = xsimd::reduce_add(accum);
            }

            outs[i] = xsimd::load_aligned(out_sum) + bias[i];
        }
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

    v_type outs[v_out_size];

private:
    template <int DS = dynamic_state>
    typename std::enable_if<DS, void>::type resize_state()
    {
        state.resize(state_size, {});
    }

    template <int DS = dynamic_state>
    typename std::enable_if<!DS, void>::type resize_state() { }

    using state_col_type = std::array<v_type, v_in_size>;
    using state_type = typename std::conditional<dynamic_state, std::vector<state_col_type, xsimd::aligned_allocator<state_col_type>>, std::array<state_col_type, state_size>>::type;
    using weights_type = std::array<std::array<v_type, v_filters_per_group>, kernel_size>;

    state_type state {};
    weights_type state_cols {};

    int state_ptr = 0;
    std::array<int, kernel_size> state_ptrs {};

    weights_type weights[out_size] {};
    v_type bias[v_out_size] {};

    /** Sets pointers to state array columns. */
    inline void setStatePointers()
    {
        for(int k = 0; k < kernel_size; ++k)
            state_ptrs[k] = (state_ptr + state_size - k * dilation_rate) % state_size;
    }
};
} // namespace RTNEURAL_NAMESPACE

#endif // CONV1DXSIMD_H_INCLUDED
