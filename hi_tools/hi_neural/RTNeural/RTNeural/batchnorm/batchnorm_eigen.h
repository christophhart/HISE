#ifndef BATCHNORMEIGEN_H_INCLUDED
#define BATCHNORMEIGEN_H_INCLUDED

#include "../Layer.h"
#include "../config.h"
#include <Eigen/Dense>

namespace RTNEURAL_NAMESPACE
{
/** Dynamic batch normalization layer. */
template <typename T>
class BatchNorm1DLayer final : public Layer<T>
{
public:
    explicit BatchNorm1DLayer(int size);

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "batchnorm"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        auto inVec = Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1>, RTNeuralEigenAlignment>(
            input, Layer<T>::in_size, 1);

        auto outVec = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>, RTNeuralEigenAlignment>(
            out, Layer<T>::in_size, 1);

        outVec = multiplier.cwiseProduct(inVec - running_mean) + beta;
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

    Eigen::Vector<T, Eigen::Dynamic> gamma;
    Eigen::Vector<T, Eigen::Dynamic> beta;

    Eigen::Vector<T, Eigen::Dynamic> running_mean;
    Eigen::Vector<T, Eigen::Dynamic> running_var;

    Eigen::Vector<T, Eigen::Dynamic> multiplier;

    T epsilon = (T)0;
};

/** Static batch normalization layer. */
template <typename T, int size, bool affine = true>
class BatchNorm1DT
{
public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;
    static constexpr bool is_affine = affine;

    BatchNorm1DT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "batchnorm"; }

    /** Returns false since batch-norm is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Resets the layer state. */
    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<isAffine, void>::type
    forward(const Eigen::Matrix<T, in_size, 1>& ins) noexcept
    {
        outs = multiplier.cwiseProduct(ins - running_mean) + beta;
    }

    /** Performs forward propagation for this layer. */
    template <bool isAffine = affine>
    RTNEURAL_REALTIME inline typename std::enable_if<!isAffine, void>::type
    forward(const Eigen::Matrix<T, in_size, 1>& ins) noexcept
    {
        outs = multiplier.cwiseProduct(ins - running_mean);
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

    Eigen::Vector<T, out_size> gamma;
    Eigen::Vector<T, out_size> beta;

    Eigen::Vector<T, out_size> running_mean;
    Eigen::Vector<T, out_size> running_var;

    Eigen::Vector<T, out_size> multiplier;

    T epsilon = (T)0;
};
}

#endif // BATCHNORMEIGEN_H_INCLUDED
