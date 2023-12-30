#ifndef GRU_H_INCLUDED
#define GRU_H_INCLUDED

#include <algorithm>

#if RTNEURAL_USE_EIGEN
#include "gru_eigen.h"
#include "gru_eigen.tpp"
#elif RTNEURAL_USE_XSIMD
#include "gru_xsimd.h"
#include "gru_xsimd.tpp"
#else
#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include "../maths/maths_stl.h"
#include <vector>

namespace RTNEURAL_NAMESPACE
{

/**
 * Dynamic implementation of a gated recurrent unit (GRU) layer
 * with tanh activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class GRULayer final : public Layer<T>
{
public:
    /** Constructs a GRU layer for a given input and output size. */
    GRULayer(int in_size, int out_size);
    GRULayer(std::initializer_list<int> sizes);
    GRULayer(const GRULayer& other);
    GRULayer& operator=(const GRULayer& other);
    virtual ~GRULayer();

    /** Resets the state of the GRU. */
    RTNEURAL_REALTIME void reset() override { std::fill(ht1, ht1 + Layer<T>::out_size, (T)0); }

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "gru"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* h) noexcept override
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
        {
            zVec[i] = MathsProvider::sigmoid(vMult(zWeights.W[i], input, Layer<T>::in_size) + vMult(zWeights.U[i], ht1, Layer<T>::out_size) + zWeights.b[0][i] + zWeights.b[1][i]);
            rVec[i] = MathsProvider::sigmoid(vMult(rWeights.W[i], input, Layer<T>::in_size) + vMult(rWeights.U[i], ht1, Layer<T>::out_size) + rWeights.b[0][i] + rWeights.b[1][i]);
            cVec[i] = MathsProvider::tanh(vMult(cWeights.W[i], input, Layer<T>::in_size) + rVec[i] * (vMult(cWeights.U[i], ht1, Layer<T>::out_size) + cWeights.b[1][i]) + cWeights.b[0][i]);
            h[i] = ((T)1 - zVec[i]) * cVec[i] + zVec[i] * ht1[i];
        }

        std::copy(h, h + Layer<T>::out_size, ht1);
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(T** wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(T** uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[2][3 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(T** bVals);

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[2][3 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<std::vector<T>>& bVals);

    /** Returns the kernel weight for the given indices. */
    RTNEURAL_REALTIME T getWVal(int i, int k) const noexcept;

    /** Returns the recurrent weight for the given indices. */
    RTNEURAL_REALTIME T getUVal(int i, int k) const noexcept;

    /** Returns the bias value for the given indices. */
    RTNEURAL_REALTIME T getBVal(int i, int k) const noexcept;

protected:
    T* ht1;

    /** Struct to hold layer weights (used internally) */
    struct WeightSet
    {
        WeightSet(int in_size, int out_size);
        ~WeightSet();

        T** W; // kernel weights
        T** U; // recurrent weights
        T** b; // bias
        const int out_size;
    };

    WeightSet zWeights;
    WeightSet rWeights;
    WeightSet cWeights;

    T* zVec;
    T* rVec;
    T* cVec;

    static constexpr int kNumBiasLayers { 2 };
};

//====================================================
/**
 * Static implementation of a gated recurrent unit (GRU) layer
 * with tanh activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, int in_sizet, int out_sizet,
    SampleRateCorrectionMode sampleRateCorr = SampleRateCorrectionMode::None,
    typename MathsProvider = DefaultMathsProvider>
class GRULayerT
{
public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    GRULayerT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "gru"; }

    /** Returns false since GRU is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Prepares the GRU to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    prepare(int delaySamples);

    /** Prepares the GRU to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    prepare(T delaySamples);

    /** Resets the state of the GRU. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<(N > 1), void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        // compute zt
        recurrent_mat_mul(outs, Uz, zt);
        kernel_mat_mul(ins, Wz, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            zt[i] = MathsProvider::sigmoid(zt[i] + bz[i] + kernel_outs[i]);

        // compute rt
        recurrent_mat_mul(outs, Ur, rt);
        kernel_mat_mul(ins, Wr, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            rt[i] = MathsProvider::sigmoid(rt[i] + br[i] + kernel_outs[i]);

        // compute h_hat
        recurrent_mat_mul(outs, Uh, ct);
        kernel_mat_mul(ins, Wh, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            ht[i] = MathsProvider::tanh(rt[i] * (ct[i] + bh1[i]) + bh0[i] + kernel_outs[i]);

        computeOutput();
    }

    /** Performs forward propagation for this layer. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<N == 1, void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        // compute zt
        recurrent_mat_mul(outs, Uz, zt);
        for(int i = 0; i < out_size; ++i)
            zt[i] = MathsProvider::sigmoid(zt[i] + bz[i] + (Wz_1[i] * ins[0]));

        // compute rt
        recurrent_mat_mul(outs, Ur, rt);
        for(int i = 0; i < out_size; ++i)
            rt[i] = MathsProvider::sigmoid(rt[i] + br[i] + (Wr_1[i] * ins[0]));

        // compute h_hat
        recurrent_mat_mul(outs, Uh, ct);
        for(int i = 0; i < out_size; ++i)
            ht[i] = MathsProvider::tanh(rt[i] * (ct[i] + bh1[i]) + bh0[i] + (Wh_1[i] * ins[0]));

        computeOutput();
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[2][3 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<std::vector<T>>& bVals);

    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

private:
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::None, void>
    computeOutput() noexcept
    {
        for(int i = 0; i < out_size; ++i)
            outs[i] = ((T)1.0 - zt[i]) * ht[i] + zt[i] * outs[i];
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr != SampleRateCorrectionMode::None, void>
    computeOutput() noexcept
    {
        for(int i = 0; i < out_size; ++i)
            outs_delayed[delayWriteIdx][i] = ((T)1.0 - zt[i]) * ht[i] + zt[i] * outs[i];

        processDelay(outs_delayed, outs, delayWriteIdx);
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    processDelay(std::vector<std::array<T, out_size>>& delayVec, T (&out)[out_size], int delayWriteIndex) noexcept
    {
        for(int i = 0; i < out_size; ++i)
            out[i] = delayVec[0][i];

        for(int j = 0; j < delayWriteIndex; ++j)
        {
            for(int i = 0; i < out_size; ++i)
                delayVec[j][i] = delayVec[j + 1][i];
        }
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    processDelay(std::vector<std::array<T, out_size>>& delayVec, T (&out)[out_size], int delayWriteIndex) noexcept
    {
        for(int i = 0; i < out_size; ++i)
            out[i] = delayPlus1Mult * delayVec[0][i] + delayMult * delayVec[1][i];

        for(int j = 0; j < delayWriteIndex; ++j)
        {
            for(int i = 0; i < out_size; ++i)
                delayVec[j][i] = delayVec[j + 1][i];
        }
    }

    static inline void recurrent_mat_mul(const T (&vec)[out_size], const T (&mat)[out_size][out_size], T (&out)[out_size]) noexcept
    {
        for(int j = 0; j < out_size; ++j)
            out[j] = std::inner_product(mat[j], mat[j] + out_size, vec, (T)0);
    }

    static inline void kernel_mat_mul(const T (&vec)[in_size], const T (&mat)[out_size][in_size], T (&out)[out_size]) noexcept
    {
        for(int j = 0; j < out_size; ++j)
            out[j] = std::inner_product(mat[j], mat[j] + in_size, vec, (T)0);
    }

    // kernel weights
    T Wr alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T Wz alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T Wh alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T kernel_outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // single-input kernel weights
    T Wz_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T Wr_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T Wh_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // recurrent weights
    T Uz alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];
    T Ur alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];
    T Uh alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];

    // biases
    T bz alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T br alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T bh0 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T bh1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // intermediate vars
    T zt alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T rt alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T ct alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T ht alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // needed for delays when doing sample rate correction
    std::vector<std::array<T, out_size>> outs_delayed;
    int delayWriteIdx = 0;
    T delayMult = (T)1;
    T delayPlus1Mult = (T)0;
};

} // namespace RTNEURAL_NAMESPACE

#endif // RTNEURAL_USE_EIGEN

#endif // GRU_H_INCLUDED
