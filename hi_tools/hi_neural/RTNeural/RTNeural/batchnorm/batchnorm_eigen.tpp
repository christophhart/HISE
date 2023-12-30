#include "batchnorm_eigen.h"

namespace RTNEURAL_NAMESPACE
{
template <typename T>
BatchNorm1DLayer<T>::BatchNorm1DLayer(int size)
    : Layer<T>(size, size)
{
    gamma = Eigen::Vector<T, Eigen::Dynamic>::Ones(size);
    beta = Eigen::Vector<T, Eigen::Dynamic>::Zero(size);
    running_mean = Eigen::Vector<T, Eigen::Dynamic>::Zero(size);
    running_var = Eigen::Vector<T, Eigen::Dynamic>::Ones(size);
    multiplier = Eigen::Vector<T, Eigen::Dynamic>::Ones(size);
}

template <typename T>
void BatchNorm1DLayer<T>::setGamma(const std::vector<T>& gammaVals)
{
    std::copy(gammaVals.begin(), gammaVals.end(), gamma.begin());
    updateMultiplier();
}

template <typename T>
void BatchNorm1DLayer<T>::setBeta(const std::vector<T>& betaVals)
{
    std::copy(betaVals.begin(), betaVals.end(), beta.begin());
}

template <typename T>
void BatchNorm1DLayer<T>::setRunningMean(const std::vector<T>& runningMean)
{
    std::copy(runningMean.begin(), runningMean.end(), running_mean.begin());
}

template <typename T>
void BatchNorm1DLayer<T>::setRunningVariance(const std::vector<T>& runningVar)
{
    std::copy(runningVar.begin(), runningVar.end(), running_var.begin());
    updateMultiplier();
}

template <typename T>
void BatchNorm1DLayer<T>::setEpsilon(T newEpsilon)
{
    epsilon = newEpsilon;
    updateMultiplier();
}

template <typename T>
void BatchNorm1DLayer<T>::updateMultiplier()
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
        multiplier[i] = gamma[i] / std::sqrt(running_var[i] + epsilon);
}

//============================================================
template <typename T, int size, bool affine>
BatchNorm1DT<T, size, affine>::BatchNorm1DT()
    : outs(outs_internal)
{
    gamma = Eigen::Vector<T, size>::Ones(size);
    beta = Eigen::Vector<T, size>::Zero(size);
    running_mean = Eigen::Vector<T, size>::Zero(size);
    running_var = Eigen::Vector<T, size>::Ones(size);
    multiplier = Eigen::Vector<T, size>::Ones(size);
}

template <typename T, int size, bool affine>
template <bool isAffine>
typename std::enable_if<isAffine, void>::type BatchNorm1DT<T, size, affine>::setGamma(const std::vector<T>& gammaVals)
{
    std::copy(gammaVals.begin(), gammaVals.end(), std::begin(gamma));
    updateMultiplier();
}

template <typename T, int size, bool affine>
template <bool isAffine>
typename std::enable_if<isAffine, void>::type BatchNorm1DT<T, size, affine>::setBeta(const std::vector<T>& betaVals)
{
    std::copy(betaVals.begin(), betaVals.end(), std::begin(beta));
}

template <typename T, int size, bool affine>
void BatchNorm1DT<T, size, affine>::setRunningMean(const std::vector<T>& runningMean)
{
    std::copy(runningMean.begin(), runningMean.end(), std::begin(running_mean));
}

template <typename T, int size, bool affine>
void BatchNorm1DT<T, size, affine>::setRunningVariance(const std::vector<T>& runningVar)
{
    std::copy(runningVar.begin(), runningVar.end(), std::begin(running_var));
    updateMultiplier();
}

template <typename T, int size, bool affine>
void BatchNorm1DT<T, size, affine>::setEpsilon(T newEpsilon)
{
    epsilon = newEpsilon;
    updateMultiplier();
}

template <typename T, int size, bool affine>
void BatchNorm1DT<T, size, affine>::updateMultiplier()
{
    for(int i = 0; i < out_size; ++i)
        multiplier[i] = gamma[i] / std::sqrt(running_var[i] + epsilon);
}
}
