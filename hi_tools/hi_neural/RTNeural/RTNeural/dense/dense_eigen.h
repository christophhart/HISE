#ifndef DENSEEIGEN_H_INCLUDED
#define DENSEEIGEN_H_INCLUDED

#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include <Eigen/Dense>

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
    static constexpr bool dense_has_bias = true;

    /** Constructs a dense layer for a given input and output size. */
    Dense(int in_size, int out_size)
        : Layer<T>(in_size, out_size)
    {
        weights = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(out_size, in_size + 1);

        inVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(in_size + 1);
        outVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(out_size);

        inVec(in_size, 0) = (T)1;
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
        for(int i = 0; i < Layer<T>::in_size; ++i)
            inVec(i, 0) = input[i];

        /**
         * out = | w b | * | input |
         *                 | 1     |
         */
        outVec.noalias() = weights * inVec;

        for(int i = 0; i < Layer<T>::out_size; ++i)
            out[i] = outVec(i, 0);
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
                weights(i, k) = newWeights[i][k];
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
                weights(i, k) = newWeights[i][k];
    }

    /**
     * Sets the layer bias from a given array of size
     * bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const T* b)
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            weights(i, Layer<T>::in_size) = b[i];
    }

    /** Returns the weights value at the given indices. */
    RTNEURAL_REALTIME T getWeight(int i, int k) const noexcept { return weights(i, k); }

    /** Returns the bias value at the given index. */
    RTNEURAL_REALTIME T getBias(int i) const noexcept { return weights(i, Layer<T>::in_size); }

private:
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> weights;

    Eigen::Matrix<T, Eigen::Dynamic, 1> inVec;
    Eigen::Matrix<T, Eigen::Dynamic, 1> outVec;
};

//====================================================
/**
 * Static implementation of a fully-connected (dense) layer,
 * with no activation.
 */
template <typename T, int in_sizet, int out_sizet, bool has_bias = true>
class DenseT
{
    using out_vec_type = Eigen::Matrix<T, out_sizet, 1>;
#if RTNEURAL_HAS_CPP17
    using in_vec_type = typename std::conditional<has_bias, Eigen::Matrix<T, in_sizet + 1, 1>, Empty>::type;
#else
    using in_vec_type = Eigen::Matrix<T, in_sizet + 1, 1>;
#endif
    using mat_type = Eigen::Matrix<T, out_sizet, in_sizet + (has_bias ? 1 : 0)>;

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;
    static constexpr bool dense_has_bias = has_bias;

    DenseT()
        : outs(outs_internal)
    {
        weights = mat_type::Zero();
        RTNEURAL_IF_CONSTEXPR(has_bias)
        {
            ins_internal = in_vec_type::Zero();
            ins_internal(in_size, 0) = (T)1;
        }
        outs = out_vec_type::Zero();
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "dense"; }

    /** Returns false since dense is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Reset is a no-op, since Dense does not have state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    template <bool b = has_bias>
    RTNEURAL_REALTIME inline typename std::enable_if<b>::type forward(const Eigen::Matrix<T, in_size, 1>& ins) noexcept
    {
        for(int i = 0; i < in_size; ++i)
            ins_internal(i, 0) = ins(i, 0);

        /**
         * out = | w b | * | input |
         *                 | 1     |
         */
        outs.noalias() = weights * ins_internal;
    }

    /** Performs forward propagation for this layer (no bias). */
    template <bool b = has_bias>
    RTNEURAL_REALTIME inline typename std::enable_if<!b>::type forward(const Eigen::Matrix<T, in_size, 1>& ins) noexcept
    {
        outs.noalias() = weights * ins;
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
            for(int k = 0; k < in_size; ++k)
                weights(i, k) = newWeights[i][k];
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
            for(int k = 0; k < in_size; ++k)
                weights(i, k) = newWeights[i][k];
    }

    /**
     * Sets the layer bias from a given array of size
     * bias[out_size]
     */
#if RTNEURAL_HAS_CPP17
    template <bool b = has_bias>
    RTNEURAL_REALTIME inline typename std::enable_if<b>::type setBias(const T* bias_vals)
#else
    RTNEURAL_REALTIME inline void setBias(const T* bias_vals)
#endif
    {
        for(int i = 0; i < out_size; ++i)
            weights(i, in_size) = bias_vals[i];
    }

    Eigen::Map<out_vec_type, RTNeuralEigenAlignment> outs;

private:
    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    in_vec_type ins_internal;

    mat_type weights;
};

} // namespace RTNEURAL_NAMESPACE

#endif // DENSEEIGEN_H_INCLUDED
