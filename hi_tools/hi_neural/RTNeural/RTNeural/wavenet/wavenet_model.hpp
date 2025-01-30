#pragma once

#include "wavenet_layer_array.hpp"

namespace wavenet
{
template <typename T,
          int condition_size,
          typename... LayerArrays>
struct Wavenet_Model
{
    std::tuple<LayerArrays...> layer_arrays;

    static constexpr auto head_layer_n_channels = std::tuple_element_t<0, std::tuple<LayerArrays...>>::n_channels;

#if RTNEURAL_USE_EIGEN
    Eigen::Matrix<T, head_layer_n_channels, 1> head_input {};
#elif RTNEURAL_USE_XSIMD
    xsimd::batch<T> head_input[RTNeural::ceil_div (head_layer_n_channels, (int) xsimd::batch<T>::size)];
#endif
    T head_scale = (T) 0;

    Wavenet_Model() = default;

    void prewarm()
    {
        RTNeural::modelt_detail::forEachInTuple (
            [] (auto& layer, size_t)
            {
                layer.reset();
            },
            layer_arrays);
        for (int i = 0; i < 1 << 14; ++i)
            forward (0.0f);
    }

    void load_weights (const nlohmann::json& model_json)
    {
        std::vector<float> model_weights = model_json.at ("weights");
        auto weights_iterator = model_weights.begin();
        RTNeural::modelt_detail::forEachInTuple (
            [&weights_iterator] (auto& layer, size_t)
            {
                layer.load_weights (weights_iterator);
            },
            layer_arrays);

        head_scale = *weights_iterator++;

        // Make sure we use the all of the weights exactly
        assert (std::distance (model_weights.begin(), weights_iterator) == model_weights.size());
    }

    T forward (T input) noexcept
    {
#if RTNEURAL_USE_EIGEN
        const auto v_ins = Eigen::Matrix<T, 1, 1>::Constant (input);
#elif RTNEURAL_USE_XSIMD
        xsimd::batch<T> v_ins[1];
        v_ins[0] = RTNeural::set_value (v_ins[0], 0, input);
#endif
        RTNeural::modelt_detail::forEachInTuple (
            [this, v_ins] (auto& layer_array, auto index_t)
            {
                static constexpr size_t index = index_t;
                if constexpr (index == 0)
                {
#if RTNEURAL_USE_EIGEN
                    head_input.setZero();
                    Eigen::Map<Eigen::Matrix<T, head_layer_n_channels, 1>, RTNeural::RTNeuralEigenAlignment> head_input_map { head_input.data() };
                    std::get<0> (layer_arrays).forward (v_ins, v_ins, head_input_map);
#elif RTNEURAL_USE_XSIMD
                    std::fill (std::begin (head_input), std::end (head_input), xsimd::batch<T> {});
                    std::get<0> (layer_arrays).forward (v_ins, v_ins, head_input);
#endif
                }
                else
                {
                    std::get<index> (layer_arrays).forward (std::get<index - 1> (layer_arrays).layer_outputs, v_ins, std::get<index - 1> (layer_arrays).head_outputs);
                }
            },
            layer_arrays);

#if RTNEURAL_USE_EIGEN
        return std::get<std::tuple_size_v<decltype (layer_arrays)> - 1> (layer_arrays).head_outputs[0] * head_scale;
#elif RTNEURAL_USE_XSIMD
        return std::get<std::tuple_size_v<decltype (layer_arrays)> - 1> (layer_arrays).head_outputs[0].get (0) * head_scale;

#endif
    }
};
} // namespace wavenet
