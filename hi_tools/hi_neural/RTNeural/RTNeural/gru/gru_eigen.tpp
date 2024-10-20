#if RTNEURAL_USE_EIGEN

#include "gru_eigen.h"

namespace RTNEURAL_NAMESPACE
{

template <typename T, typename MathsProvider>
GRULayer<T, MathsProvider>::GRULayer(int in_size, int out_size)
    : Layer<T>(in_size, out_size)
{
    wCombinedWeights = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(3 * out_size, in_size + 1);
    uCombinedWeights = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(3 * out_size, out_size + 1);
    extendedInVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(in_size + 1);
    extendedHt1 = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(out_size + 1);
    extendedInVec(Layer<T>::in_size) = (T)1;
    extendedHt1(Layer<T>::out_size) = (T)1;

    alphaVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(3 * out_size);
    betaVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(3 * out_size);
    gammaVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(2 * out_size);
    cVec = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(out_size);
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
void GRULayer<T, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < Layer<T>::in_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size * 3; ++k)
        {
            wCombinedWeights(k, i) = wVals[i][k];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setWVals(T** wVals)
{
    for(int i = 0; i < Layer<T>::in_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size * 3; ++k)
        {
            wCombinedWeights(k, i) = wVals[i][k];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size * 3; ++k)
        {
            uCombinedWeights(k, i) = uVals[i][k];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setUVals(T** uVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        for(int k = 0; k < Layer<T>::out_size * 3; ++k)
        {
            uCombinedWeights(k, i) = uVals[i][k];
        }
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setBVals(const std::vector<std::vector<T>>& bVals)
{
    for(int k = 0; k < Layer<T>::out_size * 3; ++k)
    {
        wCombinedWeights(k, Layer<T>::in_size) = bVals[0][k];
        uCombinedWeights(k, Layer<T>::out_size) = bVals[1][k];
    }
}

template <typename T, typename MathsProvider>
void GRULayer<T, MathsProvider>::setBVals(T** bVals)
{
    for(int k = 0; k < Layer<T>::out_size * 3; ++k)
    {
        wCombinedWeights(k, Layer<T>::in_size) = bVals[0][k];
        uCombinedWeights(k, Layer<T>::out_size) = bVals[1][k];
    }
}

template <typename T, typename MathsProvider>
T GRULayer<T, MathsProvider>::getWVal(int i, int k) const noexcept
{
    return wCombinedWeights[k][i];
}

template <typename T, typename MathsProvider>
T GRULayer<T, MathsProvider>::getUVal(int i, int k) const noexcept
{
    return uCombinedWeights[k][i];
}

template <typename T, typename MathsProvider>
T GRULayer<T, MathsProvider>::getBVal(int i, int k) const noexcept
{
    T val;
    if(i == 0)
    {
        val = wCombinedWeights[k][Layer<T>::in_size];
    }
    else
    {
        val = uCombinedWeights[k][Layer<T>::out_size];
    }
    return val;
}

//====================================================
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::GRULayerT()
    : outs(outs_internal)
{
    wCombinedWeights = w_k_type::Zero();
    uCombinedWeights = u_k_type::Zero();
    alphaVec = three_out_type::Zero();
    betaVec = three_out_type::Zero();
    gammaVec = two_out_type::Zero();
    cVec = out_type::Zero();
    extendedInVec = extended_in_type::Zero();
    extendedHt1 = extended_out_type::Zero();

    extendedInVec(in_sizet) = (T)1;
    extendedHt1(out_sizet) = (T)1;

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
            vec = out_type::Zero();
    }

    // reset output state
    outs = out_type::Zero();

    // reset extended internal state
    extendedHt1.setZero();
    extendedHt1(out_sizet) = (T)1;
}

// kernel weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < in_size; ++i)
    {
        for(int k = 0; k < out_size * 3; ++k)
        {
            wCombinedWeights(k, i) = wVals[i][k];
        }
    }
}

// recurrent weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < out_size * 3; ++k)
        {
            uCombinedWeights(k, i) = uVals[i][k];
        }
    }
}

// biases
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void GRULayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setBVals(const std::vector<std::vector<T>>& bVals)
{
    for(int k = 0; k < out_size * 3; ++k)
    {
        wCombinedWeights(k, in_sizet) = bVals[0][k];
        uCombinedWeights(k, out_sizet) = bVals[1][k];
    }
}

} // namespace RTNEURAL_NAMESPACE

#endif // RTNEURAL_USE_EIGEN
