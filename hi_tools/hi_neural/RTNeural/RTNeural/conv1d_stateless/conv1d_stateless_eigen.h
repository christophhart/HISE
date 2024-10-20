#ifndef CONV1D_STATELESS_EIGEN_H_INCLUDED
#define CONV1D_STATELESS_EIGEN_H_INCLUDED

#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include <Eigen/Dense>

namespace RTNEURAL_NAMESPACE
{
/**
 * Dynamic implementation of a 1-dimensional stateless convolution layer with no activation.
 * This implementation was designed to be used for a single frame of features, fully available at each forward call.
 * So the layer has a NO internal "state"
 *
 * @tparam T Type of the layer (float, double, int ...)
 */
template <typename T>
class Conv1DStateless : public Layer<T>
{
public:
    Conv1DStateless(int in_num_filters_in, int in_num_features_in, int in_num_filters_out, int in_kernel_size, int in_stride, bool in_valid_pad);
    Conv1DStateless(std::initializer_list<int> sizes);
    Conv1DStateless(const Conv1DStateless& other);
    Conv1DStateless& operator=(const Conv1DStateless& other);
    virtual ~Conv1DStateless() = default;

    static constexpr int computeNumFeaturesOut(int num_features_in, int kernel_size, int stride, int valid_pad)
    {
        // Based on tensorflow docs: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
        // Custom implementation of ceil since std::ceil is not constexpr.

        if(valid_pad)
        {
            float f = static_cast<float>(num_features_in - kernel_size + 1) / static_cast<float>(stride);
            int i = static_cast<int>(f);
            return f > static_cast<float>(i) ? i + 1 : i;
        }

        float f = static_cast<float>(num_features_in) / static_cast<float>(stride);
        int i = static_cast<int>(f);
        return f > static_cast<float>(i) ? i + 1 : i;
    }

    static constexpr int computePadLeft(int num_features_in, int kernel_size, int stride, bool valid_pad)
    {
        // Based on tensorflow: Based on tensorflow: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2ow: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
        return valid_pad ? 0 : std::max(num_features_in % stride == 0 ? kernel_size - stride : kernel_size - num_features_in % stride, 0) / 2;
    }

    static constexpr int computePadRight(int num_features_in, int kernel_size, int stride, bool valid_pad)
    {
        // Based on tensorflow: https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
        if(!valid_pad)
        {
            int total_pad = std::max(num_features_in % stride == 0 ? kernel_size - stride : kernel_size - num_features_in % stride, 0);
            return total_pad - total_pad / 2;
        }

        return 0;
    }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() override {};

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "conv1d_stateless"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* output) noexcept override
    {
        auto inMatrix = Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>,
            RTNeuralEigenAlignment>(input, num_filters_in, num_features_in);

        auto outMatrix = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>,
            RTNeuralEigenAlignment>(output, num_filters_out, num_features_out);

        if(valid_pad)
        {
            for(int i = 0; i < num_filters_out; i++)
                for(int j = 0; j < num_features_out; j++)
                    outMatrix(i, j) += kernelWeights[i].cwiseProduct(inMatrix.middleCols(j * stride, kernel_size)).sum();
        }
        else
        {
            for(int i = 0; i < num_filters_out; i++)
            {
                int j = 0;

                for(; j * stride < pad_left; j++)
                {
                    const int eff_kernel_size = kernel_size - pad_left + j * stride;
                    outMatrix(i, j) += kernelWeights[i].rightCols(eff_kernel_size).cwiseProduct(inMatrix.leftCols(eff_kernel_size)).sum();
                }

                for(; j * stride - pad_left + kernel_size < num_features_in; j++)
                    outMatrix(i, j) += kernelWeights[i].cwiseProduct(inMatrix.middleCols(j * stride - pad_left, kernel_size)).sum();

                for(; j * stride - pad_left + kernel_size <= num_features_in + pad_right; j++)
                {
                    const int eff_kernel_size = num_features_in - (j * stride - pad_left);
                    outMatrix(i, j) += kernelWeights[i].leftCols(eff_kernel_size).cwiseProduct(inMatrix.rightCols(eff_kernel_size)).sum();
                }
            }
        }
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[num_filters_out][num_filters_in][kernel_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& inWeights);

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size; }

    /** Returns the stride. */
    RTNEURAL_REALTIME int getStride() const noexcept { return stride; }

private:
    const int num_filters_in;
    const int num_features_in;
    const int num_filters_out;
    const int kernel_size;
    const int stride;
    const int num_features_out;
    const bool valid_pad;
    const int pad_left;
    const int pad_right;

    std::vector<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> kernelWeights;
};

//====================================================

/**
 * Static implementation of a 1-dimensional stateless convolution layer with no activation.
 * This implementation was designed to be used for a single frame of features, fully available at each forward call.
 * So the layer has a NO internal "state"
 *
 * @tparam T Type of the layer (float, double, int ...)
 * @tparam num_filters_in_t number of input filters
 * @tparam num_features_in_t number of input features
 * @tparam num_filters_out_t number of output filters
 * @tparam kernel_size_t size of the convolution kernel
 * @tparam stride_t convolution stride
 * @tparam valid_pad_t if true: pad is "valid", if false: pad is "same"
 */
template <typename T, int num_filters_in_t, int num_features_in_t, int num_filters_out_t, int kernel_size_t,
    int stride_t, bool valid_pad_t>
class Conv1DStatelessT
{
    static constexpr int num_features_out = Conv1DStateless<T>::computeNumFeaturesOut(num_features_in_t, kernel_size_t, stride_t, valid_pad_t);
    static constexpr int pad_left = Conv1DStateless<T>::computePadLeft(num_features_in_t, kernel_size_t, stride_t, valid_pad_t);
    static constexpr int pad_right = Conv1DStateless<T>::computePadRight(num_features_in_t, kernel_size_t, stride_t, valid_pad_t);

    using weights_type = Eigen::Matrix<T, num_filters_in_t, kernel_size_t>;
    using input_type = Eigen::Matrix<T, num_filters_in_t, num_features_in_t>;
    using output_type = Eigen::Matrix<T, num_filters_out_t, num_features_out>;

public:
    Conv1DStatelessT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "conv1d_stateless"; }

    /** Returns false since convolution is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Empty function, this layer has no state */
    RTNEURAL_REALTIME void reset() {};

    /** Performs forward propagation for this layer if pad is "valid". */
    template <bool isValid = valid_pad_t>
    RTNEURAL_REALTIME inline typename std::enable_if<isValid, void>::type
    forward(const input_type& inMatrix) noexcept
    {
        // perform a multichannel convolution
        for(int i = 0; i < num_filters_out_t; i++)
        {
            for(int j = 0; j < num_features_out; j++)
            {
                // TODO: manage to use middleCols<kernel_size>(j*stride)
                outs(i, j) = kernelWeights[i].cwiseProduct(inMatrix.middleCols(j * stride_t, kernel_size_t)).sum();
            }
        }
    }

    /** Performs forward propagation for this layer if pad is "same" */
    template <bool isValid = valid_pad_t>
    RTNEURAL_REALTIME inline typename std::enable_if<!isValid, void>::type
    forward(const input_type& inMatrix) noexcept
    {
        // perform a multichannel convolution
        for(int i = 0; i < num_filters_out_t; i++)
        {
            int j = 0;

            for(; j * stride_t < pad_left; j++)
            {
                const int eff_kernel_size = kernel_size_t - pad_left + j * stride_t;
                outs(i, j) = kernelWeights[i].rightCols(eff_kernel_size).cwiseProduct(inMatrix.leftCols(eff_kernel_size)).sum();
            }

            for(; j * stride_t - pad_left + kernel_size_t < num_features_in_t; j++)
                // TODO: manage to use middleCols<kernel_size>(j*stride)
                outs(i, j) = kernelWeights[i].cwiseProduct(inMatrix.middleCols(j * stride_t - pad_left, kernel_size_t)).sum();

            for(; j * stride_t - pad_left + kernel_size_t <= num_features_in_t + pad_right; j++)
            {
                const int eff_kernel_size = num_features_in_t - (j * stride_t - pad_left);
                outs(i, j) = kernelWeights[i].leftCols(eff_kernel_size).cwiseProduct(inMatrix.rightCols(eff_kernel_size)).sum();
            }
        }
    }

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[num_filters_out][num_filters_in][kernel_size]
     */
    RTNEURAL_REALTIME void setWeights(const std::vector<std::vector<std::vector<T>>>& inWeights);

    /**
     * Sets the layer weights.
     *
     * The weights vector must have size weights[kernel_size][num_filters_in][num_filters_out]
     */
    RTNEURAL_REALTIME void setWeightsTransposed(const std::vector<std::vector<std::vector<T>>>& inWeights);

    /** Returns the size of the convolution kernel. */
    RTNEURAL_REALTIME int getKernelSize() const noexcept { return kernel_size_t; }

    /** Returns the convolution dilation rate. */
    RTNEURAL_REALTIME int getStride() const noexcept { return stride_t; }

    Eigen::Map<output_type, RTNeuralEigenAlignment> outs;

private:
    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[num_filters_out_t * num_features_out];

    weights_type kernelWeights[num_filters_out_t];
};

} // RTNEURAL

#endif // CONV1D_STATELESS_EIGEN_H_INCLUDED
