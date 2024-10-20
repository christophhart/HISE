#include "conv1d_stateless_eigen.h"

namespace RTNEURAL_NAMESPACE
{
template <typename T>
Conv1DStateless<T>::Conv1DStateless(int in_num_filters_in, int in_num_features_in, int in_num_filters_out, int in_kernel_size, int in_stride, bool in_valid_pad)
    : num_filters_in(in_num_filters_in)
    , num_features_in(in_num_features_in)
    , num_filters_out(in_num_filters_out)
    , kernel_size(in_kernel_size)
    , stride(in_stride)
    , valid_pad(in_valid_pad)
    , num_features_out(computeNumFeaturesOut(in_num_features_in, in_kernel_size, in_stride, in_valid_pad))
    , pad_left(computePadLeft(in_num_features_in, in_kernel_size, in_stride, in_valid_pad))
    , pad_right(computePadRight(in_num_features_in, in_kernel_size, in_stride, in_valid_pad))
    , Layer<T>(in_num_filters_in * in_num_features_in, in_num_filters_out * computeNumFeaturesOut(in_num_features_in, in_kernel_size, in_stride, in_valid_pad))
{
    kernelWeights.resize(num_filters_out, Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(num_filters_in, kernel_size));
}

template <typename T>
Conv1DStateless<T>::Conv1DStateless(std::initializer_list<int> sizes)
    : Conv1DStateless<T>(*sizes.begin(), *(sizes.begin() + 1), *(sizes.begin() + 2), *(sizes.begin() + 3), *(sizes.begin() + 4), *(sizes.begin() + 5))
{
}

template <typename T>
Conv1DStateless<T>::Conv1DStateless(const Conv1DStateless& other)
    : Conv1DStateless(other.num_filters_in, other.num_features_in, other.num_filters_out, other.kernel_size, other.stride, other.valid_pad)
{
}

template <typename T>
Conv1DStateless<T>& Conv1DStateless<T>::operator=(const Conv1DStateless<T>& other)
{
    return *this = Conv1DStateless<T>(other);
}

template <typename T>
void Conv1DStateless<T>::setWeights(const std::vector<std::vector<std::vector<T>>>& inWeights)
{
    for(int i = 0; i < num_filters_out; ++i)
        for(int k = 0; k < num_filters_in; ++k)
            for(int j = 0; j < kernel_size; ++j)
                kernelWeights[i](k, j) = inWeights.at(i).at(k).at(j);
}

//====================================================

template <typename T, int num_filters_in_t, int num_features_in_t, int num_filters_out_t, int kernel_size_t, int stride_t, bool valid_pad_t>
Conv1DStatelessT<T, num_filters_in_t, num_features_in_t, num_filters_out_t, kernel_size_t, stride_t, valid_pad_t>::Conv1DStatelessT()
    : outs(outs_internal)
{
    for(int k = 0; k < num_filters_out_t; ++k)
        kernelWeights[k] = weights_type::Zero();
}

template <typename T, int num_filters_in_t, int num_features_in_t, int num_filters_out_t, int kernel_size_t, int stride_t, bool valid_pad_t>
void Conv1DStatelessT<T, num_filters_in_t, num_features_in_t, num_filters_out_t, kernel_size_t, stride_t, valid_pad_t>::setWeights(const std::vector<std::vector<std::vector<T>>>& inWeights)
{
    for(int i = 0; i < num_filters_out_t; ++i)
        for(int k = 0; k < num_filters_in_t; ++k)
            for(int j = 0; j < kernel_size_t; ++j)
                kernelWeights[i](k, j) = inWeights.at(i).at(k).at(j);
}

template <typename T, int num_filters_in_t, int num_features_in_t, int num_filters_out_t, int kernel_size_t, int stride_t, bool valid_pad_t>
void Conv1DStatelessT<T, num_filters_in_t, num_features_in_t, num_filters_out_t, kernel_size_t, stride_t, valid_pad_t>::setWeightsTransposed(const std::vector<std::vector<std::vector<T>>>& inWeights)
{
    for(int i = 0; i < num_filters_out_t; ++i)
        for(int k = 0; k < num_filters_in_t; ++k)
            for(int j = 0; j < kernel_size_t; ++j)
                kernelWeights[i](k, j) = inWeights.at(j).at(k).at(i);
}
} // RTNEURAL_NAMESPACE
