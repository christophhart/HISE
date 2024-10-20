#ifndef DENSE_H_INCLUDED
#define DENSE_H_INCLUDED

#include <algorithm>
#include <numeric>
#include <vector>

#if RTNEURAL_USE_EIGEN
#include "dense_eigen.h"
#elif RTNEURAL_USE_XSIMD
#include "dense_xsimd.h"
#else
#include "../Layer.h"
#include "../config.h"

namespace RTNEURAL_NAMESPACE
{

#ifndef DOXYGEN
/** Single-output dense layer used internally */
template <typename T>
class Dense1
{
public:
    explicit Dense1(int in_size)
        : in_size(in_size)
    {
        weights = new T[in_size];
    }

    ~Dense1() { delete[] weights; }

    RTNEURAL_REALTIME inline T forward(const T* input) noexcept
    {
        return std::inner_product(weights, weights + in_size, input, (T)0) + bias;
    }

    RTNEURAL_REALTIME void setWeights(const T* newWeights)
    {
        for(int i = 0; i < in_size; ++i)
            weights[i] = newWeights[i];
    }

    RTNEURAL_REALTIME void setBias(T b) { bias = b; }

    RTNEURAL_REALTIME T getWeight(int i) const noexcept { return weights[i]; }

    RTNEURAL_REALTIME T getBias() const noexcept { return bias; }

private:
    const int in_size;
    T bias;

    T* weights;
};
#endif // DOXYGEN

/**
 * Dynamic implementation of a fully-connected (dense) layer,
 * with no activation.
 */
template <typename T>
class Dense final : public Layer<T>
{
public:
    /** Constructs a dense layer for a given input and output size. */
    Dense(int in_size, int out_size)
        : Layer<T>(in_size, out_size)
    {
        subLayers = new Dense1<T>*[out_size];
        for(int i = 0; i < out_size; ++i)
            subLayers[i] = new Dense1<T>(in_size);
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

    virtual ~Dense()
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            delete subLayers[i];

        delete[] subLayers;
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "dense"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            out[i] = subLayers[i]->forward(input);
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
            subLayers[i]->setWeights(newWeights[i].data());
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
            subLayers[i]->setWeights(newWeights[i]);
    }

    /**
     * Sets the layer bias from a given array of size
     * bias[out_size]
     */
    RTNEURAL_REALTIME void setBias(const T* b)
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
            subLayers[i]->setBias(b[i]);
    }

    /** Returns the weights value at the given indices. */
    RTNEURAL_REALTIME T getWeight(int i, int k) const noexcept
    {
        return subLayers[i]->getWeight(k);
    }

    /** Returns the bias value at the given index. */
    RTNEURAL_REALTIME T getBias(int i) const noexcept { return subLayers[i]->getBias(); }

private:
    Dense1<T>** subLayers;
};

//====================================================
/**
 * Static implementation of a fully-connected (dense) layer,
 * with no activation.
 */
template <typename T, int in_sizet, int out_sizet>
class DenseT
{
    static constexpr auto weights_size = in_sizet * out_sizet;

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    DenseT()
    {
        for(int i = 0; i < weights_size; ++i)
            weights[i] = (T)0.0;

        for(int i = 0; i < out_size; ++i)
            bias[i] = (T)0.0;

        for(int i = 0; i < out_size; ++i)
            outs[i] = (T)0.0;
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "dense"; }

    /** Returns false since dense is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Reset is a no-op, since Dense does not have state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T (&ins)[in_size]) noexcept
    {
        for(int i = 0; i < out_size; ++i)
            outs[i] = std::inner_product(ins, ins + in_size, &weights[i * in_size], (T)0) + bias[i];
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
                auto idx = i * in_size + k;
                weights[idx] = newWeights[i][k];
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
                auto idx = i * in_size + k;
                weights[idx] = newWeights[i][k];
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
            bias[i] = b[i];
    }

    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

private:
    T bias[out_size];
    T weights[weights_size];
};

} // namespace RTNEURAL_NAMESPACE

#endif // RTNEURAL_USE_STL

#endif // DENSE_H_INCLUDED
