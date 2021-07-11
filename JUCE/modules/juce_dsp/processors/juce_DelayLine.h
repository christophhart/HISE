/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{
namespace dsp
{

//==============================================================================
/**
    A collection of structs to pass as the template argument when setting the
    interpolation type for the DelayLine class.
*/
namespace DelayLineInterpolationTypes
{
    /**
        No interpolation between successive samples in the delay line will be
        performed. This is useful when the delay is a constant integer or to
        create lo-fi audio effects.

        @tags{DSP}
    */
    struct None {};

    /**
        Successive samples in the delay line will be linearly interpolated. This
        type of interpolation has a low compuational cost where the delay can be
        modulated in real time, but it also introduces a low-pass filtering effect
        into your audio signal.

        @tags{DSP}
    */
    struct Linear {};

    /**
        Successive samples in the delay line will be interpolated using a 3rd order
        Lagrange interpolator. This method incurs more computational overhead than
        linear interpolation but reduces the low-pass filtering effect whilst
        remaining amenable to real time delay modulation.

        @tags{DSP}
    */
    struct Lagrange3rd {};

    /**
        Successive samples in the delay line will be interpolated using 1st order
        Thiran interpolation. This method is very efficient, and features a flat
        amplitude frequency response in exchange for less accuracy in the phase
        response. This interpolation method is stateful so is unsuitable for
        applications requiring fast delay modulation.

        @tags{DSP}
    */
    struct Thiran {};
}

//==============================================================================
/**
    A delay line processor featuring several algorithms for the fractional delay
    calculation, block processing, and sample-by-sample processing useful when
    modulating the delay in real time or creating a standard delay effect with
    feedback.

    Note: If you intend to change the delay in real time, you may want to smooth
    changes to the delay systematically using either a ramp or a low-pass filter.

    @see SmoothedValue, FirstOrderTPTFilter

    @tags{DSP}
*/
template <typename SampleType, typename InterpolationType = DelayLineInterpolationTypes::Linear>
class DelayLine
{
public:
    //==============================================================================
    /** Default constructor. */
    DelayLine();

    /** Constructor. */
    explicit DelayLine (int maximumDelayInSamples);

    //==============================================================================
    /** Sets the delay in samples. */
    void setDelay (SampleType newDelayInSamples);

    /** Returns the current delay in samples. */
    SampleType getDelay() const;

	/** Sets the upper limit for the delay time (in samples). */
	void setMaxDelaySamples(int newMaxSize);

    //==============================================================================
    /** Initialises the processor. */
    void prepare (const ProcessSpec& spec);

    /** Resets the internal state variables of the processor. */
    void reset();

    //==============================================================================
    /** Pushes a single sample into one channel of the delay line.

        Use this function and popSample instead of process if you need to modulate
        the delay in real time instead of using a fixed delay value, or if you want
        to code a delay effect with a feedback loop.

        @see setDelay, popSample, process
    */
    void pushSample (int channel, SampleType sample);

    /** Pops a single sample from one channel of the delay line.

        Use this function to modulate the delay in real time or implement standard
        delay effects with feedback.

        @param channel              the target channel for the delay line.

        @param delayInSamples       sets the wanted fractional delay in samples, or -1
                                    to use the value being used before or set with
                                    setDelay function.

        @param updateReadPointer    should be set to true if you use the function
                                    once for each sample, or false if you need
                                    multi-tap delay capabilities.

        @see setDelay, pushSample, process
    */
    SampleType popSample (int channel, SampleType delayInSamples = -1, bool updateReadPointer = true);

    //==============================================================================
    /** Processes the input and output samples supplied in the processing context.

        Can be used for block processing when the delay is not going to change
        during processing. The delay must first be set by calling setDelay.

        @see setDelay
    */
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumChannels() == writePos.size());
        jassert (inputBlock.getNumSamples()  == numSamples);

        if (context.isBypassed)
        {
            outputBlock.copyFrom (inputBlock);
            return;
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* inputSamples = inputBlock.getChannelPointer (channel);
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i)
            {
                pushSample ((int) channel, inputSamples[i]);
                outputSamples[i] = popSample ((int) channel);
            }
        }
    }

private:
    //==============================================================================
    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::None>::value, SampleType>::type
    interpolateSample (int channel) const
    {
        auto index = (readPos[(size_t) channel] + delayInt) % totalSize;
        return bufferData.getSample (channel, index);
    }

    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::Linear>::value, SampleType>::type
    interpolateSample (int channel) const
    {
        auto index1 = readPos[(size_t) channel] + delayInt;
        auto index2 = index1 + 1;

        if (index2 >= totalSize)
        {
            index1 %= totalSize;
            index2 %= totalSize;
        }

        auto value1 = bufferData.getSample (channel, index1);
        auto value2 = bufferData.getSample (channel, index2);

        return value1 + delayFrac * (value2 - value1);
    }

    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::Lagrange3rd>::value, SampleType>::type
    interpolateSample (int channel) const
    {
        auto index1 = readPos[(size_t) channel] + delayInt;
        auto index2 = index1 + 1;
        auto index3 = index2 + 1;
        auto index4 = index3 + 1;

        if (index4 >= totalSize)
        {
            index1 %= totalSize;
            index2 %= totalSize;
            index3 %= totalSize;
            index4 %= totalSize;
        }

        auto* samples = bufferData.getReadPointer (channel);

        auto value1 = samples[index1];
        auto value2 = samples[index2];
        auto value3 = samples[index3];
        auto value4 = samples[index4];

        auto d1 = delayFrac - 1.f;
        auto d2 = delayFrac - 2.f;
        auto d3 = delayFrac - 3.f;

        auto c1 = -d1 * d2 * d3 / 6.f;
        auto c2 = d2 * d3 * 0.5f;
        auto c3 = -d1 * d3 * 0.5f;
        auto c4 = d1 * d2 / 6.f;

        return value1 * c1 + delayFrac * (value2 * c2 + value3 * c3 + value4 * c4);
    }

    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::Thiran>::value, SampleType>::type
    interpolateSample (int channel)
    {
        auto index1 = readPos[(size_t) channel] + delayInt;
        auto index2 = index1 + 1;

        if (index2 >= totalSize)
        {
            index1 %= totalSize;
            index2 %= totalSize;
        }

        auto value1 = bufferData.getSample (channel, index1);
        auto value2 = bufferData.getSample (channel, index2);

        auto output = delayFrac == 0 ? value1 : value2 + alpha * (value1 - v[(size_t) channel]);
        v[(size_t) channel] = output;

        return output;
    }

    //==============================================================================
    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::None>::value, void>::type
    updateInternalVariables()
    {
    }

    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::Linear>::value, void>::type
    updateInternalVariables()
    {
    }

    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::Lagrange3rd>::value, void>::type
    updateInternalVariables()
    {
        if (delayInt >= 1)
        {
            delayFrac++;
            delayInt--;
        }
    }

    template <typename T = InterpolationType>
    typename std::enable_if <std::is_same <T, DelayLineInterpolationTypes::Thiran>::value, void>::type
    updateInternalVariables()
    {
        if (delayFrac < (SampleType) 0.618 && delayInt >= 1)
        {
            delayFrac++;
            delayInt--;
        }

        alpha = (1 - delayFrac) / (1 + delayFrac);
    }

    //==============================================================================
    double sampleRate;

	ProcessSpec lastSpecs;

    //==============================================================================
    AudioBuffer<SampleType> bufferData;
    std::vector<SampleType> v;
    std::vector<int> writePos, readPos;
    SampleType delay = 0.0, delayFrac = 0.0;
    int delayInt = 0, totalSize = 4;
    SampleType alpha = 0.0;
};



} // namespace dsp
} // namespace juce
