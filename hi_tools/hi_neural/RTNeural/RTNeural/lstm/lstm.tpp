#include "lstm.h"

namespace RTNEURAL_NAMESPACE
{

#if !RTNEURAL_USE_EIGEN && !RTNEURAL_USE_XSIMD

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::LSTMLayer(int in_size, int out_size)
    : Layer<T>(in_size, out_size)
    , fWeights(in_size, out_size)
    , iWeights(in_size, out_size)
    , oWeights(in_size, out_size)
    , cWeights(in_size, out_size)
{
    ht1 = new T[out_size];
    ct1 = new T[out_size];

    fVec = new T[out_size];
    iVec = new T[out_size];
    oVec = new T[out_size];
    ctVec = new T[out_size];
    cVec = new T[out_size];
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
    if(&other != this)
        *this = LSTMLayer<T, MathsProvider>(other);
    return *this;
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::~LSTMLayer()
{
    delete[] ht1;
    delete[] ct1;

    delete[] fVec;
    delete[] iVec;
    delete[] oVec;
    delete[] ctVec;
    delete[] cVec;
}

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::reset()
{
    std::fill(ht1, ht1 + Layer<T>::out_size, (T)0);
    std::fill(ct1, ct1 + Layer<T>::out_size, (T)0);
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::WeightSet::WeightSet(int in_size, int out_size)
    : out_size(out_size)
{
    W = new T*[out_size];
    U = new T*[out_size];
    b = new T[out_size];

    for(int i = 0; i < out_size; ++i)
    {
        W[i] = new T[in_size];
        U[i] = new T[out_size];
    }
}

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::WeightSet::~WeightSet()
{
    delete[] b;

    for(int i = 0; i < out_size; ++i)
    {
        delete[] W[i];
        delete[] U[i];
    }

    delete[] W;
    delete[] U;
}

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
    for(int i = 0; i < out_size; ++i)
    {
        // single-input kernel weights
        Wf_1[i] = (T)0;
        Wi_1[i] = (T)0;
        Wo_1[i] = (T)0;
        Wc_1[i] = (T)0;

        // biases
        bf[i] = (T)0;
        bi[i] = (T)0;
        bo[i] = (T)0;
        bc[i] = (T)0;

        // intermediate vars
        ft[i] = (T)0;
        it[i] = (T)0;
        ot[i] = (T)0;
        ht[i] = (T)0;
    }

    for(int i = 0; i < out_size; ++i)
    {
        // recurrent weights
        for(int k = 0; k < out_size; ++k)
        {
            Uf[i][k] = (T)0;
            Ui[i][k] = (T)0;
            Uo[i][k] = (T)0;
            Uc[i][k] = (T)0;
        }

        // kernel weights
        for(int k = 0; k < in_size; ++k)
        {
            Wf[i][k] = (T)0;
            Wi[i][k] = (T)0;
            Wo[i][k] = (T)0;
            Wc[i][k] = (T)0;
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
    if(sampleRateCorr != SampleRateCorrectionMode::None)
    {
        for(auto& x : ct_delayed)
            std::fill(x.begin(), x.end(), T {});

        for(auto& x : outs_delayed)
            std::fill(x.begin(), x.end(), T {});
    }

    // reset output state
    for(int i = 0; i < out_size; ++i)
    {
        ct[i] = (T)0;
        outs[i] = (T)0;
    }
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < in_size; ++i)
    {
        for(int j = 0; j < out_size; ++j)
        {
            Wi[j][i] = wVals[i][j];
            Wf[j][i] = wVals[i][j + out_size];
            Wc[j][i] = wVals[i][j + 2 * out_size];
            Wo[j][i] = wVals[i][j + 3 * out_size];
        }
    }

    for(int j = 0; j < out_size; ++j)
    {
        Wi_1[j] = wVals[0][j];
        Wf_1[j] = wVals[0][j + out_size];
        Wc_1[j] = wVals[0][j + 2 * out_size];
        Wo_1[j] = wVals[0][j + 3 * out_size];
    }
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int j = 0; j < out_size; ++j)
        {
            Ui[j][i] = uVals[i][j];
            Uf[j][i] = uVals[i][j + out_size];
            Uc[j][i] = uVals[i][j + 2 * out_size];
            Uo[j][i] = uVals[i][j + 3 * out_size];
        }
    }
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setBVals(const std::vector<T>& bVals)
{
    for(int k = 0; k < out_size; ++k)
    {
        bi[k] = bVals[k];
        bf[k] = bVals[k + out_size];
        bc[k] = bVals[k + 2 * out_size];
        bo[k] = bVals[k + 3 * out_size];
    }
}

#endif // !RTNEURAL_USE_EIGEN && !RTNEURAL_USE_XSIMD

} // namespace RTNEURAL_NAMESPACE
