#ifndef LSTM_XSIMD_H_INCLUDED
#define LSTM_XSIMD_H_INCLUDED

#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include "../maths/maths_xsimd.h"
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
class LSTMLayer : public Layer<T>
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
            fVec[i] = vMult(fWeights.W[i].data(), input, prod_in.data(), Layer<T>::in_size) + vMult(fWeights.U[i].data(), ht1.data(), prod_out.data(), Layer<T>::out_size);
            iVec[i] = vMult(iWeights.W[i].data(), input, prod_in.data(), Layer<T>::in_size) + vMult(iWeights.U[i].data(), ht1.data(), prod_out.data(), Layer<T>::out_size);
            oVec[i] = vMult(oWeights.W[i].data(), input, prod_in.data(), Layer<T>::in_size) + vMult(oWeights.U[i].data(), ht1.data(), prod_out.data(), Layer<T>::out_size);
            ctVec[i] = vMult(cWeights.W[i].data(), input, prod_in.data(), Layer<T>::in_size) + vMult(cWeights.U[i].data(), ht1.data(), prod_out.data(), Layer<T>::out_size);
        }

        vAdd(fVec.data(), fWeights.b.data(), fVec.data(), Layer<T>::out_size);
        sigmoid<T, MathsProvider>(fVec.data(), fVec.data(), Layer<T>::out_size);

        vAdd(iVec.data(), iWeights.b.data(), iVec.data(), Layer<T>::out_size);
        sigmoid<T, MathsProvider>(iVec.data(), iVec.data(), Layer<T>::out_size);

        vAdd(oVec.data(), oWeights.b.data(), oVec.data(), Layer<T>::out_size);
        sigmoid<T, MathsProvider>(oVec.data(), oVec.data(), Layer<T>::out_size);

        vAdd(ctVec.data(), cWeights.b.data(), ctVec.data(), Layer<T>::out_size);
        tanh<T, MathsProvider>(ctVec.data(), ctVec.data(), Layer<T>::out_size);

        vProd(fVec.data(), ct1.data(), cVec.data(), Layer<T>::out_size);
        vProd(iVec.data(), ctVec.data(), prod_out.data(), Layer<T>::out_size);
        vAdd(cVec.data(), prod_out.data(), cVec.data(), Layer<T>::out_size);

        tanh<T, MathsProvider>(cVec.data(), h, Layer<T>::out_size);
        vProd(h, oVec.data(), h, Layer<T>::out_size);

        vCopy(cVec.data(), ct1.data(), Layer<T>::out_size);
        vCopy(h, ht1.data(), Layer<T>::out_size);
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
    using vec_type = std::vector<T, xsimd::aligned_allocator<T>>;
    using vec2_type = std::vector<vec_type>;

    vec_type ht1;
    vec_type ct1;

    struct WeightSet
    {
        WeightSet(int in_size, int out_size);
        ~WeightSet();

        vec2_type W; // kernel weights
        vec2_type U; // recurrent weights
        vec_type b; // bias
        const int out_size;
    };

    WeightSet fWeights;
    WeightSet iWeights;
    WeightSet oWeights;
    WeightSet cWeights;

    vec_type fVec;
    vec_type iVec;
    vec_type oVec;
    vec_type ctVec;
    vec_type cVec;

    vec_type prod_in;
    vec_type prod_out;
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
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_in_size = ceil_div(in_sizet, v_size);
    static constexpr auto v_out_size = ceil_div(out_sizet, v_size);

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
    forward(const v_type (&ins)[v_in_size]) noexcept
    {
        // compute ft
        recurrent_mat_mul(outs, Uf, ft);
        kernel_mat_mul(ins, Wf, kernel_outs);
        for(int i = 0; i < v_out_size; ++i)
            ft[i] = MathsProvider::sigmoid(ft[i] + bf[i] + kernel_outs[i]);

        // compute it
        recurrent_mat_mul(outs, Ui, it);
        kernel_mat_mul(ins, Wi, kernel_outs);
        for(int i = 0; i < v_out_size; ++i)
            it[i] = MathsProvider::sigmoid(it[i] + bi[i] + kernel_outs[i]);

        // compute ot
        recurrent_mat_mul(outs, Uo, ot);
        kernel_mat_mul(ins, Wo, kernel_outs);
        for(int i = 0; i < v_out_size; ++i)
            ot[i] = MathsProvider::sigmoid(ot[i] + bo[i] + kernel_outs[i]);

        computeOutputs(ins);
    }

    /** Performs forward propagation for this layer. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<N == 1, void>::type
    forward(const v_type (&ins)[v_in_size]) noexcept
    {
        // compute ft
        recurrent_mat_mul(outs, Uf, ft);
        for(int i = 0; i < v_out_size; ++i)
            ft[i] = MathsProvider::sigmoid(xsimd::fma(Wf_1[i], ins[0], ft[i] + bf[i]));

        // compute it
        recurrent_mat_mul(outs, Ui, it);
        for(int i = 0; i < v_out_size; ++i)
            it[i] = MathsProvider::sigmoid(xsimd::fma(Wi_1[i], ins[0], it[i] + bi[i]));

        // compute ot
        recurrent_mat_mul(outs, Uo, ot);
        for(int i = 0; i < v_out_size; ++i)
            ot[i] = MathsProvider::sigmoid(xsimd::fma(Wo_1[i], ins[0], ot[i] + bo[i]));

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

    v_type outs[v_out_size];

private:
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::None, void>
    computeOutputs(const v_type (&ins)[v_in_size]) noexcept
    {
        computeOutputsInternal(ins, ct, outs);
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr != SampleRateCorrectionMode::None, void>
    computeOutputs(const v_type (&ins)[v_in_size]) noexcept
    {
        computeOutputsInternal(ins, ct_delayed[delayWriteIdx], outs_delayed[delayWriteIdx]);

        processDelay(ct_delayed, ct, delayWriteIdx);
        processDelay(outs_delayed, outs, delayWriteIdx);
    }

    template <typename VecType, int N = in_size>
    inline std::enable_if_t<(N > 1), void>
    computeOutputsInternal(const v_type (&ins)[v_in_size], VecType& ctVec, VecType& outsVec) noexcept
    {
        // compute ct
        recurrent_mat_mul(outs, Uc, ht);
        kernel_mat_mul(ins, Wc, kernel_outs);
        for(int i = 0; i < v_out_size; ++i)
            ctVec[i] = xsimd::fma(it[i], MathsProvider::tanh(ht[i] + bc[i] + kernel_outs[i]), ft[i] * ct[i]);

        // compute output
        for(int i = 0; i < v_out_size; ++i)
            outsVec[i] = ot[i] * MathsProvider::tanh(ctVec[i]);
    }

    template <typename VecType, int N = in_size>
    inline std::enable_if_t<N == 1, void>
    computeOutputsInternal(const v_type (&ins)[v_in_size], VecType& ctVec, VecType& outsVec) noexcept
    {
        // compute ct
        recurrent_mat_mul(outs, Uc, ht);
        for(int i = 0; i < v_out_size; ++i)
            ctVec[i] = xsimd::fma(it[i], MathsProvider::tanh(xsimd::fma(Wc_1[i], ins[0], ht[i] + bc[i])), ft[i] * ct[i]);

        // compute output
        for(int i = 0; i < v_out_size; ++i)
            outsVec[i] = ot[i] * MathsProvider::tanh(ctVec[i]);
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    processDelay(std::vector<std::array<v_type, v_out_size>>& delayVec, v_type (&out)[v_out_size], int delayWriteIndex) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
            out[i] = delayVec[0][i];

        for(int j = 0; j < delayWriteIndex; ++j)
        {
            for(int i = 0; i < v_out_size; ++i)
                delayVec[j][i] = delayVec[j + 1][i];
        }
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    processDelay(std::vector<std::array<v_type, v_out_size>>& delayVec, v_type (&out)[v_out_size], int delayWriteIndex) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
            out[i] = delayPlus1Mult * delayVec[0][i] + delayMult * delayVec[1][i];

        for(int j = 0; j < delayWriteIndex; ++j)
        {
            for(int i = 0; i < v_out_size; ++i)
                delayVec[j][i] = delayVec[j + 1][i];
        }
    }

    static inline void recurrent_mat_mul(const v_type (&vec)[v_out_size], const v_type (&mat)[out_size][v_out_size], v_type (&out)[v_out_size]) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
            out[i] = v_type(0);

        T scalar_in alignas(RTNEURAL_DEFAULT_ALIGNMENT)[v_size] { (T)0 };
        for(int k = 0; k < v_out_size; ++k)
        {
            vec[k].store_aligned(scalar_in);
            for(int i = 0; i < v_out_size; ++i)
            {
                for(int j = 0; j < v_size; ++j)
                    out[i] += scalar_in[j] * mat[k * v_size + j][i];
            }
        }
    }

    static inline void kernel_mat_mul(const v_type (&vec)[v_in_size], const v_type (&mat)[in_size][v_out_size], v_type (&out)[v_out_size]) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
            out[i] = v_type(0);

        T scalar_in alignas(RTNEURAL_DEFAULT_ALIGNMENT)[v_size] { (T)0 };
        for(int k = 0; k < v_in_size; ++k)
        {
            vec[k].store_aligned(scalar_in);
            for(int i = 0; i < v_out_size; ++i)
            {
                for(int j = 0; j < v_size; ++j)
                    out[i] += scalar_in[j] * mat[k * v_size + j][i];
            }
        }
    }

    static inline v_type sigmoid(v_type x) noexcept
    {
        return (T)1.0 / ((T)1.0 + xsimd::exp(-x));
    }

    // kernel weights
    v_type Wf[in_size][v_out_size];
    v_type Wi[in_size][v_out_size];
    v_type Wo[in_size][v_out_size];
    v_type Wc[in_size][v_out_size];
    v_type kernel_outs[v_out_size];

    // single-input kernel weights
    v_type Wf_1[v_out_size];
    v_type Wi_1[v_out_size];
    v_type Wo_1[v_out_size];
    v_type Wc_1[v_out_size];

    // recurrent weights
    v_type Uf[out_size][v_out_size];
    v_type Ui[out_size][v_out_size];
    v_type Uo[out_size][v_out_size];
    v_type Uc[out_size][v_out_size];

    // biases
    v_type bf[v_out_size];
    v_type bi[v_out_size];
    v_type bo[v_out_size];
    v_type bc[v_out_size];

    // intermediate vars
    v_type ft[v_out_size];
    v_type it[v_out_size];
    v_type ot[v_out_size];
    v_type ht[v_out_size];
    v_type ct[v_out_size];

    // needed for delays when doing sample rate correction
    std::vector<std::array<v_type, v_out_size>> ct_delayed;
    std::vector<std::array<v_type, v_out_size>> outs_delayed;
    int delayWriteIdx = 0;
    v_type delayMult = (T)1;
    v_type delayPlus1Mult = (T)0;
};

} // namespace RTNEURAL_NAMESPACE

#endif // LSTM_XSIMD_H_INCLUDED
