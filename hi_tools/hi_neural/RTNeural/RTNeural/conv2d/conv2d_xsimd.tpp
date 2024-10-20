#include "conv2d_xsimd.h"

namespace RTNEURAL_NAMESPACE
{
template <typename T>
Conv2D<T>::Conv2D(int in_num_filters_in, int in_num_filters_out, int in_num_features_in, int in_kernel_size_time, int in_kernel_size_feature,
    int in_dilation_rate, int in_stride, bool in_valid_pad)
    : num_filters_in(in_num_filters_in)
    , num_filters_out(in_num_filters_out)
    , num_features_in(in_num_features_in)
    , kernel_size_time(in_kernel_size_time)
    , kernel_size_feature(in_kernel_size_feature)
    , dilation_rate(in_dilation_rate)
    , stride(in_stride)
    , num_features_out(Conv1DStateless<T>::computeNumFeaturesOut(in_num_features_in, in_kernel_size_feature, in_stride, in_valid_pad))
    , receptive_field(1 + (in_kernel_size_time - 1) * in_dilation_rate) // See "Dilated (atrous) convolution" note here: https://distill.pub/2019/computing-receptive-fields/
    , valid_pad(in_valid_pad)
    , Layer<T>(in_num_features_in * in_num_filters_in, Conv1DStateless<T>::computeNumFeaturesOut(in_num_features_in, in_kernel_size_feature, in_stride, in_valid_pad) * in_num_filters_out)
{
    conv1dLayers.resize(kernel_size_time, Conv1DStateless<T>(num_filters_in, num_features_in, num_filters_out, kernel_size_feature, stride, valid_pad));
    bias.resize(num_filters_out, (T)0);

    state.resize(receptive_field);
    for(auto& stateMat : state)
    {
        stateMat.resize(num_filters_out * num_features_out, (T)0);
    }
}

template <typename T>
Conv2D<T>::Conv2D(std::initializer_list<int> sizes)
    : Conv2D<T>(*sizes.begin(), *(sizes.begin() + 1), *(sizes.begin() + 2), *(sizes.begin() + 3), *(sizes.begin() + 4),
        *(sizes.begin() + 5), *(sizes.begin() + 6), *(sizes.begin() + 7))
{
}

template <typename T>
Conv2D<T>::Conv2D(const Conv2D& other)
    : Conv2D<T>(other.num_filters_in, other.num_filters_out, other.num_features_in, other.kernel_size_time, other.kernel_size_feature,
        other.dilation_rate, other.stride, other.valid_pad)
{
}

template <typename T>
Conv2D<T>& Conv2D<T>::operator=(const Conv2D& other)
{
    return *this = Conv2D<T>(other);
}

template <typename T>
void Conv2D<T>::setWeights(const std::vector<std::vector<std::vector<std::vector<T>>>>& inWeights)
{
    for(int i = 0; i < kernel_size_time; i++)
    {
        conv1dLayers[i].setWeights(inWeights[i]);
    }
}

template <typename T>
void Conv2D<T>::setBias(const std::vector<T>& inBias)
{
    std::copy(inBias.begin(), inBias.end(), bias.begin());
}

template <typename T, int num_filters_in_t, int num_filters_out_t, int num_features_in_t, int kernel_size_time_t, int kernel_size_feature_t, int dilation_rate_t, int stride_t, bool valid_pad_t>
Conv2DT<T, num_filters_in_t, num_filters_out_t, num_features_in_t, kernel_size_time_t, kernel_size_feature_t, dilation_rate_t, stride_t, valid_pad_t>::Conv2DT()
{
}

template <typename T, int num_filters_in_t, int num_filters_out_t, int num_features_in_t, int kernel_size_time_t,
    int kernel_size_feature_t, int dilation_rate_t, int stride_t, bool valid_pad_t>
void Conv2DT<T, num_filters_in_t, num_filters_out_t, num_features_in_t, kernel_size_time_t, kernel_size_feature_t,
    dilation_rate_t, stride_t, valid_pad_t>::setWeights(const std::vector<std::vector<std::vector<std::vector<T>>>>& inWeights)
{
    for(int i = 0; i < kernel_size_time_t; i++)
    {
        conv1dLayers[i].setWeights(inWeights[i]);
    }
}

template <typename T, int num_filters_in_t, int num_filters_out_t, int num_features_in_t, int kernel_size_time_t,
    int kernel_size_feature_t, int dilation_rate_t, int stride_t, bool valid_pad_t>
void Conv2DT<T, num_filters_in_t, num_filters_out_t, num_features_in_t, kernel_size_time_t,
    kernel_size_feature_t, dilation_rate_t, stride_t, valid_pad_t>::setBias(const std::vector<T>& inBias)
{
    std::copy(inBias.begin(), inBias.end(), reinterpret_cast<T*>(std::begin(bias)));
}
} // RTNEURAL_NAMESPACE
