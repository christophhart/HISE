#ifndef LSTM_EIGEN_INCLUDED
#define LSTM_EIGEN_INCLUDED

#include "../Layer.h"
#include "../common.h"
#include "../config.h"
#include "../maths/maths_eigen.h"

namespace RTNEURAL_NAMESPACE
{

/**
 * Dynamic implementation of a LSTM layer with tanh
 * activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class LSTMLayer : public Layer<T>
{
public:
    /** Constructs a LSTM layer for a given input and output size. */
    LSTMLayer(int in_size, int out_size);
    LSTMLayer(std::initializer_list<int> sizes);
    LSTMLayer(const LSTMLayer& other);
    LSTMLayer& operator=(const LSTMLayer& other);
    virtual ~LSTMLayer() = default;

    /** Returns the name of this layer. */
    std::string getName() const noexcept override { return "lstm"; }

    /** Resets the state of the LSTM. */
    RTNEURAL_REALTIME void reset() override;

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* h) noexcept override
    {
        for(int i = 0; i < Layer<T>::in_size; ++i)
        {
            extendedInVecHt1(i) = input[i];
        }

        /**
         * | f  |   | Wf  Uf  Bf  |   | input |
         * | i  | = | Wi  Ui  Bi  | * | ht1   |
         * | o  |   | Wo  Uo  Bo  |   | 1     |
         * | ct |   | Wct Uct Bct |
         */
        fioctVecs.noalias() = combinedWeights * extendedInVecHt1;

        fioVecs = fioctVecs.segment(0, Layer<T>::out_size * 3);
        ctVec = MathsProvider::tanh(fioctVecs.segment(Layer<T>::out_size * 3, Layer<T>::out_size));

        fioVecs = MathsProvider::sigmoid(fioVecs);

        ct1 = fioVecs.segment(0, Layer<T>::out_size).cwiseProduct(ct1) + fioVecs.segment(Layer<T>::out_size, Layer<T>::out_size).cwiseProduct(ctVec);
        cTanhVec = MathsProvider::tanh(ct1);

        ht1 = fioVecs.segment(Layer<T>::out_size * 2, Layer<T>::out_size).cwiseProduct(cTanhVec);

        for(int i = 0; i < Layer<T>::out_size; ++i)
        {
            h[i] = extendedInVecHt1(Layer<T>::in_size + i) = ht1(i);
        }
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[4 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<T>& bVals);

private:
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> combinedWeights;

    Eigen::Matrix<T, Eigen::Dynamic, 1> extendedInVecHt1;

    Eigen::Matrix<T, Eigen::Dynamic, 1> fioctVecs;
    Eigen::Matrix<T, Eigen::Dynamic, 1> fioVecs;
    Eigen::Matrix<T, Eigen::Dynamic, 1> ctVec;

    Eigen::Matrix<T, Eigen::Dynamic, 1> cTanhVec;

    Eigen::Matrix<T, Eigen::Dynamic, 1> ht1;
    Eigen::Matrix<T, Eigen::Dynamic, 1> ct1;
};

//====================================================
/**
 * Static implementation of a LSTM layer with tanh
 * activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, int in_sizet, int out_sizet,
    SampleRateCorrectionMode sampleRateCorr = SampleRateCorrectionMode::None,
    typename MathsProvider = DefaultMathsProvider>
class LSTMLayerT
{
    using weights_combined_type = Eigen::Matrix<T, 4 * out_sizet, in_sizet + out_sizet + 1>;
    using extended_in_out_type = Eigen::Matrix<T, in_sizet + out_sizet + 1, 1>;
    using four_out_type = Eigen::Matrix<T, 4 * out_sizet, 1>;
    using three_out_type = Eigen::Matrix<T, 3 * out_sizet, 1>;

    using in_type = Eigen::Matrix<T, in_sizet, 1>;
    using out_type = Eigen::Matrix<T, out_sizet, 1>;

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    LSTMLayerT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "lstm"; }

    /** Returns false since LSTM is not an activation. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Prepares the LSTM to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    prepare(int delaySamples);

    /** Prepares the LSTM to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    prepare(T delaySamples);

    /** Resets the state of the LSTM. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const in_type& ins) noexcept
    {
        for(int i = 0; i < in_sizet; ++i)
        {
            extendedInHt1Vec(i) = ins(i);
        }

        /**
         * | f  |   | Wf  Uf  Bf  |   | input |
         * | i  | = | Wi  Ui  Bi  | * | ht1   |
         * | o  |   | Wo  Uo  Bo  |   | 1     |
         * | ct |   | Wct Uct Bct |
         */
        fioctsVecs.noalias() = combinedWeights * extendedInHt1Vec;

        fioVecs = MathsProvider::sigmoid(fioctsVecs.segment(0, 3 * out_sizet));
        ctVec = MathsProvider::tanh(fioctsVecs.segment(3 * out_sizet, out_sizet));

        computeOutputs();
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[4 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<T>& bVals);

    Eigen::Map<out_type, RTNeuralEigenAlignment> outs;

private:
    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::None, void>
    computeOutputs() noexcept
    {
        computeOutputsInternal(cVec, outs);

        for(int i = 0; i < out_sizet; ++i)
        {
            extendedInHt1Vec(in_sizet + i) = outs(i);
        }
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr != SampleRateCorrectionMode::None, void>
    computeOutputs() noexcept
    {
        computeOutputsInternal(ct_delayed[delayWriteIdx], outs_delayed[delayWriteIdx]);

        processDelay(ct_delayed, cVec, delayWriteIdx);
        processDelay(outs_delayed, outs, delayWriteIdx);

        for(int i = 0; i < out_sizet; ++i)
        {
            extendedInHt1Vec(in_sizet + i) = outs(i);
        }
    }

    template <typename VecType1, typename VecType2>
    inline void computeOutputsInternal(VecType1& cVecLocal, VecType2& outsVec) noexcept
    {
        cVecLocal.noalias()
            = fioVecs.segment(0, out_sizet)
                  .cwiseProduct(cVec)
            + fioVecs.segment(out_sizet, out_sizet)
                  .cwiseProduct(ctVec);

        cTanhVec = MathsProvider::tanh(cVecLocal);
        outsVec.noalias() = fioVecs.segment(out_sizet * 2, out_sizet).cwiseProduct(cTanhVec);
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
    weights_combined_type combinedWeights;
    extended_in_out_type extendedInHt1Vec;
    four_out_type fioctsVecs;
    three_out_type fioVecs;
    out_type cTanhVec;

    // intermediate values
    out_type ctVec;
    out_type cVec;

    // needed for delays when doing sample rate correction
    std::vector<out_type> ct_delayed;
    std::vector<out_type> outs_delayed;
    int delayWriteIdx = 0;
    T delayMult = (T)1;
    T delayPlus1Mult = (T)0;
};

} // namespace RTNEURAL_NAMESPACE

#endif // LSTM_EIGEN_INCLUDED
