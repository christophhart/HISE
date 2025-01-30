#pragma once

#include "wavenet_layer.hpp"

namespace wavenet
{
template <int... values>
using Dilations = std::integer_sequence<int, values...>;

template <typename T,
          int in_size,
          int condition_size,
          int head_size,
          int channels,
          int kernel_size,
          typename DilationsSequence,
          bool has_head_bias,
          typename MathsProvider>
struct Layer_Array
{
    template <typename>
    struct Layers_Helper
    {
    };

    template <int... dilation_vals>
    struct Layers_Helper<Dilations<dilation_vals...>>
    {
        using type = std::tuple<Wavenet_Layer<T, condition_size, channels, kernel_size, dilation_vals, MathsProvider>...>;
    };

    using Layers = typename Layers_Helper<DilationsSequence>::type;

    static constexpr auto n_channels = channels;

    RTNeural::DenseT<T, in_size, channels> rechannel; // no bias!
    Layers layers;
    static constexpr auto num_layers = std::tuple_size_v<decltype (layers)>;
    RTNeural::DenseT<T, channels, head_size, has_head_bias> head_rechannel;

    using Last_Layer_Type = std::remove_reference_t<decltype (std::get<std::tuple_size_v<decltype (layers)> - 1> (layers))>;
    decltype (Last_Layer_Type::outs)& layer_outputs { std::get<std::tuple_size_v<decltype (layers)> - 1> (layers).outs };
    decltype (RTNeural::DenseT<T, channels, head_size>::outs)& head_outputs { head_rechannel.outs };

    void reset()
    {
        RTNeural::modelt_detail::forEachInTuple ([] (auto& layer, size_t)
                                                 { layer.reset(); },
                                                 layers);
    }

    void load_weights (std::vector<float>::iterator& weights)
    {
        std::vector<std::vector<float>> rechannel_weights (channels, std::vector<float> (in_size));
        for (int i = 0; i < channels; i++)
            for (int j = 0; j < in_size; j++)
                rechannel_weights[i][j] = *(weights++);
        rechannel.setWeights (rechannel_weights);

        RTNeural::modelt_detail::forEachInTuple ([&weights] (auto& layer, size_t)
                                                 { layer.load_weights (weights); },
                                                 layers);

        std::vector<std::vector<float>> head_rechannel_weights (head_size, std::vector<float> (channels));
        for (int i = 0; i < head_size; i++)
            for (int j = 0; j < channels; j++)
                head_rechannel_weights[i][j] = *(weights++);
        head_rechannel.setWeights (head_rechannel_weights);

        if constexpr (has_head_bias)
        {
            std::vector<float> head_rechannel_bias (head_size);
            for (int i = 0; i < head_size; i++)
                head_rechannel_bias[i] = *(weights++);
            head_rechannel.setBias (head_rechannel_bias.data());
        }
    }

#if RTNEURAL_USE_EIGEN
    void forward (const Eigen::Matrix<T, in_size, 1>& ins,
                  const Eigen::Matrix<T, condition_size, 1>& condition,
                  Eigen::Map<Eigen::Matrix<T, channels, 1>, RTNeural::RTNeuralEigenAlignment>& head_io)
#elif RTNEURAL_USE_XSIMD
    void forward (const xsimd::batch<T> (&ins)[RTNeural::ceil_div (in_size, (int) xsimd::batch<T>::size)],
                  const xsimd::batch<T> (&condition)[RTNeural::ceil_div (condition_size, (int) xsimd::batch<T>::size)],
                  xsimd::batch<T> (&head_io)[RTNeural::ceil_div (channels, (int) xsimd::batch<T>::size)])
#endif
    {
        rechannel.forward (ins);

        RTNeural::modelt_detail::forEachInTuple (
            [&] (auto& layer, auto index_t)
            {
                static constexpr size_t index = index_t;
                if constexpr (index == 0)
                    layer.forward (rechannel.outs, condition, head_io);
                else
                    layer.forward (std::get<index - 1> (layers).outs, condition, head_io);
            },
            layers);

        head_rechannel.forward (head_io);
    }
};
} // namespace wavenet
