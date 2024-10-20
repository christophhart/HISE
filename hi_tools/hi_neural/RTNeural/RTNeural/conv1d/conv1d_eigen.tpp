#include "conv1d_eigen.h"

namespace RTNEURAL_NAMESPACE
{

template <typename T>
Conv1D<T>::Conv1D(int in_size, int out_size, int kernel_size, int dilation, int num_groups)
    : Layer<T>(in_size, out_size)
    , dilation_rate(dilation)
    , kernel_size(kernel_size)
    , state_size((kernel_size - 1) * dilation + 1)
    , groups(num_groups)
    , filters_per_group(in_size / groups)
    , channels_per_group(out_size / groups)
{
    kernelWeights.resize(out_size);
    for(int i = 0; i < out_size; ++i)
        kernelWeights[i] = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(filters_per_group, kernel_size);

    bias = Eigen::Vector<T, Eigen::Dynamic>::Zero(out_size);
    state = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(in_size, state_size);
    state_cols = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>::Zero(filters_per_group, kernel_size);
    state_ptrs = Eigen::Vector<int, Eigen::Dynamic>::Zero(kernel_size);
}

template <typename T>
Conv1D<T>::Conv1D(std::initializer_list<int> sizes)
    : Conv1D<T>(*sizes.begin(), *(sizes.begin() + 1), *(sizes.begin() + 2), *(sizes.begin() + 3))
{
}

template <typename T>
Conv1D<T>::Conv1D(const Conv1D<T>& other)
    : Conv1D<T>(other.in_size, other.out_size, other.kernel_size, other.dilation_rate)
{
}

template <typename T>
Conv1D<T>& Conv1D<T>::operator=(const Conv1D<T>& other)
{
    return *this = Conv1D<T>(other);
}

template <typename T>
Conv1D<T>::~Conv1D() = default;

template <typename T>
void Conv1D<T>::reset()
{
    state_ptr = 0;
    state_ptrs.setZero();
    state_cols.setZero();
    state.setZero();
}

template <typename T>
void Conv1D<T>::setWeights(const std::vector<std::vector<std::vector<T>>>& weights)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
        for(int k = 0; k < filters_per_group; ++k)
            for(int j = 0; j < kernel_size; ++j)
                kernelWeights[i](k, j) = weights[i][k][j];
}

template <typename T>
void Conv1D<T>::setBias(const std::vector<T>& biasVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
        bias(i) = biasVals[i];
}

//====================================================
template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::Conv1DT()
    : outs(outs_internal)
{
    for(int k = 0; k < out_size; ++k)
        weights[k] = weights_type::Zero();

    bias = vec_type::Zero();

    resize_state();
    reset();
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::reset()
{
    state.setZero();
    state_cols = weights_type::Zero();
    state_ptrs = state_ptrs_type::Zero();
    state_ptr = 0;
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::setWeights(const std::vector<std::vector<std::vector<T>>>& ws)
{
    for(int i = 0; i < out_size; ++i)
        for(int k = 0; k < filters_per_group; ++k)
            for(int j = 0; j < kernel_size; ++j)
                weights[i](k, j) = ws[i][k][j];
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::setBias(const std::vector<T>& biasVals)
{
    for(int i = 0; i < out_size; ++i)
        bias(i) = biasVals[i];
}

} // namespace RTNEURAL_NAMESPACE
