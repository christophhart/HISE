#include "conv1d_xsimd.h"

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
    weights = vec3_type(out_size, vec2_type(kernel_size, vec_type(filters_per_group, (T)0)));
    bias.resize(out_size, (T)0);
    state = vec2_type(state_size, vec_type(in_size, (T)0));
    state_cols = vec2_type(kernel_size, vec_type(filters_per_group, (T)0));
    state_ptrs.resize(kernel_size);
    prod_state.resize(filters_per_group);
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
    for(int k = 0; k < state_size; ++k)
        std::fill(state[k].begin(), state[k].end(), (T)0);

    for(int k = 0; k < kernel_size; ++k)
        std::fill(state_cols[k].begin(), state_cols[k].end(), (T)0);

    std::fill(state_ptrs.begin(), state_ptrs.end(), 0);
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
            for(int k = 0; k < v_filters_per_group; ++k)
                weights[i][j][k] = v_type((T)0.0);

    for(int i = 0; i < v_out_size; ++i)
        bias[i] = v_type((T)0.0);

    for(int i = 0; i < v_out_size; ++i)
        outs[i] = v_type((T)0.0);

    resize_state();
    reset();
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::reset()
{
    for(int i = 0; i < state_size; ++i)
        for(int k = 0; k < v_in_size; ++k)
            state[i][k] = v_type((T)0.0);

    for(int i = 0; i < kernel_size; ++i)
        for(int k = 0; k < v_filters_per_group; ++k)
            state_cols[i][k] = v_type((T)0.0);

    state_ptr = 0;
    for(int i = 0; i < kernel_size; ++i)
        state_ptrs[i] = 0;
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::setWeights(const std::vector<std::vector<std::vector<T>>>& ws)
{
    for(int i = 0; i < out_size; ++i)
    {
        for(int k = 0; k < filters_per_group; ++k)
        {
            for(int j = 0; j < kernel_size; ++j)
            {
                auto& w = weights[i][j][k / v_size];
                w = set_value(w, k % v_size, ws[i][k][j]);
            }
        }
    }
}

template <typename T, int in_sizet, int out_sizet, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
void Conv1DT<T, in_sizet, out_sizet, kernel_size, dilation_rate, groups, dynamic_state>::setBias(const std::vector<T>& biasVals)
{
    for(int i = 0; i < out_size; ++i)
        bias[i / v_size] = set_value(bias[i / v_size], i % v_size, biasVals[i]);
}

} // namespace RTNEURAL_NAMESPACE
