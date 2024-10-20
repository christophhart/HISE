#ifndef DENSEXSIMD_H_INCLUDED
#define DENSEXSIMD_H_INCLUDED

#include "../Layer.h"
#include "../config.h"
#include "../../modules/xsimd/xsimd.hpp"

namespace RTNEURAL_NAMESPACE
{

/**
 * Dynamic implementation of a fully-connected (dense) layer,
 * with no activation.
 */
template <typename T>
class Dense : public Layer<T>
{
public:
    /** Constructs a dense layer for a given input and output size. */
    Dense(int in_size, int out_size)
        : Layer<T>(in_size, out_size)
    {
        prod.resize(in_size, (T)0);
        weights = vec2_type(out_size, vec_type(in_size, (T)0));

        bias.resize(out_size, (T)0);
        sums.resize(out_size, (T)0);
    }

    Dense(std::initializer_list<int> sizes)
        : Dense(*sizes.begin(), *(sizes.begin() + 1))
    {
    }

    Dense(const Dense& other)
        : Dense(other.in_size, other.out_size)
    {
    }

    Dense& operator=(const Dense& other)
    {
        return *this = Dense(other);
    }

    virtual ~Dense() = default;

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "dense"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        for(int l = 0; l < Layer<T>::out_size; ++l)
        {
            xsimd::transform(input, &input[Layer<T>::in_size], weights[l].data(), prod.data(),
                [](auto const& a, auto const& b)
                {
	                return a * b;
                });

            auto sum = xsimd::reduce(prod.begin(), prod.begin() + Layer<T>::in_size, (T)0);
            out[l] = sum + bias[l];
        }
    }

    /**
     * Sets the layer weights from a given vector.
     *
     * The dimension of the weights vector must be
     * weights[out_size][in_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<T>>& newWeights)
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            for(int k = 0; k < Layer<T>::in_size; ++k)
                weights[i][k] = newWeights[i][k];
    }

    /**
     * Sets the layer weights from a given array.
     *
     * The dimension of the weights array must be
     * weights[out_size][in_size]
     */
    RTNEURAL_REALTIME void setWeights(T** newWeights)
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            for(int k = 0; k < Layer<T>::in_size; ++k)
                weights[i][k] = newWeights[i][k];
    }

    /**
     * Sets the layer bias from a given array of size
     * bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const T* b)
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            bias[i] = b[i];
    }

    /** Returns the weights value at the given indices. */
    RTNEURAL_REALTIME T getWeight(int i, int k) const noexcept { return weights[i][k]; }

    /** Returns the bias value at the given index. */
    RTNEURAL_REALTIME T getBias(int i) const noexcept { return bias[i]; }

private:
    using vec_type = std::vector<T, xsimd::aligned_allocator<T>>;
    using vec2_type = std::vector<vec_type>;

    vec_type bias;
    vec2_type weights;
    vec_type prod;
    vec_type sums;
};

//====================================================
/**
 * Static implementation of a fully-connected (dense) layer,
 * with no activation.
 */
template <typename T, int in_sizet, int out_sizet>
class DenseT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_in_size = ceil_div(in_sizet, v_size);
    static constexpr auto v_out_size = ceil_div(out_sizet, v_size);

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    DenseT()
    {
        for(int i = 0; i < v_out_size; ++i)
            for(int k = 0; k < in_size; ++k)
                weights[k][i] = v_type((T)0.0);

        for(int i = 0; i < v_out_size; ++i)
            bias[i] = v_type((T)0.0);

        for(int i = 0; i < v_out_size; ++i)
            outs[i] = v_type((T)0.0);
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "dense"; }

    /** Returns false since dense is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Reset is a no-op, since Dense does not have state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_in_size]) noexcept
    {
        static constexpr auto v_size_inner = std::min(v_size, in_size);

        for(int i = 0; i < v_out_size; ++i)
            outs[i] = bias[i];

        T scalar_in alignas(RTNEURAL_DEFAULT_ALIGNMENT)[v_size] { (T)0 };
        for(int k = 0; k < v_in_size; ++k)
        {
            ins[k].store_aligned(scalar_in);
            for(int i = 0; i < v_out_size; ++i)
            {
                for(int j = 0; j < v_size_inner; ++j)
                    outs[i] += scalar_in[j] * weights[k * v_size + j][i];
            }
        }
    }

    /**
     * Sets the layer weights from a given vector.
     *
     * The dimension of the weights vector must be
     * weights[out_size][in_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<T>>& newWeights)
    {
        for(int i = 0; i < out_size; ++i)
        {
            for(int k = 0; k < in_size; ++k)
            {
                weights[k][i / v_size] = set_value(weights[k][i / v_size], i % v_size, newWeights[i][k]);
            }
        }
    }

    /**
     * Sets the layer weights from a given vector.
     *
     * The dimension of the weights array must be
     * weights[out_size][in_size]
     */
    RTNEURAL_REALTIME void setWeights(T** newWeights)
    {
        for(int i = 0; i < out_size; ++i)
        {
            for(int k = 0; k < in_size; ++k)
            {
                weights[k][i / v_size] = set_value(weights[k][i / v_size], i % v_size, newWeights[i][k]);
            }
        }
    }

    /**
     * Sets the layer bias from a given array of size
     * bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const T* b)
    {
        for(int i = 0; i < out_size; ++i)
            bias[i / v_size] = set_value(bias[i / v_size], i % v_size, b[i]);
    }

    v_type outs[v_out_size];

private:
    v_type bias[v_out_size];
    v_type weights[in_size][v_out_size];
};

/**
 * Static implementation of a fully-connected (dense) layer,
 * optimized for out_size=1.
 */
template <typename T, int in_sizet>
class DenseT<T, in_sizet, 1>
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_in_size = ceil_div(in_sizet, v_size);

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = 1;

    DenseT()
    {
        for(int i = 0; i < v_in_size; ++i)
            weights[i] = v_type((T)0.0);

        outs[0] = v_type((T)0.0);
    }

    std::string getName() const noexcept { return "dense"; }
    constexpr bool isActivation() const noexcept { return false; }

    RTNEURAL_REALTIME void reset() { }

    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_in_size]) noexcept
    {
        v_type y {};
        for(int k = 0; k < v_in_size; ++k)
            y += ins[k] * weights[k];

        outs[0] = v_type(xsimd::reduce_add(y) + bias);
    }

    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<T>>& newWeights)
    {
        for(int i = 0; i < out_size; ++i)
        {
            for(int k = 0; k < in_size; ++k)
            {
                auto idx = k / v_size;
                weights[idx] = set_value(weights[idx], k % v_size, newWeights[i][k]);
            }
        }
    }

    RTNEURAL_REALTIME void setWeights(T** newWeights)
    {
        for(int i = 0; i < out_size; ++i)
        {
            for(int k = 0; k < in_size; ++k)
            {
                auto idx = k / v_size;
                weights[idx] = set_value(weights[idx], k % v_size, newWeights[i][k]);
            }
        }
    }

    RTNEURAL_REALTIME void setBias(const T* b)
    {
        bias = b[0];
    }

    v_type outs[1];

private:
    T bias;
    v_type weights[v_in_size];
};

/**
 * Static implementation of a fully-connected (dense) layer,
 * optimized for in_size=1.
 */
template <typename T, int out_sizet>
class DenseT<T, 1, out_sizet>
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_out_size = ceil_div(out_sizet, v_size);

public:
    static constexpr auto in_size = 1;
    static constexpr auto out_size = out_sizet;

    DenseT()
    {
        for(int i = 0; i < v_out_size; ++i)
            weights[i] = v_type((T)0.0);

        for(int i = 0; i < v_out_size; ++i)
            bias[i] = v_type((T)0.0);

        for(int i = 0; i < v_out_size; ++i)
            outs[i] = v_type((T)0.0);
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "dense"; }

    /** Returns false since dense is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Reset is a no-op, since Dense does not have state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[1]) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
            outs[i] = bias[i];

        for(int i = 0; i < v_out_size; ++i)
            outs[i] += ins[0] * weights[i];
    }

    /**
     * Sets the layer weights from a given vector.
     *
     * The dimension of the weights vector must be
     * weights[out_size][in_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<T>>& newWeights)
    {
        for(int i = 0; i < out_size; ++i)
            weights[i / v_size] = set_value(weights[i / v_size], i % v_size, newWeights[i][0]);
    }

    /**
     * Sets the layer weights from a given vector.
     *
     * The dimension of the weights array must be
     * weights[out_size][in_size]
     */
    RTNEURAL_REALTIME void setWeights(T** newWeights)
    {
        for(int i = 0; i < out_size; ++i)
            weights[i / v_size] = set_value(weights[i / v_size], i % v_size, newWeights[i][0]);
    }

    /**
     * Sets the layer bias from a given array of size
     * bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const T* b)
    {
        for(int i = 0; i < out_size; ++i)
            bias[i / v_size] = set_value(bias[i / v_size], i % v_size, b[i]);
    }

    v_type outs[v_out_size];

private:
    v_type bias[v_out_size];
    v_type weights[v_out_size];
};

} // namespace RTNEURAL_NAMESPACE

#endif // DENSEXSIMD_H_INCLUDED
