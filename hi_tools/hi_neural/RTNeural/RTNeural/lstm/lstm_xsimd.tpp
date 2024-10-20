#include "lstm_xsimd.h"

namespace RTNEURAL_NAMESPACE
{

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::LSTMLayer(int in_size, int out_size)
    : Layer<T>(in_size, out_size)
    , fWeights(in_size, out_size)
    , iWeights(in_size, out_size)
    , oWeights(in_size, out_size)
    , cWeights(in_size, out_size)
{
    ht1.resize(out_size, (T)0);
    ct1.resize(out_size, (T)0);

    fVec.resize(out_size, (T)0);
    iVec.resize(out_size, (T)0);
    oVec.resize(out_size, (T)0);
    ctVec.resize(out_size, (T)0);
    cVec.resize(out_size, (T)0);

    prod_in.resize(in_size, (T)0);
    prod_out.resize(out_size, (T)0);
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::LSTMLayer(std::initializer_list<int> sizes)
    : LSTMLayer<T, MathsProvider>(*sizes.begin(), *(sizes.begin() + 1))
{
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::LSTMLayer(const LSTMLayer& other)
    : LSTMLayer<T, MathsProvider>(other.in_size, other.out_size)
{
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>& LSTMLayer<T, MathsProvider>::operator=(const LSTMLayer<T, MathsProvider>& other)
{
    return *this = LSTMLayer<T, MathsProvider>(other);
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::~LSTMLayer() = default;

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::reset()
{
    std::fill(ht1.begin(), ht1.end(), (T)0);
    std::fill(ct1.begin(), ct1.end(), (T)0);
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::WeightSet::WeightSet(int in_size, int out_size)
    : out_size(out_size)
{
    W = vec2_type(out_size, vec_type(in_size, (T)0));
    U = vec2_type(out_size, vec_type(out_size, (T)0));
    b.resize(out_size, (T)0);
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::WeightSet::~WeightSet() = default;

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < Layer<T>::in_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            iWeights.W[k][i] = wVals[i][k];
            fWeights.W[k][i] = wVals[i][k + Layer<T>::out_size];
            cWeights.W[k][i] = wVals[i][k + Layer<T>::out_size * 2];
            oWeights.W[k][i] = wVals[i][k + Layer<T>::out_size * 3];
        }
    }
}

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            iWeights.U[k][i] = uVals[i][k];
            fWeights.U[k][i] = uVals[i][k + Layer<T>::out_size];
            cWeights.U[k][i] = uVals[i][k + Layer<T>::out_size * 2];
            oWeights.U[k][i] = uVals[i][k + Layer<T>::out_size * 3];
        }
    }
}

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::setBVals(const std::vector<T>& bVals)
{
    for(int k = 0; k < Layer<T>::out_size; ++k)
    {
        iWeights.b[k] = bVals[k];
        fWeights.b[k] = bVals[k + Layer<T>::out_size];
        cWeights.b[k] = bVals[k + Layer<T>::out_size * 2];
        oWeights.b[k] = bVals[k + Layer<T>::out_size * 3];
    }
}

//====================================================
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::LSTMLayerT()
{
    for(int i = 0; i < v_out_size; ++i)
    {
        // single-input kernel weights
        Wf_1[i] = v_type((T)0);
        Wi_1[i] = v_type((T)0);
        Wo_1[i] = v_type((T)0);
        Wc_1[i] = v_type((T)0);

        // biases
        bf[i] = v_type((T)0);
        bi[i] = v_type((T)0);
        bo[i] = v_type((T)0);
        bc[i] = v_type((T)0);

        // intermediate vars
        ft[i] = v_type((T)0);
        it[i] = v_type((T)0);
        ot[i] = v_type((T)0);
        ht[i] = v_type((T)0);
    }

    // kernel weights
    for(int k = 0; k < in_size; ++k)
    {
        for(int i = 0; i < v_out_size; ++i)
        {
            Wf[k][i] = v_type((T)0);
            Wi[k][i] = v_type((T)0);
            Wo[k][i] = v_type((T)0);
            Wc[k][i] = v_type((T)0);
        }
    }

    // recurrent weights
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < v_out_size; ++k)
        {
            Uf[i][k] = v_type((T)0);
            Ui[i][k] = v_type((T)0);
            Uo[i][k] = v_type((T)0);
            Uc[i][k] = v_type((T)0);
        }
    }

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
template <SampleRateCorrectionMode srCorr>
std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::prepare(int delaySamples)
{
    delayWriteIdx = delaySamples - 1;
    ct_delayed.resize(delayWriteIdx + 1, {});
    outs_delayed.resize(delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
template <SampleRateCorrectionMode srCorr>
std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::prepare(T delaySamples)
{
    const auto delayOffFactor = delaySamples - std::floor(delaySamples);
    delayMult = (T)1 - delayOffFactor;
    delayPlus1Mult = delayOffFactor;

    delayWriteIdx = (int)std::ceil(delaySamples) - (int)std::ceil(delayOffFactor);
    ct_delayed.resize(delayWriteIdx + 1, {});
    outs_delayed.resize(delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::reset()
{
    if constexpr(sampleRateCorr != SampleRateCorrectionMode::None)
    {
        for(auto& x : ct_delayed)
            std::fill(x.begin(), x.end(), v_type {});

        for(auto& x : outs_delayed)
            std::fill(x.begin(), x.end(), v_type {});
    }

    // reset output state
    for(int i = 0; i < v_out_size; ++i)
    {
        ct[i] = v_type((T)0);
        outs[i] = v_type((T)0);
    }
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < in_size; ++k)
        {
            Wi[k][i / v_size] = set_value(Wi[k][i / v_size], i % v_size, wVals[k][i]);
            Wf[k][i / v_size] = set_value(Wf[k][i / v_size], i % v_size, wVals[k][i + out_size]);
            Wc[k][i / v_size] = set_value(Wc[k][i / v_size], i % v_size, wVals[k][i + 2 * out_size]);
            Wo[k][i / v_size] = set_value(Wo[k][i / v_size], i % v_size, wVals[k][i + 3 * out_size]);
        }
    }

    for(int j = 0; j < out_size; ++j)
    {
        Wi_1[j / v_size] = set_value(Wi_1[j / v_size], j % v_size, wVals[0][j]);
        Wf_1[j / v_size] = set_value(Wf_1[j / v_size], j % v_size, wVals[0][j + out_size]);
        Wc_1[j / v_size] = set_value(Wc_1[j / v_size], j % v_size, wVals[0][j + 2 * out_size]);
        Wo_1[j / v_size] = set_value(Wo_1[j / v_size], j % v_size, wVals[0][j + 3 * out_size]);
    }
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < out_size; ++k)
        {
            Ui[k][i / v_size] = set_value(Ui[k][i / v_size], i % v_size, uVals[k][i]);
            Uf[k][i / v_size] = set_value(Uf[k][i / v_size], i % v_size, uVals[k][i + out_size]);
            Uc[k][i / v_size] = set_value(Uc[k][i / v_size], i % v_size, uVals[k][i + 2 * out_size]);
            Uo[k][i / v_size] = set_value(Uo[k][i / v_size], i % v_size, uVals[k][i + 3 * out_size]);
        }
    }
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setBVals(const std::vector<T>& bVals)
{
    for(int k = 0; k < out_size; ++k)
    {
        bi[k / v_size] = set_value(bi[k / v_size], k % v_size, bVals[k]);
        bf[k / v_size] = set_value(bf[k / v_size], k % v_size, bVals[k + out_size]);
        bc[k / v_size] = set_value(bc[k / v_size], k % v_size, bVals[k + 2 * out_size]);
        bo[k / v_size] = set_value(bo[k / v_size], k % v_size, bVals[k + 3 * out_size]);
    }
}

} // namespace RTNEURAL_NAMESPACE
