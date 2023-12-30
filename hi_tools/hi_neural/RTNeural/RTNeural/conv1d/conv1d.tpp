#include "conv1d.h"

namespace RTNEURAL_NAMESPACE
{

#if !RTNEURAL_USE_EIGEN && !RTNEURAL_USE_XSIMD

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
    weights = new T**[out_size];
    for(int i = 0; i < out_size; ++i)
    {
        weights[i] = new T*[kernel_size];
        for(int k = 0; k < kernel_size; ++k)
        {
            weights[i][k] = new T[filters_per_group];
            std::fill(weights[i][k], weights[i][k] + filters_per_group, (T)0);
        }
    }

    bias = new T[out_size];

    state = new T*[state_size];
    for(int k = 0; k < state_size; ++k)
        state[k] = new T[in_size];

    state_cols = new T*[kernel_size];
    for(int k = 0; k < kernel_size; ++k)
        state_cols[k] = new T[filters_per_group];

    state_ptrs = new int[kernel_size];
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
    if(&other != this)
        *this = Conv1D<T>(other);

    return *this;
}

template <typename T>
Conv1D<T>::~Conv1D()
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
    {
        for(int k = 0; k < kernel_size; ++k)
            delete[] weights[i][k];

        delete[] weights[i];
    }

    delete[] weights;
    delete[] bias;

    for(int k = 0; k < state_size; ++k)
        delete[] state[k];
    delete[] state;

    for(int k = 0; k < kernel_size; ++k)
        delete[] state_cols[k];
    delete[] state_cols;

    delete[] state_ptrs;
}

template <typename T>
void Conv1D<T>::reset()
{
    for(int k = 0; k < state_size; ++k)
        std::fill(state[k], state[k] + Layer<T>::in_size, (T)0);

    for(int k = 0; k < kernel_size; ++k)
        std::fill(state_cols[k], state_cols[k] + filters_per_group, (T)0);

    for(int k = 0; k < kernel_size; ++k)
        state_ptrs[k] = 0;

    state_ptr = 0;
}

template <typename T>
void Conv1D<T>::setWeights(const std::vector<std::vector<std::vector<T>>>& ws)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
        for(int k = 0; k < filters_per_group; ++k)
            for(int j = 0; j < kernel_size; ++j)
                weights[i][j][k] = ws[i][k][j];
}

template <typename T>
void Conv1D<T>::setBias(const std::vector<T>& biasVals)
{
    for(int i = 0; i < Layer<T>::out_size; ++i)
        bias[i] = biasVals[i];
}

//====================================================
template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::Conv1DT()
{
    for(int i = 0; i < out_size; ++i)
        for(int j = 0; j < kernel_size; ++j)
            for(int k = 0; k < filters_per_group; ++k)
                weights[i][j][k] = (T)0.0;

    for(int i = 0; i < out_size; ++i)
        bias[i] = (T)0.0;

    for(int i = 0; i < out_size; ++i)
        outs[i] = (T)0.0;

    resize_state();
    reset();
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::reset()
{
    for(int i = 0; i < state_size; ++i)
        for(int k = 0; k < in_size; ++k)
            state[i][k] = (T)0.0;

    for(int i = 0; i < kernel_size; ++i)
        for(int k = 0; k < filters_per_group; ++k)
            state_cols[i][k] = (T)0.0;

    state_ptr = 0;
    for(int i = 0; i < kernel_size; ++i)
        state_ptrs[i] = 0;
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::setWeights(const std::vector<std::vector<std::vector<T>>>& ws)
{
    for(int i = 0; i < out_size; ++i)
        for(int k = 0; k < filters_per_group; ++k)
            for(int j = 0; j < kernel_size; ++j)
                weights[i][j][k] = ws[i][k][j];
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::setBias(const std::vector<T>& biasVals)
{
    for(int i = 0; i < out_size; ++i)
        bias[i] = biasVals[i];
}

#endif

} // namespace RTNEURAL_NAMESPACE
