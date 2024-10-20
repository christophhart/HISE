#ifndef BATCHNORM2DEIGEN_H_INCLUDED
#define BATCHNORM2DEIGEN_H_INCLUDED

#include "../Layer.h"
#include "../config.h"
#include <Eigen/Dense>

namespace RTNEURAL_NAMESPACE
{
/** Dynamic batch normalization layer. */
template <typename T>
class BatchNorm2DLayer final : public Layer<T>
{
public:
    BatchNorm2DLayer(int num_filters, int num_features);

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "batchnorm2d"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        auto inMat = Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>, RTNeuralEigenAlignment>(
            input, num_filters, num_features);

        auto outMat = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>, RTNeuralEigenAlignment>(
            out, num_filters, num_features);

        // TODO: Should be possible to do it in one line with .colwise() but did not manage to do it yet.
        for(int i = 0; i < num_features; i++)
        {
            outMat.col(i) = (inMat.col(i) - running_mean).cwiseProduct(multiplier) + beta;
        }
    }

    /** Sets the layer "gamma" values. */
    RTNEURAL_REALTIME void setGamma(const std::vector<T>& gammaVals);

    /** Sets the layer "beta" values. */
    RTNEURAL_REALTIME void setBeta(const std::vector<T>& betaVals);

    /** Sets the layer's trained running mean. */
    RTNEURAL_REALTIME void setRunningMean(const std::vector<T>& runningMean);

    /** Set's the layer's trained running variance. */
    RTNEURAL_REALTIME void setRunningVariance(const std::vector<T>& runningVar);

    /** Set's the layer "epsilon" value. */
    RTNEURAL_REALTIME void setEpsilon(T epsilon);

private:
    void updateMultiplier();

    const int num_filters;
    const int num_features;

    Eigen::Vector<T, Eigen::Dynamic> gamma;
    Eigen::Vector<T, Eigen::Dynamic> beta;

    Eigen::Vector<T, Eigen::Dynamic> running_mean;
    Eigen::Vector<T, Eigen::Dynamic> running_var;

    Eigen::Vector<T, Eigen::Dynamic> multiplier;

    T epsilon = (T)0;
};

/** Static batch normalization layer. */
template <typename T, int num_filters_t, int num_features_t, bool affine = true>
class BatchNorm2DT
{
public:
    static constexpr auto in_size = num_filters_t * num_features_t;
    static constexpr auto out_size = num_filters_t * num_features_t;
    static constexpr auto num_filters = num_filters_t;
    static constexpr auto num_features = num_features_t;
    static constexpr bool is_affine = affine;

    BatchNorm2DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "batchnorm2d"; }

    /** Returns false since batch-norm is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<isAffine, void>::type
    forward(const Eigen::Vector<T, in_size>& ins) noexcept
    {
        auto inMat = Eigen::Map<const Eigen::Matrix<T, num_filters_t, num_features_t>, RTNeuralEigenAlignment>(ins.data());
        auto outMat = Eigen::Map<Eigen::Matrix<T, num_filters_t, num_features_t>, RTNeuralEigenAlignment>(outs.data());

        for(int i = 0; i < num_features; i++)
        {
            outMat.col(i) = (inMat.col(i) - running_mean).cwiseProduct(multiplier) + beta;
        }
    }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<!isAffine, void>::type
    forward(const Eigen::Vector<T, in_size>& ins) noexcept
    {
        auto inMat = Eigen::Map<const Eigen::Matrix<T, num_filters_t, num_features_t>, RTNeuralEigenAlignment>(ins.data());
        auto outMat = Eigen::Map<Eigen::Matrix<T, num_filters_t, num_features_t>, RTNeuralEigenAlignment>(outs.data());

        for(int i = 0; i < num_features; i++)
        {
            outMat.col(i) = (inMat.col(i) - running_mean).cwiseProduct(multiplier);
        }
    }

    /** Sets the layer "gamma" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<isAffine, void>::type setGamma(const std::vector<T>& gammaVals);

    /** Sets the layer "gamma" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<!isAffine, void>::type setGamma(const std::vector<T>&) { }

    /** Sets the layer "beta" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<isAffine, void>::type setBeta(const std::vector<T>& betaVals);

    /** Sets the layer "beta" values. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME typename std::enable_if<!isAffine, void>::type setBeta(const std::vector<T>&) { }

    /** Sets the layer's trained running mean. */
    RTNEURAL_REALTIME void setRunningMean(const std::vector<T>& runningMean);

    /** Set's the layer's trained running variance. */
    RTNEURAL_REALTIME void setRunningVariance(const std::vector<T>& runningVar);

    /** Set's the layer "epsilon" value. */
    RTNEURAL_REALTIME void setEpsilon(T epsilon);

    Eigen::Map<Eigen::Vector<T, out_size>, RTNeuralEigenAlignment> outs;

private:
    void updateMultiplier();

    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    Eigen::Vector<T, num_filters_t> gamma;
    Eigen::Vector<T, num_filters_t> beta;

    Eigen::Vector<T, num_filters_t> running_mean;
    Eigen::Vector<T, num_filters_t> running_var;

    Eigen::Vector<T, num_filters_t> multiplier;

    T epsilon = (T)0;
};
}

#endif // BATCHNORM2DEIGEN_H_INCLUDED
