#include "lstm_eigen.h"

namespace RTNEURAL_NAMESPACE
{

template <typename T, typename MathsProvider>
LSTMLayer<T, MathsProvider>::LSTMLayer(int in_size, int out_size)
    : Layer<T>(in_size, out_size)
{
    combinedWeights = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(4 * out_size, in_size + out_size + 1);
    extendedInVecHt1 = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(in_size + out_size + 1);
    extendedInVecHt1(in_size + out_size) = (T)1;

    fioctVecs = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(4 * out_size);
    fioVecs = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(3 * out_size);
    ctVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(out_size);

    cTanhVec = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(out_size, 1);

    ht1 = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(out_size);
    ct1 = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(out_size);
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
void LSTMLayer<T, MathsProvider>::reset()
{
    ht1.setZero();
    ct1.setZero();
    extendedInVecHt1.setZero();
    extendedInVecHt1(Layer<T>::in_size + Layer<T>::out_size) = (T)1;
}

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < Layer<T>::in_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            combinedWeights(k, i) = wVals[i][k + Layer<T>::out_size]; // Wf
            combinedWeights(k + Layer<T>::out_size, i) = wVals[i][k]; // Wi
            combinedWeights(k + Layer<T>::out_size * 2, i) = wVals[i][k + Layer<T>::out_size * 3]; // Wo
            combinedWeights(k + Layer<T>::out_size * 3, i) = wVals[i][k + Layer<T>::out_size * 2]; // Wc
        }
    }
}

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    int col;
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        col = i + Layer<T>::in_size;
        for(int k = 0; k < Layer<T>::out_size; ++k)
        {
            combinedWeights(k, col) = uVals[i][k + Layer<T>::out_size]; // Uf
            combinedWeights(k + Layer<T>::out_size, col) = uVals[i][k]; // Ui
            combinedWeights(k + Layer<T>::out_size * 2, col) = uVals[i][k + Layer<T>::out_size * 3]; // Uo
            combinedWeights(k + Layer<T>::out_size * 3, col) = uVals[i][k + Layer<T>::out_size * 2]; // Uc
        }
    }
}

template <typename T, typename MathsProvider>
void LSTMLayer<T, MathsProvider>::setBVals(const std::vector<T>& bVals)
{
    int col = Layer<T>::in_size + Layer<T>::out_size;
    for(int k = 0; k < Layer<T>::out_size; ++k)
    {
        combinedWeights(k, col) = bVals[k + Layer<T>::out_size]; // Bf
        combinedWeights(k + Layer<T>::out_size, col) = bVals[k]; // Bi
        combinedWeights(k + Layer<T>::out_size * 2, col) = bVals[k + Layer<T>::out_size * 3]; // Bo
        combinedWeights(k + Layer<T>::out_size * 3, col) = bVals[k + Layer<T>::out_size * 2]; // Bc
    }
}

//====================================================
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::LSTMLayerT()
    : outs(outs_internal)
{
    combinedWeights = weights_combined_type::Zero();
    extendedInHt1Vec = extended_in_out_type::Zero();
    fioctsVecs = four_out_type::Zero();
    fioVecs = three_out_type::Zero();

    ctVec = out_type::Zero();
    cTanhVec = out_type::Zero();

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
            x = out_type::Zero();

        for(auto& x : outs_delayed)
            x = out_type::Zero();
    }

    // reset output state
    extendedInHt1Vec.setZero();
    extendedInHt1Vec(in_sizet + out_sizet) = (T)1;
    outs = out_type::Zero();
    cVec = out_type::Zero();
    ctVec.setZero();
}

// kernel weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < in_size; ++i)
    {
        for(int k = 0; k < out_size; ++k)
        {
            combinedWeights(k, i) = wVals[i][k + out_sizet]; // Wf
            combinedWeights(k + out_sizet, i) = wVals[i][k]; // Wi
            combinedWeights(k + out_sizet * 2, i) = wVals[i][k + out_sizet * 3]; // Wo
            combinedWeights(k + out_sizet * 3, i) = wVals[i][k + out_sizet * 2]; // Wc
        }
    }
}

// recurrent weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    int col;
    for(int i = 0; i < out_size; ++i)
    {
        col = i + in_sizet;
        for(int k = 0; k < out_size; ++k)
        {
            combinedWeights(k, col) = uVals[i][k + out_sizet]; // Uf
            combinedWeights(k + out_sizet, col) = uVals[i][k]; // Ui
            combinedWeights(k + out_sizet * 2, col) = uVals[i][k + out_sizet * 3]; // Uo
            combinedWeights(k + out_sizet * 3, col) = uVals[i][k + out_sizet * 2]; // Uc
        }
    }
}

// biases
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setBVals(const std::vector<T>& bVals)
{
    int col = in_size + out_size;
    for(int k = 0; k < out_size; ++k)
    {
        combinedWeights(k, col) = bVals[k + out_sizet]; // Bf
        combinedWeights(k + out_sizet, col) = bVals[k]; // Bi
        combinedWeights(k + out_sizet * 2, col) = bVals[k + out_sizet * 3]; // Bo
        combinedWeights(k + out_sizet * 3, col) = bVals[k + out_sizet * 2]; // Bc
    }
}

} // namespace RTNEURAL_NAMESPACE
