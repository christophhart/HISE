#include "batchnorm2d_eigen.h"

namespace RTNEURAL_NAMESPACE
{
template <typename T>
BatchNorm2DLayer<T>::BatchNorm2DLayer(int in_num_filters, int in_num_features)
    : Layer<T>(in_num_filters * in_num_features, in_num_filters * in_num_features)
    , num_filters(in_num_filters)
    , num_features(in_num_features)
{
    gamma = Eigen::Vector<T, Eigen::Dynamic>::Ones(num_filters);
    beta = Eigen::Vector<T, Eigen::Dynamic>::Zero(num_filters);
    running_mean = Eigen::Vector<T, Eigen::Dynamic>::Zero(num_filters);
    running_var = Eigen::Vector<T, Eigen::Dynamic>::Ones(num_filters);
    multiplier = Eigen::Vector<T, Eigen::Dynamic>::Ones(num_filters);
}

template <typename T>
void BatchNorm2DLayer<T>::setGamma(const std::vector<T>& gammaVals)
{
    std::copy(gammaVals.begin(), gammaVals.end(), gamma.begin());
    updateMultiplier();
}

template <typename T>
void BatchNorm2DLayer<T>::setBeta(const std::vector<T>& betaVals)
{
    std::copy(betaVals.begin(), betaVals.end(), beta.begin());
}

template <typename T>
void BatchNorm2DLayer<T>::setRunningMean(const std::vector<T>& runningMean)
{
    std::copy(runningMean.begin(), runningMean.end(), running_mean.begin());
}

template <typename T>
void BatchNorm2DLayer<T>::setRunningVariance(const std::vector<T>& runningVar)
{
    std::copy(runningVar.begin(), runningVar.end(), running_var.begin());
    updateMultiplier();
}

template <typename T>
void BatchNorm2DLayer<T>::setEpsilon(T newEpsilon)
{
    epsilon = newEpsilon;
    updateMultiplier();
}

template <typename T>
void BatchNorm2DLayer<T>::updateMultiplier()
{
    for(int i = 0; i < num_filters; ++i)
        multiplier[i] = gamma[i] / std::sqrt(running_var[i] + epsilon);
}

//============================================================
template <typename T, int num_filters_t, int num_features_t, bool affine>
BatchNorm2DT<T, num_filters_t, num_features_t, affine>::BatchNorm2DT()
    : outs(outs_internal)
{
    gamma = Eigen::Vector<T, num_filters_t>::Ones(num_filters_t);
    beta = Eigen::Vector<T, num_filters_t>::Zero(num_filters_t);
    running_mean = Eigen::Vector<T, num_filters_t>::Zero(num_filters_t);
    running_var = Eigen::Vector<T, num_filters_t>::Ones(num_filters_t);
    multiplier = Eigen::Vector<T, num_filters_t>::Ones(num_filters_t);
}

template <typename T, int num_filters_t, int num_features_t, bool affine>
template <bool isAffine>
typename std::enable_if<isAffine, void>::type BatchNorm2DT<T, num_filters_t, num_features_t, affine>::setGamma(const std::vector<T>& gammaVals)
{
    std::copy(gammaVals.begin(), gammaVals.end(), std::begin(gamma));
    updateMultiplier();
}

template <typename T, int num_filters_t, int num_features_t, bool affine>
template <bool isAffine>
typename std::enable_if<isAffine, void>::type BatchNorm2DT<T, num_filters_t, num_features_t, affine>::setBeta(const std::vector<T>& betaVals)
{
    std::copy(betaVals.begin(), betaVals.end(), std::begin(beta));
}

template <typename T, int num_filters_t, int num_features_t, bool affine>
void BatchNorm2DT<T, num_filters_t, num_features_t, affine>::setRunningMean(const std::vector<T>& runningMean)
{
    std::copy(runningMean.begin(), runningMean.end(), std::begin(running_mean));
}

template <typename T, int num_filters_t, int num_features_t, bool affine>
void BatchNorm2DT<T, num_filters_t, num_features_t, affine>::setRunningVariance(const std::vector<T>& runningVar)
{
    std::copy(runningVar.begin(), runningVar.end(), std::begin(running_var));
    updateMultiplier();
}

template <typename T, int num_filters_t, int num_features_t, bool affine>
void BatchNorm2DT<T, num_filters_t, num_features_t, affine>::setEpsilon(T newEpsilon)
{
    epsilon = newEpsilon;
    updateMultiplier();
}

template <typename T, int num_filters_t, int num_features_t, bool affine>
void BatchNorm2DT<T, num_filters_t, num_features_t, affine>::updateMultiplier()
{
    for(int i = 0; i < num_filters_t; ++i)
        multiplier[i] = gamma[i] / std::sqrt(running_var[i] + epsilon);
}
}
