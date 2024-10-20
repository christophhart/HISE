#pragma once

#include "model_loader.h"

namespace RTNEURAL_NAMESPACE
{

#ifndef DOXYGEN
/**
 * Some utilities for constructing and working
 * with variadic templates of layers.
 *
 * Note that this API may change at any time,
 * so probably don't use any of this directly.
 */
namespace modelt_detail
{
    /** utils for making offset index sequences */
    template <std::size_t N, typename Seq>
    struct offset_sequence;

    template <std::size_t N, std::size_t... Ints>
    struct offset_sequence<N, std::index_sequence<Ints...>>
    {
        using type = std::index_sequence<Ints + N...>;
    };
    template <std::size_t N, typename Seq>
    using offset_sequence_t = typename offset_sequence<N, Seq>::type;

    /** Functions to do a function for each element in the tuple */
    template <typename Fn, typename Tuple, size_t... Ix>
    constexpr void forEachInTuple(Fn&& fn, Tuple&& tuple, std::index_sequence<Ix...>) noexcept(noexcept(std::initializer_list<int> { (fn(std::get<Ix>(tuple), Ix), 0)... }))
    {
        (void)std::initializer_list<int> { ((void)fn(std::get<Ix>(tuple), Ix), 0)... };
    }

    template <typename T>
    using TupleIndexSequence = std::make_index_sequence<std::tuple_size<std::remove_cv_t<std::remove_reference_t<T>>>::value>;

    template <typename Fn, typename Tuple>
    constexpr void forEachInTuple(Fn&& fn, Tuple&& tuple) noexcept(noexcept(forEachInTuple(std::forward<Fn>(fn), std::forward<Tuple>(tuple), TupleIndexSequence<Tuple> {})))
    {
        forEachInTuple(std::forward<Fn>(fn), std::forward<Tuple>(tuple), TupleIndexSequence<Tuple> {});
    }

    template <size_t start, size_t num>
    using TupleIndexSequenceRange = offset_sequence_t<start, std::make_index_sequence<num>>;

    template <size_t start, size_t num, typename Fn, typename Tuple>
    constexpr void forEachInTupleRange(Fn&& fn, Tuple&& tuple) noexcept(noexcept(forEachInTuple(std::forward<Fn>(fn), std::forward<Tuple>(tuple), TupleIndexSequenceRange<start, num> {})))
    {
        forEachInTuple(std::forward<Fn>(fn), std::forward<Tuple>(tuple), TupleIndexSequenceRange<start, num> {});
    }

    // unrolled loop for forward inferencing
    template <size_t idx, size_t Niter>
    struct forward_unroll
    {
        template <typename T>
        static void call(T& t)
        {
            std::get<idx>(t).forward(std::get<idx - 1>(t).outs);
            forward_unroll<idx + 1, Niter - 1>::call(t);
        }
    };

    template <size_t idx>
    struct forward_unroll<idx, 0>
    {
        template <typename T>
        static void call(T&) { }
    };

    template <typename T, typename LayerType>
    void loadLayer(LayerType&, int&, const nlohmann::json&, const std::string&, int, bool debug)
    {
        json_parser::debug_print("Loading a no-op layer!", debug);
    }

    template <typename T, int in_size, int out_size>
    void loadLayer(DenseT<T, in_size, out_size>& dense, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];

        if(checkDense<T>(dense, type, layerDims, debug))
            loadDense<T>(dense, weights);

        if(!l.contains("activation"))
        {
            json_stream_idx++;
        }
        else
        {
            const auto activationType = l["activation"].get<std::string>();
            if(activationType.empty())
                json_stream_idx++;
        }
    }

    template <typename T, int in_size, int out_size, int kernel_size, int dilation_rate, int groups, bool dynamic_state>
    void loadLayer(Conv1DT<T, in_size, out_size, kernel_size, dilation_rate, groups, dynamic_state>& conv, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& l_weights = l["weights"];
        const auto l_kernel = l["kernel_size"].back().get<int>();
        const auto l_dilation = l["dilation"].back().get<int>();
        const auto l_groups = l.value("groups", 1);

        if(checkConv1D<T>(conv, type, layerDims, l_kernel, l_dilation, l_groups, debug))
            loadConv1D<T>(conv, l_kernel, l_dilation, l_weights);

        if(!l.contains("activation"))
        {
            json_stream_idx++;
        }
        else
        {
            const auto activationType = l["activation"].get<std::string>();
            if(activationType.empty())
                json_stream_idx++;
        }
    }
    template <typename T, int num_filters_in_t, int num_filters_out_t, int num_features_in_t, int kernel_size_time_t,
        int kernel_size_feature_t, int dilation_rate_t, int stride_t, bool valid_pad_t>
    void loadLayer(Conv2DT<T, num_filters_in_t, num_filters_out_t, num_features_in_t, kernel_size_time_t,
                       kernel_size_feature_t, dilation_rate_t, stride_t, valid_pad_t>& conv,
        int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];
        const auto kernel_time = l["kernel_size_time"].back().get<int>();
        const auto kernel_feature = l["kernel_size_feature"].back().get<int>();

        const auto dilation = l["dilation"].back().get<int>();
        const auto strides = l["strides"].back().get<int>();
        const bool valid_pad = l["padding"].get<std::string>() == "valid";

        if(checkConv2D<T>(conv, type, layerDims, kernel_time, kernel_feature, dilation, strides, valid_pad, debug))
            loadConv2D<T>(conv, weights);

        if(!l.contains("activation"))
        {
            json_stream_idx++;
        }
        else
        {
            const auto activationType = l["activation"].get<std::string>();
            if(activationType.empty())
                json_stream_idx++;
        }
    }

    template <typename T, int in_size, int out_size, SampleRateCorrectionMode mode>
    void loadLayer(GRULayerT<T, in_size, out_size, mode>& gru, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];

        if(checkGRU<T>(gru, type, layerDims, debug))
            loadGRU<T>(gru, weights);

        json_stream_idx++;
    }

    template <typename T, int in_size, int out_size, SampleRateCorrectionMode mode>
    void loadLayer(LSTMLayerT<T, in_size, out_size, mode>& lstm, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];

        if(checkLSTM<T>(lstm, type, layerDims, debug))
            loadLSTM<T>(lstm, weights);

        json_stream_idx++;
    }

    template <typename T, int size>
    void loadLayer(PReLUActivationT<T, size>& prelu, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];

        if(checkPReLU<T>(prelu, type, layerDims, debug))
            loadPReLU<T>(prelu, weights);

        json_stream_idx++;
    }

    template <typename T, int size, bool affine>
    void loadLayer(BatchNorm1DT<T, size, affine>& batch_norm, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];

        if(checkBatchNorm<T>(batch_norm, type, layerDims, weights, debug))
        {
            loadBatchNorm<T>(batch_norm, weights);
            batch_norm.setEpsilon(l["epsilon"].get<float>());
        }

        json_stream_idx++;
    }

    template <typename T, int num_filters, int num_features, bool affine>
    void loadLayer(BatchNorm2DT<T, num_filters, num_features, affine>& batch_norm, int& json_stream_idx, const nlohmann::json& l,
        const std::string& type, int layerDims, bool debug)
    {
        using namespace json_parser;

        debug_print("Layer: " + type, debug);
        debug_print("  Dims: " + std::to_string(layerDims), debug);
        const auto& weights = l["weights"];

        if(checkBatchNorm2D<T>(batch_norm, type, layerDims, weights, debug))
        {
            loadBatchNorm<T>(batch_norm, weights);
            batch_norm.setEpsilon(l["epsilon"].get<float>());
        }

        json_stream_idx++;
    }

    template <typename T, int in_size, typename... Layers>
    void parseJson(const nlohmann::json& parent, std::tuple<Layers...>& layers, const bool debug = false, std::initializer_list<std::string> custom_layers = {})
    {
        using namespace json_parser;

        auto shape = parent["in_shape"];
        auto json_layers = parent["layers"];

        if(!shape.is_array() || !json_layers.is_array())
            return;

        // If 4D: nDims is num_features * num_channels
        const int nDims = shape.size() == 4 ? shape[2].get<int>() * shape[3].get<int>() : shape.back().get<int>();

        debug_print("# dimensions: " + std::to_string(nDims), debug);

        if(nDims != in_size)
        {
            debug_print("Incorrect input size!", debug);
            return;
        }

        int json_stream_idx = 0;
        modelt_detail::forEachInTuple([&](auto& layer, size_t)
            {
                if(json_stream_idx >= (int)json_layers.size())
                {
                    debug_print("Too many layers!", debug);
                    return;
                }

                const auto l = json_layers.at(json_stream_idx);
                const auto type = l["type"].get<std::string>();
                const auto layerShape = l["shape"];

                // If 4D: layerDims is num_features * num_channels
                const int layerDims = layerShape.size() == 4 ? layerShape[2].get<int>() * layerShape[3].get<int>() : layerShape.back().get<int>();

                if(layer.isActivation()) // activation layers don't need initialisation
                {
                    if(!l.contains("activation"))
                    {
                        debug_print("No activation layer expected!", debug);
                        return;
                    }

                    const auto activationType = l["activation"].get<std::string>();
                    if(!activationType.empty())
                    {
                        debug_print("  activation: " + activationType, debug);
                        checkActivation(layer, activationType, layerDims, debug);
                    }

                    json_stream_idx++;
                    return;
                }

                if(std::find(custom_layers.begin(), custom_layers.end(), type) != custom_layers.end())
                {
                    debug_print ("Skipping loading weights for custom layer: " + type, debug);
                    json_stream_idx++;
                    return;
                }

                modelt_detail::loadLayer<T>(layer, json_stream_idx, l, type, layerDims, debug); },
            layers);
    }
} // namespace modelt_detail
#endif // DOXYGEN

/**
 *  A static sequential neural network model.
 *
 *  To use this class, you must define the layers at compile-time:
 *  ```
 *  ModelT<double, 1, 1,
 *      DenseT<double, 1, 8>,
 *      TanhActivationT<double, 8>,
 *      DenseT<double, 8, 1>
 *  > model;
 *  ```
 */
template <typename T, int in_size, int out_size, typename... Layers>
class ModelT
{
public:
    static constexpr auto input_size = in_size;
    static constexpr auto output_size = out_size;

    ModelT()
    {
#if RTNEURAL_USE_XSIMD
        for(int i = 0; i < v_in_size; ++i)
            v_ins[i] = v_type((T)0);
#elif RTNEURAL_USE_EIGEN
        auto& layer_outs = get<n_layers - 1>().outs;
        new(&layer_outs) Eigen::Map<Eigen::Matrix<T, out_size, 1>, RTNeuralEigenAlignment>(outs);
#endif
    }

    /** Get a reference to the layer at index `Index`. */
    template <int Index>
    RTNEURAL_REALTIME auto& get() noexcept
    {
        return std::get<Index>(layers);
    }

    /** Get a reference to the layer at index `Index`. */
    template <int Index>
    RTNEURAL_REALTIME const auto& get() const noexcept
    {
        return std::get<Index>(layers);
    }

    /** Resets the state of the network layers. */
    RTNEURAL_REALTIME void reset()
    {
        modelt_detail::forEachInTuple([&](auto& layer, size_t)
            { layer.reset(); },
            layers);
    }

    /** Performs forward propagation for this model. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<(N > 1), T>::type
    forward(const T* input)
    {
#if RTNEURAL_USE_XSIMD
        for(int i = 0; i < v_in_size; ++i)
            v_ins[i] = xsimd::load_aligned(input + i * v_size);
#elif RTNEURAL_USE_EIGEN
        auto v_ins = Eigen::Map<const vec_type, RTNeuralEigenAlignment>(input);
#else // RTNEURAL_USE_STL
        std::copy(input, input + in_size, v_ins);
#endif
        std::get<0>(layers).forward(v_ins);
        modelt_detail::forward_unroll<1, n_layers - 1>::call(layers);

#if RTNEURAL_USE_XSIMD
        for(int i = 0; i < v_out_size; ++i)
            xsimd::store_aligned(outs + i * v_size, get<n_layers - 1>().outs[i]);
#elif RTNEURAL_USE_EIGEN
#else // RTNEURAL_USE_STL
        auto& layer_outs = get<n_layers - 1>().outs;
        std::copy(layer_outs, layer_outs + out_size, outs);
#endif
        return outs[0];
    }

    /** Performs forward propagation for this model. */
    template <int N = in_size>
    RTNEURAL_REALTIME inline typename std::enable_if<N == 1, T>::type
    forward(const T* input)
    {
#if RTNEURAL_USE_XSIMD
        v_ins[0] = (v_type)input[0];
#elif RTNEURAL_USE_EIGEN
        const auto v_ins = vec_type::Constant(input[0]);
#else // RTNEURAL_USE_STL
        v_ins[0] = input[0];
#endif

        std::get<0>(layers).forward(v_ins);
        modelt_detail::forward_unroll<1, n_layers - 1>::call(layers);

#if RTNEURAL_USE_XSIMD
        for(int i = 0; i < v_out_size; ++i)
            xsimd::store_aligned(outs + i * v_size, get<n_layers - 1>().outs[i]);
#elif RTNEURAL_USE_EIGEN
#else // RTNEURAL_USE_STL
        auto& layer_outs = get<n_layers - 1>().outs;
        std::copy(layer_outs, layer_outs + out_size, outs);
#endif
        return outs[0];
    }

    /** Returns a pointer to the output of the final layer in the network. */
    RTNEURAL_REALTIME inline const T* getOutputs() const noexcept
    {
        return outs;
    }

    /** Loads neural network model weights from a json stream. */
    void parseJson(const nlohmann::json& parent, const bool debug = false, std::initializer_list<std::string> custom_layers = {})
    {
        modelt_detail::parseJson<T, in_size>(parent, layers, debug, custom_layers);
    }

    /** Loads neural network model weights from a json stream. */
    void parseJson(std::ifstream& jsonStream, const bool debug = false, std::initializer_list<std::string> custom_layers = {})
    {
        nlohmann::json parent;
        jsonStream >> parent;
        return parseJson(parent, debug, custom_layers);
    }

private:
#if RTNEURAL_USE_XSIMD
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_in_size = ceil_div(in_size, v_size);
    static constexpr auto v_out_size = ceil_div(out_size, v_size);
    v_type v_ins[v_in_size];
#elif RTNEURAL_USE_EIGEN
    using vec_type = Eigen::Matrix<T, in_size, 1>;
#else // RTNEURAL_USE_STL
    T v_ins alignas(RTNEURAL_DEFAULT_ALIGNMENT)[in_size];
#endif

#if RTNEURAL_USE_XSIMD
    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[v_out_size * v_size];
#else
    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];
#endif

    std::tuple<Layers...> layers;
    static constexpr size_t n_layers = sizeof...(Layers);
};

#if RTNEURAL_USE_EIGEN || !RTNEURAL_USE_XSIMD
/** A static sequential 2D neural network model. */
template <typename T, int num_filters_in, int num_features_in, int num_filters_out, int num_features_out, typename... Layers>
using ModelT2D = ModelT<T, num_filters_in * num_features_in, num_filters_out * num_features_out, Layers...>;
#else
/**
 * A static sequential 2D neural network model.
 *
 * This API is still somewhat unstable, so maybe hold off on using it for now,
 * unless you're in the mood to help with debugging.
 */
template <typename T, int num_filters_in, int num_features_in, int num_filters_out, int num_features_out, typename... Layers>
class ModelT2D
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_num_filters_in = ceil_div(num_filters_in, v_size);
    static constexpr auto v_num_filters_out = ceil_div(num_filters_out, v_size);
    static constexpr auto v_in_size = v_num_filters_in * num_features_in;
    static constexpr auto v_out_size = v_num_filters_out * num_features_out;

public:
    static constexpr auto input_size = num_filters_in * num_features_in;
    static constexpr auto output_size = num_filters_out * num_features_out;
    static constexpr auto input_size_padded = v_in_size * v_size;
    static constexpr auto output_size_padded = v_out_size * v_size;

    ModelT2D()
    {
        for(int i = 0; i < v_in_size; ++i)
            v_ins[i] = v_type((T)0);
    }

    /** Get a reference to the layer at index `Index`. */
    template <int Index>
    auto& get() noexcept
    {
        return std::get<Index>(layers);
    }

    /** Get a reference to the layer at index `Index`. */
    template <int Index>
    const auto& get() const noexcept
    {
        return std::get<Index>(layers);
    }

    /** Resets the state of the network layers. */
    void reset()
    {
        modelt_detail::forEachInTuple([&](auto& layer, size_t)
            { layer.reset(); },
            layers);
    }

    /** Performs forward propagation for this model. */
    inline T forward(const T* input)
    {
        for(int feature_index = 0; feature_index < num_features_in; ++feature_index)
        {
            alignas(RTNEURAL_DEFAULT_ALIGNMENT) T load_arr[v_size * v_num_filters_in] {};
            std::copy(input + feature_index * num_filters_in, input + feature_index * num_filters_in + num_filters_in, std::begin(load_arr));
            for(int i = 0; i < v_num_filters_in; ++i)
                v_ins[feature_index * num_filters_in + i] = xsimd::load_aligned(load_arr + i * v_size);
        }
        std::get<0>(layers).forward(v_ins);
        modelt_detail::forward_unroll<1, n_layers - 1>::call(layers);

        for(int feature_index = 0; feature_index < num_features_out; ++feature_index)
        {
            alignas(RTNEURAL_DEFAULT_ALIGNMENT) T store_arr[v_size * v_num_filters_out] {};
            for(int i = 0; i < v_num_filters_out; ++i)
                xsimd::store_aligned(store_arr + i * v_size, get<n_layers - 1>().outs[feature_index * num_filters_out + i]);
            std::copy(std::begin(store_arr), std::begin(store_arr) + num_filters_out, outs + feature_index * num_filters_out);
        }

        return outs[0];
    }

    /** Returns a pointer to the output of the final layer in the network. */
    inline const T* getOutputs() const noexcept
    {
        return outs;
    }

    /** Loads neural network model weights from a json stream. */
    void parseJson(const nlohmann::json& parent, const bool debug = false, std::initializer_list<std::string> custom_layers = {})
    {
        modelt_detail::parseJson<T, input_size>(parent, layers, debug, custom_layers);
    }

    /** Loads neural network model weights from a json stream. */
    void parseJson(std::ifstream& jsonStream, const bool debug = false, std::initializer_list<std::string> custom_layers = {})
    {
        nlohmann::json parent;
        jsonStream >> parent;
        return parseJson(parent, debug, custom_layers);
    }

private:
    v_type v_ins[v_in_size] {};

    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[output_size] {};

    std::tuple<Layers...> layers;
    static constexpr size_t n_layers = sizeof...(Layers);
};
#endif // RTNEURAL_USE_XSIMD
} // namespace RTNEURAL_NAMESPACE
