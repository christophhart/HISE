#ifndef GRUEIGEN_H_INCLUDED
#define GRUEIGEN_H_INCLUDED

#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include "../maths/maths_eigen.h"

namespace RTNEURAL_NAMESPACE
{

/**
 * Dynamic implementation of a gated recurrent unit (GRU) layer
 * with tanh activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class GRULayer : public Layer<T>
{
public:
    /** Constructs a GRU layer for a given input and output size. */
    GRULayer(int in_size, int out_size);
    GRULayer(std::initializer_list<int> sizes);
    GRULayer(const GRULayer& other);
    GRULayer& operator=(const GRULayer& other);
    virtual ~GRULayer() = default;

    /** Resets the state of the GRU. */
    RTNEURAL_REALTIME void reset() override
    {
        extendedHt1.setZero();
        extendedHt1(Layer<T>::out_size) = (T)1;
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "gru"; }

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* h) noexcept override
    {
        for(int i = 0; i < Layer<T>::in_size; ++i)
        {
            extendedInVec(i) = input[i];
        }

        /**
         *         | Wz bz[0] |   | input |   | Wz * input + bz[0] |
         * alpha = | Wr br[0] | * | 1     | = | Wr * input + br[0] |
         *         | Wc bc[0] |               | Wc * input + bc[0] |
         *
         *        | Uz bz[1] |   | h(t-1) |   | Uz * h(t-1) + bz[1] |
         * beta = | Ur br[1] | * | 1      | = | Ur * h(t-1) + br[1] |
         *        | Uc bc[1] |                | Uc * h(t-1) + bc[1] |
         */
        alphaVec.noalias() = wCombinedWeights * extendedInVec;
        betaVec.noalias() = uCombinedWeights * extendedHt1;

        /**
         * gamma = sigmoid( | z |   = sigmoid(alpha[0 : 2*out_sizet] + beta[0 : 2*out_sizet])
         *                  | r | )
         */
        gammaVec.noalias() = alphaVec.segment(0, 2 * Layer<T>::out_size) + betaVec.segment(0, 2 * Layer<T>::out_size);
        gammaVec = MathsProvider::sigmoid(gammaVec);

        /**
         * c = tanh( alpha[2*out_sizet : 3*out_sizet] + r.cwiseProduct(beta[2*out_sizet : 3*out_sizet] )
         * i.e. c = tanh( Wc * input + bc[0] + r.cwiseProduct(Uc * h(t-1) + bc[1]) )
         */
        cVec.noalias() = alphaVec.segment(2 * Layer<T>::out_size, Layer<T>::out_size) + gammaVec.segment(Layer<T>::out_size, Layer<T>::out_size).cwiseProduct(betaVec.segment(2 * Layer<T>::out_size, Layer<T>::out_size));
        cVec = MathsProvider::tanh(cVec);

        /**
         * h(t-1) = (1 - z).cwiseProduct(c) + z.cwiseProduct(h(t-1))
         *        = c - z.cwiseProduct(c) + z.cwiseProduct(ht(t-1))
         *        = c + z.cwiseProduct(h(t-1) - c)
         */
        extendedHt1.segment(0, Layer<T>::out_size) = cVec + gammaVec.segment(0, Layer<T>::out_size).cwiseProduct(extendedHt1.segment(0, Layer<T>::out_size) - cVec);

        for(int i = 0; i < Layer<T>::out_size; ++i)
        {
            h[i] = extendedHt1(i);
        }
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(T** wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(T** uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[2][3 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(T** bVals);

    /** Returns the kernel weight for the given indices. */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /** Returns the recurrent weight for the given indices. */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /** Returns the bias value for the given indices. */
    RTNEURAL_REALTIME void setBVals(const std::vector<std::vector<T>>& bVals);

    RTNEURAL_REALTIME T getWVal(int i, int k) const noexcept;
    RTNEURAL_REALTIME T getUVal(int i, int k) const noexcept;
    RTNEURAL_REALTIME T getBVal(int i, int k) const noexcept;

private:
    // Kernels
    // | Wz bz0 |
    // | Wr br0 |
    // | Wc bc0 |
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> wCombinedWeights;

    // | Uz bz1 |
    // | Ur br1 |
    // | Uc bc1 |
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> uCombinedWeights;

    // Input vec
    Eigen::Matrix<T, Eigen::Dynamic, 1> extendedInVec;

    // h(t-1) vec
    Eigen::Matrix<T, Eigen::Dynamic, 1> extendedHt1;

    // Scratch memory
    Eigen::Matrix<T, Eigen::Dynamic, 1> alphaVec;
    Eigen::Matrix<T, Eigen::Dynamic, 1> betaVec;
    Eigen::Matrix<T, Eigen::Dynamic, 1> gammaVec;
    Eigen::Matrix<T, Eigen::Dynamic, 1> cVec;
};

//====================================================
/**
 * Static implementation of a gated recurrent unit (GRU) layer
 * with tanh activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, int in_sizet, int out_sizet,
    SampleRateCorrectionMode sampleRateCorr = SampleRateCorrectionMode::None,
    typename MathsProvider = DefaultMathsProvider>
class GRULayerT
{
    using in_type = Eigen::Matrix<T, in_sizet, 1>;
    using extended_in_type = Eigen::Matrix<T, in_sizet + 1, 1>;
    using out_type = Eigen::Matrix<T, out_sizet, 1>;
    using extended_out_type = Eigen::Matrix<T, out_sizet + 1, 1>;

    using w_k_type = Eigen::Matrix<T, out_sizet * 3, in_sizet + 1>;
    using u_k_type = Eigen::Matrix<T, out_sizet * 3, out_sizet + 1>;

    using three_out_type = Eigen::Matrix<T, out_sizet * 3, 1>;
    using two_out_type = Eigen::Matrix<T, out_sizet * 2, 1>;

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    GRULayerT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "gru"; }

    /** Returns false since GRU is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Prepares the GRU to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    prepare(int delaySamples);

    /** Prepares the GRU to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    prepare(T delaySamples);

    /** Resets the state of the GRU. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const in_type& ins) noexcept
    {
        for(int i = 0; i < in_sizet; ++i)
        {
            extendedInVec(i) = ins(i);
        }

        /**
         *         | Wz bz[0] |   | input |   | Wz * input + bz[0] |
         * alpha = | Wr br[0] | * | 1     | = | Wr * input + br[0] |
         *         | Wc bc[0] |               | Wc * input + bc[0] |
         *
         *        | Uz bz[1] |   | h(t-1) |   | Uz * h(t-1) + bz[1] |
         * beta = | Ur br[1] | * | 1      | = | Ur * h(t-1) + br[1] |
         *        | Uc bc[1] |                | Uc * h(t-1) + bc[1] |
         */
        alphaVec.noalias() = wCombinedWeights * extendedInVec;
        betaVec.noalias() = uCombinedWeights * extendedHt1;

        /**
         * gamma = sigmoid( | z |   = sigmoid(alpha[0 : 2*out_sizet] + beta[0 : 2*out_sizet])
         *                  | r | )
         */
        gammaVec = MathsProvider::sigmoid(alphaVec.segment(0, 2 * out_sizet) + betaVec.segment(0, 2 * out_sizet));

        /**
         * c = tanh( alpha[2*out_sizet : 3*out_sizet] + r.cwiseProduct(beta[2*out_sizet : 3*out_sizet] )
         * i.e. c = tanh( Wc * input + bc[0] + r.cwiseProduct(Uc * h(t-1) + bc[1]) )
         */
        cVec.noalias() = alphaVec.segment(2 * out_sizet, out_sizet) + gammaVec.segment(out_sizet, out_sizet).cwiseProduct(betaVec.segment(2 * out_sizet, out_sizet));
        cVec = MathsProvider::tanh(cVec);

        /**
         * h(t-1) = (1 - z).cwiseProduct(c) + z.cwiseProduct(h(t-1))
         *        = c - z.cwiseProduct(c) + z.cwiseProduct(ht(t-1))
         *        = c + z.cwiseProduct(h(t-1) - c)
         */
        extendedHt1.segment(0, out_sizet) = cVec + gammaVec.segment(0, out_sizet).cwiseProduct(extendedHt1.segment(0, out_sizet) - cVec);

        computeOutput();
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][3 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[2][3 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<std::vector<T>>& bVals);

    Eigen::Map<out_type, RTNeuralEigenAlignment> outs;

private:
    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::None, void>
    computeOutput() noexcept
    {
        for(int i = 0; i < out_sizet; ++i)
        {
            outs(i) = extendedHt1(i);
        }
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr != SampleRateCorrectionMode::None, void>
    computeOutput() noexcept
    {
        for(int i = 0; i < out_sizet; ++i)
        {
            outs_delayed[delayWriteIdx][i] = extendedHt1(i);
        }

        processDelay(outs_delayed, outs, delayWriteIdx);

        for(int i = 0; i < out_sizet; ++i)
        {
            extendedHt1(i) = outs(i);
        }
    }

    template <typename OutVec, SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    processDelay(std::vector<out_type>& delayVec, OutVec& out, int delayWriteIndex) noexcept
    {
        out = delayVec[0];

        for(int j = 0; j < delayWriteIndex; ++j)
            delayVec[j] = delayVec[j + 1];
    }

    template <typename OutVec, SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    processDelay(std::vector<out_type>& delayVec, OutVec& out, int delayWriteIndex) noexcept
    {
        out = delayPlus1Mult * delayVec[0] + delayMult * delayVec[1];

        for(int j = 0; j < delayWriteIndex; ++j)
            delayVec[j] = delayVec[j + 1];
    }

    // kernel weights
    w_k_type wCombinedWeights;
    u_k_type uCombinedWeights;

    // scratch memory
    three_out_type alphaVec;
    three_out_type betaVec;
    two_out_type gammaVec;

    // input, output, memory
    out_type cVec;
    extended_in_type extendedInVec;
    extended_out_type extendedHt1;

    // needed for delays when doing sample rate correction
    std::vector<out_type> outs_delayed;
    int delayWriteIdx = 0;
    T delayMult = (T)1;
    T delayPlus1Mult = (T)0;
};

} // namespace RTNEURAL_NAMESPACE

#endif // GRUEIGEN_H_INCLUDED
