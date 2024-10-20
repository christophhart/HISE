#ifndef LSTM_H_INCLUDED
#define LSTM_H_INCLUDED

#if RTNEURAL_USE_EIGEN
#include "lstm_eigen.h"
#include "lstm_eigen.tpp"
#elif RTNEURAL_USE_XSIMD
#include "lstm_xsimd.h"
#include "lstm_xsimd.tpp"
#else
#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include "../maths/maths_stl.h"
#include <vector>

namespace RTNEURAL_NAMESPACE
{

/**
 * Dynamic implementation of a LSTM layer with tanh
 * activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class LSTMLayer final : public Layer<T>
{
public:
    /** Constructs a LSTM layer for a given input and output size. */
    LSTMLayer(int in_size, int out_size);
    LSTMLayer(std::initializer_list<int> sizes);
    LSTMLayer(const LSTMLayer& other);
    LSTMLayer& operator=(const LSTMLayer& other);
    virtual ~LSTMLayer();

    /** Resets the state of the LSTM. */
    RTNEURAL_REALTIME void reset() override;

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "lstm"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* h) noexcept override
    {
        for(int i = 0; i < Layer<T>::out_size; ++i)
        {
            fVec[i] = MathsProvider::sigmoid(vMult(fWeights.W[i], input, Layer<T>::in_size) + vMult(fWeights.U[i], ht1, Layer<T>::out_size) + fWeights.b[i]);
            iVec[i] = MathsProvider::sigmoid(vMult(iWeights.W[i], input, Layer<T>::in_size) + vMult(iWeights.U[i], ht1, Layer<T>::out_size) + iWeights.b[i]);
            oVec[i] = MathsProvider::sigmoid(vMult(oWeights.W[i], input, Layer<T>::in_size) + vMult(oWeights.U[i], ht1, Layer<T>::out_size) + oWeights.b[i]);
            ctVec[i] = MathsProvider::tanh(vMult(cWeights.W[i], input, Layer<T>::in_size) + vMult(cWeights.U[i], ht1, Layer<T>::out_size) + cWeights.b[i]);
            cVec[i] = fVec[i] * ct1[i] + iVec[i] * ctVec[i];
            h[i] = oVec[i] * MathsProvider::tanh(cVec[i]);
        }

        std::copy(cVec, cVec + Layer<T>::out_size, ct1);
        std::copy(h, h + Layer<T>::out_size, ht1);
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[4 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<T>& bVals);

protected:
    T* ht1;
    T* ct1;

    /** Struct to hold layer weights (used internally) */
    struct WeightSet
    {
        WeightSet(int in_size, int out_size);
        ~WeightSet();

        T** W; // kernel weights
        T** U; // recurrent weights
        T* b; // bias
        const int out_size;
    };

    WeightSet fWeights;
    WeightSet iWeights;
    WeightSet oWeights;
    WeightSet cWeights;

    T* fVec;
    T* iVec;
    T* oVec;
    T* ctVec;
    T* cVec;
};

//====================================================
/**
 * Static implementation of a LSTM layer with tanh
 * activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, int in_sizet, int out_sizet,
    SampleRateCorrectionMode sampleRateCorr = SampleRateCorrectionMode::None,
    typename MathsProvider = DefaultMathsProvider>
class LSTMLayerT
{
public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    LSTMLayerT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "lstm"; }

    /** Returns false since LSTM is not an activation. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Prepares the LSTM to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    prepare(int delaySamples);

    /** Prepares the LSTM to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    prepare(T delaySamples);

    /** Resets the state of the LSTM. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<(N > 1), void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        // compute ft
        recurrent_mat_mul(outs, Uf, ft);
        kernel_mat_mul(ins, Wf, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            ft[i] = MathsProvider::sigmoid(ft[i] + bf[i] + kernel_outs[i]);

        // compute it
        recurrent_mat_mul(outs, Ui, it);
        kernel_mat_mul(ins, Wi, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            it[i] = MathsProvider::sigmoid(it[i] + bi[i] + kernel_outs[i]);

        // compute ot
        recurrent_mat_mul(outs, Uo, ot);
        kernel_mat_mul(ins, Wo, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            ot[i] = MathsProvider::sigmoid(ot[i] + bo[i] + kernel_outs[i]);

        computeOutputs(ins);
    }

    /** Performs forward propagation for this layer. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<N == 1, void>::type
    forward(const T (&ins)[in_size]) noexcept
    {
        // compute ft
        recurrent_mat_mul(outs, Uf, ft);
        for(int i = 0; i < out_size; ++i)
            ft[i] = MathsProvider::sigmoid(ft[i] + bf[i] + (Wf_1[i] * ins[0]));

        // compute it
        recurrent_mat_mul(outs, Ui, it);
        for(int i = 0; i < out_size; ++i)
            it[i] = MathsProvider::sigmoid(it[i] + bi[i] + (Wi_1[i] * ins[0]));

        // compute ot
        recurrent_mat_mul(outs, Uo, ot);
        for(int i = 0; i < out_size; ++i)
            ot[i] = MathsProvider::sigmoid(ot[i] + bo[i] + (Wo_1[i] * ins[0]));

        computeOutputs(ins);
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[4 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<T>& bVals);

    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

private:
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::None, void>
    computeOutputs(const T (&ins)[in_size]) noexcept
    {
        computeOutputsInternal(ins, ct, outs);
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr != SampleRateCorrectionMode::None, void>
    computeOutputs(const T (&ins)[in_size]) noexcept
    {
        computeOutputsInternal(ins, ct_delayed[delayWriteIdx], outs_delayed[delayWriteIdx]);

        processDelay(ct_delayed, ct, delayWriteIdx);
        processDelay(outs_delayed, outs, delayWriteIdx);
    }

    template <typename VecType, int N = in_size>
    inline std::enable_if_t<(N > 1), void>
    computeOutputsInternal(const T (&ins)[in_size], VecType& ctVec, VecType& outsVec) noexcept
    {
        // compute ct
        recurrent_mat_mul(outs, Uc, ht);
        kernel_mat_mul(ins, Wc, kernel_outs);
        for(int i = 0; i < out_size; ++i)
            ctVec[i] = it[i] * MathsProvider::tanh(ht[i] + bc[i] + kernel_outs[i]) + ft[i] * ct[i];

        // compute output
        for(int i = 0; i < out_size; ++i)
            outsVec[i] = ot[i] * MathsProvider::tanh(ctVec[i]);
    }

    template <typename VecType, int N = in_size>
    inline std::enable_if_t<N == 1, void>
    computeOutputsInternal(const T (&ins)[in_size], VecType& ctVec, VecType& outsVec) noexcept
    {
        // compute ct
        recurrent_mat_mul(outs, Uc, ht);
        for(int i = 0; i < out_size; ++i)
            ctVec[i] = it[i] * MathsProvider::tanh(ht[i] + bc[i] + (Wc_1[i] * ins[0])) + ft[i] * ct[i];

        // compute output
        for(int i = 0; i < out_size; ++i)
            outsVec[i] = ot[i] * MathsProvider::tanh(ctVec[i]);
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
    T Wf alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T Wi alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T Wo alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T Wc alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][in_size];
    T kernel_outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // single-input kernel weights
    T Wf_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T Wi_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T Wo_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T Wc_1 alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // recurrent weights
    T Uf alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];
    T Ui alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];
    T Uo alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];
    T Uc alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size][out_size];

    // biases
    T bf alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T bi alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T bo alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T bc alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // intermediate vars
    T ft alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T it alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T ot alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T ht alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
    T ct alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    // needed for delays when doing sample rate correction
    std::vector<std::array<T, out_size>> ct_delayed;
    std::vector<std::array<T, out_size>> outs_delayed;
    int delayWriteIdx = 0;
    T delayMult = (T)1;
    T delayPlus1Mult = (T)0;
};

} // namespace RTNEURAL_NAMESPACE

#endif

#endif // LSTM_H_INCLUDED
