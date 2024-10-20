#include "gru_xsimd.h"

namespace RTNEURAL_NAMESPACE
{

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::GRULayer(int in_size, int out_size)
    : Layer<T>(in_size, out_size)
    , zWeights(in_size, out_size)
    , rWeights(in_size, out_size)
    , cWeights(in_size, out_size)
{
    ht1.resize(out_size, (T)0);
    zVec.resize(out_size, (T)0);
    rVec.resize(out_size, (T)0);
    cVec.resize(out_size, (T)0);
    cTmp.resize(out_size, (T)0);

    prod_in.resize(in_size, (T)0);
    prod_out.resize(out_size, (T)0);

    ones.resize(out_size, (T)1);
}

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::GRULayer(std::initializer_list<int> sizes)
    : GRULayer<T, MathsProvider>(*sizes.begin(), *(sizes.begin() + 1))
{
}

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::GRULayer(const GRULayer<T, MathsProvider>& other)
    : GRULayer<T, MathsProvider>(other.in_size, other.out_size)
{
}

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>& GRULayer<T, MathsProvider>::operator=(const GRULayer<T, MathsProvider>& other)
{
    return *this = GRULayer<T, MathsProvider>(other);
}

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::~GRULayer() = default;

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::WeightSet::WeightSet(int in_size, int out_size)
    : out_size(out_size)
{
    W = vec2_type(out_size, vec_type(in_size, (T)0));
    U = vec2_type(out_size, vec_type(out_size, (T)0));

    b[0].resize(out_size, (T)0);
    b[1].resize(out_size, (T)0);
}

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::WeightSet::~WeightSet() = default;

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < Layer<T>::in_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            zWeights.W[k][i] = wVals[i][k];
            rWeights.W[k][i] = wVals[i][k + Layer<T>::out_size];
            cWeights.W[k][i] = wVals[i][k + Layer<T>::out_size * 2];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setWVals(T** wVals)
{
    for(int i = 0; i < Layer<T>::in_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            zWeights.W[k][i] = wVals[i][k];
            rWeights.W[k][i] = wVals[i][k + Layer<T>::out_size];
            cWeights.W[k][i] = wVals[i][k + Layer<T>::out_size * 2];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            zWeights.U[k][i] = uVals[i][k];
            rWeights.U[k][i] = uVals[i][k + Layer<T>::out_size];
            cWeights.U[k][i] = uVals[i][k + Layer<T>::out_size * 2];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setUVals(T** uVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            zWeights.U[k][i] = uVals[i][k];
            rWeights.U[k][i] = uVals[i][k + Layer<T>::out_size];
            cWeights.U[k][i] = uVals[i][k + Layer<T>::out_size * 2];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setBVals(const std::vector<std::vector<T>>& bVals)
{
    for(int i = 0; i < 2; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            zWeights.b[i][k] = bVals[i][k];
            rWeights.b[i][k] = bVals[i][k + Layer<T>::out_size];
            cWeights.b[i][k] = bVals[i][k + Layer<T>::out_size * 2];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setBVals(T** bVals)
{
    for(int i = 0; i < 2; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            zWeights.b[i][k] = bVals[i][k];
            rWeights.b[i][k] = bVals[i][k + Layer<T>::out_size];
            cWeights.b[i][k] = bVals[i][k + Layer<T>::out_size * 2];
        }
    }
}

template <typename T, typename MathsProvider>
T GRULayer<T, MathsProvider>::getWVal(int i, int k) const noexcept
{
    T** set = zWeights.W;
    if(k > 2 * Layer<T>::out_size)
    {
        k -= 2 * Layer<T>::out_size;
        set = cWeights.W;
    }
    else if(k > Layer<T>::out_size)
    {
        k -= Layer<T>::out_size;
        set = rWeights.W;
    }

    return set[i][k];
}

template <typename T, typename MathsProvider>
T GRULayer<T, MathsProvider>::getUVal(int i, int k) const noexcept
{
    T** set = zWeights.U;
    if(k > 2 * Layer<T>::out_size)
    {
        k -= 2 * Layer<T>::out_size;
        set = cWeights.U;
    }
    else if(k > Layer<T>::out_size)
    {
        k -= Layer<T>::out_size;
        set = rWeights.U;
    }

    return set[i][k];
}

template <typename T, typename MathsProvider>
T GRULayer<T, MathsProvider>::getBVal(int i, int k) const noexcept
{
    T** set = zWeights.b;
    if(k > 2 * Layer<T>::out_size)
    {
        k -= 2 * Layer<T>::out_size;
        set = cWeights.b;
    }
    else if(k > Layer<T>::out_size)
    {
        k -= Layer<T>::out_size;
        set = rWeights.b;
    }

    return set[i][k];
}

//====================================================
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::GRULayerT()
{
    for(int i = 0; i < v_out_size; ++i)
    {
        // single-input kernel weights
        Wz_1[i] = v_type((T)0);
        Wr_1[i] = v_type((T)0);
        Wh_1[i] = v_type((T)0);

        // biases
        bz[i] = v_type((T)0);
        br[i] = v_type((T)0);
        bh0[i] = v_type((T)0);
        bh1[i] = v_type((T)0);

        // intermediate vars
        zt[i] = v_type((T)0);
        rt[i] = v_type((T)0);
        ct[i] = v_type((T)0);
        ht[i] = v_type((T)0);
    }

    // kernel weights
    for(int k = 0; k < in_size; ++k)
    {
        for(int i = 0; i < v_out_size; ++i)
        {
            Wz[k][i] = v_type((T)0);
            Wr[k][i] = v_type((T)0);
            Wh[k][i] = v_type((T)0);
        }
    }

    // recurrent weights
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < v_out_size; ++k)
        {
            Uz[i][k] = v_type((T)0);
            Ur[i][k] = v_type((T)0);
            Uh[i][k] = v_type((T)0);
        }
    }

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
template <SampleRateCorrectionMode srCorr>
std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::prepare(int delaySamples)
{
    delayWriteIdx = delaySamples - 1;
    outs_delayed.resize(delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
template <SampleRateCorrectionMode srCorr>
std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::prepare(T delaySamples)
{
    const auto delayOffFactor = delaySamples - std::floor(delaySamples);
    delayMult = (T)1 - delayOffFactor;
    delayPlus1Mult = delayOffFactor;

    delayWriteIdx = (int)std::ceil(delaySamples) - (int)std::ceil(delayOffFactor);
    outs_delayed.resize(delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::reset()
{
    if(sampleRateCorr != SampleRateCorrectionMode::None)
    {
        for(auto& vec : outs_delayed)
            std::fill(vec.begin(), vec.end(), v_type {});
    }

    // reset output state
    for(int i = 0; i < v_out_size; ++i)
        outs[i] = v_type((T)0);
}

// kernel weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < in_size; ++k)
        {
            Wz[k][i / v_size] = set_value(Wz[k][i / v_size], i % v_size, wVals[k][i]);
            Wr[k][i / v_size] = set_value(Wr[k][i / v_size], i % v_size, wVals[k][i + out_size]);
            Wh[k][i / v_size] = set_value(Wh[k][i / v_size], i % v_size, wVals[k][i + 2 * out_size]);
        }
    }

    for(int j = 0; j < out_size; ++j)
    {
        Wz_1[j / v_size] = set_value(Wz_1[j / v_size], j % v_size, wVals[0][j]);
        Wr_1[j / v_size] = set_value(Wr_1[j / v_size], j % v_size, wVals[0][j + out_size]);
        Wh_1[j / v_size] = set_value(Wh_1[j / v_size], j % v_size, wVals[0][j + 2 * out_size]);
    }
}

// recurrent weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < out_size; ++k)
        {
            Uz[k][i / v_size] = set_value(Uz[k][i / v_size], i % v_size, uVals[k][i]);
            Ur[k][i / v_size] = set_value(Ur[k][i / v_size], i % v_size, uVals[k][i + out_size]);
            Uh[k][i / v_size] = set_value(Uh[k][i / v_size], i % v_size, uVals[k][i + 2 * out_size]);
        }
    }
}

// biases
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setBVals(const std::vector<std::vector<T>>& bVals)
{
    for(int k = 0; k < out_size; ++k)
    {
        bz[k / v_size] = set_value(bz[k / v_size], k % v_size, bVals[0][k] + bVals[1][k]);
        br[k / v_size] = set_value(br[k / v_size], k % v_size, bVals[0][k + out_size] + bVals[1][k + out_size]);
        bh0[k / v_size] = set_value(bh0[k / v_size], k % v_size, bVals[0][k + 2 * out_size]);
        bh1[k / v_size] = set_value(bh1[k / v_size], k % v_size, bVals[1][k + 2 * out_size]);
    }
}

} // namespace RTNEURAL_NAMESPACE
