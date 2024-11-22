#pragma once

#include "model_loader.h"

namespace RTNEURAL_NAMESPACE
{
namespace torch_helpers
{
    namespace detail
    {
        /** Torch Conv1D layers store their kernel weights in reverse order. */
        template <typename T>
        void reverseKernels(std::vector<std::vector<std::vector<T>>>& conv_weights)
        {
            for(auto& channel_weights : conv_weights)
            {
                for(auto& kernel : channel_weights)
                {
                    std::reverse(kernel.begin(), kernel.end());
                }
            }
        }

        /** Reverses the channel order for 1D convolutional layer weights */
        template <typename T>
        std::vector<std::vector<std::vector<T>>>
        reverseChannels(std::vector<std::vector<std::vector<T>>>& conv_weights)
        {
            std::vector<std::vector<std::vector<T>>> aux { conv_weights[0].size() };

            // Reverse channels in auxiliary variable
            for(size_t j = 0; j < conv_weights[0].size(); j++)
            {
                aux[j].resize(conv_weights.size());
                for(size_t i = 0; i < conv_weights.size(); i++)
                {
                    aux[j][i].resize(conv_weights[i][j].size());
                    std::copy(conv_weights[i][j].begin(),
                        conv_weights[i][j].end(),
                        aux[j][i].begin());
                }
            }

            return aux;
        }

        /** Transposes the rows and columns of a matrix stored as a 2D vector. */
        template <typename T>
        std::vector<std::vector<T>> transpose(const std::vector<std::vector<T>>& x)
        {
            auto outer_size = x.size();
            auto inner_size = x[0].size();
            std::vector<std::vector<T>> y(inner_size, std::vector<T>(outer_size, (T)0));

            for(size_t i = 0; i < outer_size; ++i)
            {
                for(size_t j = 0; j < inner_size; ++j)
                    y[j][i] = x[i][j];
            }

            return y;
        }

        /** Swaps the "r" and "z" indexes of a GRU layer weights. */
        template <typename T>
        void swap_rz(std::vector<std::vector<T>>& vec2d, int size)
        {
            for(auto& vec : vec2d)
                std::swap_ranges(vec.begin(), vec.begin() + size, vec.begin() + size);
        }
    }

    /** Loads a Dense layer from a JSON object containing a PyTorch state_dict. */
    template <typename T, typename DenseType>
    void loadDense(const nlohmann::json& modelJson, const std::string& layerPrefix, DenseType& dense, bool hasBias = true)
    {
        const std::vector<std::vector<T>> dense_weights = modelJson.at(layerPrefix + "weight");
        dense.setWeights(dense_weights);

        RTNEURAL_IF_CONSTEXPR(DenseType::dense_has_bias)
        {
            if(hasBias)
            {
                const std::vector<T> dense_bias = modelJson.at(layerPrefix + "bias");
                dense.setBias(dense_bias.data());
            }
            else
            {
                const std::vector<T> dense_bias((size_t)dense.out_size, (T)0);
                dense.setBias(dense_bias.data());
            }
        }
    }

    /** Loads a ConvTranspose1D layer from a JSON object containing a PyTorch state_dict. */
    template <typename T, typename Conv1DType>
    void loadConvTranspose1D(const nlohmann::json& modelJson, const std::string& layerPrefix, Conv1DType& conv, bool hasBias = true)
    {
        std::vector<std::vector<std::vector<T>>> conv_weights = modelJson.at(layerPrefix + "weight");
        conv_weights = detail::reverseChannels<T>(conv_weights);
        conv.setWeights(conv_weights);

        if(hasBias)
        {
            std::vector<T> conv_bias = modelJson.at(layerPrefix + "bias");
            conv.setBias(conv_bias);
        }
        else
        {
            std::vector<T> conv_bias((size_t)conv.out_size, (T)0);
            conv.setBias(conv_bias);
        }
    }

    /** Loads a Conv1D layer from a JSON object containing a PyTorch state_dict. */
    template <typename T, typename Conv1DType>
    void loadConv1D(const nlohmann::json& modelJson, const std::string& layerPrefix, Conv1DType& conv, bool hasBias = true)
    {
        std::vector<std::vector<std::vector<T>>> conv_weights = modelJson.at(layerPrefix + "weight");
        detail::reverseKernels(conv_weights);
        conv.setWeights(conv_weights);

        if(hasBias)
        {
            std::vector<T> conv_bias = modelJson.at(layerPrefix + "bias");
            conv.setBias(conv_bias);
        }
        else
        {
            std::vector<T> conv_bias((size_t)conv.out_size, (T)0);
            conv.setBias(conv_bias);
        }
    }

    /**
     * Loads a GRU layer from a JSON object containing a PyTorch state_dict.
     * If your PyTorch GRU has num_layers > 1, you must call this method once
     * for each layer, with the correct layer object and layer_index.
     */
    template <typename T, typename GRUType>
    void loadGRU(const nlohmann::json& modelJson, const std::string& layerPrefix, GRUType& gru, bool hasBias = true, int layer_index = 0)
    {
        // For the kernel and recurrent weights, PyTorch stores the weights similar to the
        // Tensorflow format, but transposed, and with the "r" and "z" indexes swapped.

        const std::vector<std::vector<T>> gru_ih_weights = modelJson.at(layerPrefix + "weight_ih_l" + std::to_string(layer_index));
        auto wVals = detail::transpose(gru_ih_weights);
        detail::swap_rz(wVals, gru.out_size);
        gru.setWVals(wVals);

        const std::vector<std::vector<T>> gru_hh_weights = modelJson.at(layerPrefix + "weight_hh_l" + std::to_string(layer_index));
        auto uVals = detail::transpose(gru_hh_weights);
        detail::swap_rz(uVals, gru.out_size);
        gru.setUVals(uVals);

        // PyTorch stores the GRU bias pretty much the same as TensorFlow as well,
        // just in two separate vectors. And again, we need to swap the "r" and "z" parts.

        if(hasBias)
        {
            const std::vector<T> gru_ih_bias = modelJson.at(layerPrefix + "bias_ih_l" + std::to_string(layer_index));
            const std::vector<T> gru_hh_bias = modelJson.at(layerPrefix + "bias_hh_l" + std::to_string(layer_index));
            std::vector<std::vector<T>> gru_bias { gru_ih_bias, gru_hh_bias };
            detail::swap_rz(gru_bias, gru.out_size);
            gru.setBVals(gru_bias);
        }
        else
        {
            const std::vector<T> gru_ih_bias((size_t)gru.out_size * 3, (T)0);
            const std::vector<T> gru_hh_bias((size_t)gru.out_size * 3, (T)0);
            std::vector<std::vector<T>> gru_bias { gru_ih_bias, gru_hh_bias };
            gru.setBVals(gru_bias);
        }
    }

    /**
     * Loads a LSTM layer from a JSON object containing a PyTorch state_dict.
     * If your PyTorch LSTM has num_layers > 1, you must call this method once
     * for each layer, with the correct layer object and layer_index.
     */
    template <typename T, typename LSTMType>
    void loadLSTM(const nlohmann::json& modelJson, const std::string& layerPrefix, LSTMType& lstm, bool hasBias = true, int layer_index = 0)
    {
        const std::vector<std::vector<T>> lstm_weights_ih = modelJson.at(layerPrefix + "weight_ih_l" + std::to_string(layer_index));
        lstm.setWVals(detail::transpose(lstm_weights_ih));

        const std::vector<std::vector<T>> lstm_weights_hh = modelJson.at(layerPrefix + "weight_hh_l" + std::to_string(layer_index));
        lstm.setUVals(detail::transpose(lstm_weights_hh));

        if(hasBias)
        {
            std::vector<T> lstm_bias_ih = modelJson.at(layerPrefix + "bias_ih_l" + std::to_string(layer_index));
            std::vector<T> lstm_bias_hh = modelJson.at(layerPrefix + "bias_hh_l" + std::to_string(layer_index));
            for(size_t i = 0; i < lstm_bias_ih.size(); ++i)
                lstm_bias_hh[i] += lstm_bias_ih[i];
            lstm.setBVals(lstm_bias_hh);
        }
        else
        {
            std::vector<T> lstm_bias_hh((size_t)lstm.out_size * 4, (T)0);
            lstm.setBVals(lstm_bias_hh);
        }
    }
}
}
