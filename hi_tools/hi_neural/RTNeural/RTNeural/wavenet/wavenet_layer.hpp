#pragma once



namespace wavenet
{
template <typename T,
          int condition_size,
          int channels,
          int kernel_size,
          int dilation,
          typename MathsProvider,
          typename Activation = RTNeural::TanhActivationT<T, channels, MathsProvider>>
// TODO: gated?
struct Wavenet_Layer
{
    RTNeural::Conv1DT<T, channels, channels, kernel_size, dilation> conv;
    RTNeural::DenseT<T, condition_size, channels, false> input_mixin;
    RTNeural::DenseT<T, channels, channels> _1x1;
    Activation activation;

#if RTNEURAL_USE_EIGEN
    Eigen::Matrix<T, channels, 1> outs;
#elif RTNEURAL_USE_XSIMD
    xsimd::batch<T> outs[RTNeural::ceil_div (channels, (int) xsimd::batch<T>::size)];
#endif

    void reset()
    {
        conv.reset();
    }

    void load_weights (std::vector<float>::iterator& weights)
    {
        conv.reset();

        std::vector<std::vector<std::vector<float>>> conv_weights (channels, std::vector<std::vector<float>> (channels, std::vector<float> (kernel_size)));
        for (int i = 0; i < channels; ++i)
            for (int j = 0; j < channels; ++j)
                for (int k = 0; k < kernel_size; k++)
                    conv_weights[i][j][k] = *(weights++);
        RTNeural::torch_helpers::detail::reverseKernels (conv_weights);
        conv.setWeights (conv_weights);

        std::vector<float> conv_bias (channels);
        for (int i = 0; i < channels; ++i)
            conv_bias[i] = *(weights++);
        conv.setBias (conv_bias);

        std::vector<std::vector<float>> input_mixin_weights (channels, std::vector<float> (condition_size));
        for (int i = 0; i < channels; i++)
            for (int j = 0; j < condition_size; j++)
                input_mixin_weights[i][j] = *(weights++);
        input_mixin.setWeights (input_mixin_weights);

        std::vector<std::vector<float>> _1x1_weights (channels, std::vector<float> (channels));
        for (int i = 0; i < channels; i++)
            for (int j = 0; j < channels; j++)
                _1x1_weights[i][j] = *(weights++);
        _1x1.setWeights (_1x1_weights);

        std::vector<float> _1x1_bias (channels);
        for (int i = 0; i < channels; i++)
            _1x1_bias[i] = *(weights++);
        _1x1.setBias (_1x1_bias.data());
    }

#if RTNEURAL_USE_EIGEN
    void forward (const Eigen::Matrix<T, channels, 1>& ins,
                  const Eigen::Matrix<T, condition_size, 1>& condition,
                  Eigen::Map<Eigen::Matrix<T, channels, 1>, RTNeural::RTNeuralEigenAlignment>& head_io)
#elif RTNEURAL_USE_XSIMD
    void forward (const xsimd::batch<T> (&ins)[RTNeural::ceil_div (channels, (int) xsimd::batch<T>::size)],
                  const xsimd::batch<T> (&condition)[RTNeural::ceil_div (condition_size, (int) xsimd::batch<T>::size)],
                  xsimd::batch<T> (&head_io)[RTNeural::ceil_div (channels, (int) xsimd::batch<T>::size)])
#endif
    {
        conv.forward (ins);
        input_mixin.forward (condition);

#if RTNEURAL_USE_EIGEN
        outs = conv.outs + input_mixin.outs;
#elif RTNEURAL_USE_XSIMD
        for (int i = 0; i < std::size (outs); ++i)
            outs[i] = conv.outs[i] + input_mixin.outs[i];
#endif

        activation.forward (outs);

#if RTNEURAL_USE_EIGEN
        head_io.noalias() += activation.outs;
#elif RTNEURAL_USE_XSIMD
        for (int i = 0; i < std::size (head_io); ++i)
            head_io[i] += activation.outs[i];
#endif

        _1x1.forward (activation.outs);

#if RTNEURAL_USE_EIGEN
        outs = ins + _1x1.outs;
#elif RTNEURAL_USE_XSIMD
        for (int i = 0; i < std::size (outs); ++i)
            outs[i] = ins[i] + _1x1.outs[i];
#endif
    }
};
} // namespace wavenet
