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
template <typename SampleType, typename InterpolationType>
DelayLine<SampleType, InterpolationType>::DelayLine()
    : DelayLine (0)
{
}

template <typename SampleType, typename InterpolationType>
DelayLine<SampleType, InterpolationType>::DelayLine (int maximumDelayInSamples)
{
    jassert (maximumDelayInSamples >= 0);

    totalSize = jmax (4, maximumDelayInSamples + 1);
    sampleRate = 44100.0;
}

template <typename SampleType, typename InterpolationType /*= DelayLineInterpolationTypes::Linear*/>
void juce::dsp::DelayLine<SampleType, InterpolationType>::setMaxDelaySamples(int newMaxSize)
{
	totalSize = jmax<int>(4, newMaxSize + 1);
	
	if(lastSpecs.numChannels > 0)
		prepare(lastSpecs);

	updateInternalVariables();
}

//==============================================================================
template <typename SampleType, typename InterpolationType>
void DelayLine<SampleType, InterpolationType>::setDelay (SampleType newDelayInSamples)
{
    auto upperLimit = (SampleType) (totalSize - 1);

    delay     = jlimit ((SampleType) 0, upperLimit, newDelayInSamples);
    delayInt  = static_cast<int> (std::floor (delay));
    delayFrac = delay - (SampleType) delayInt;

    updateInternalVariables();
}

template <typename SampleType, typename InterpolationType>
SampleType DelayLine<SampleType, InterpolationType>::getDelay() const
{
    return delay;
}

//==============================================================================
template <typename SampleType, typename InterpolationType>
void DelayLine<SampleType, InterpolationType>::prepare (const ProcessSpec& spec)
{
	lastSpecs = spec;

    jassert (spec.numChannels > 0);

    bufferData.setSize ((int) spec.numChannels, totalSize, false, false, true);

    writePos.resize (spec.numChannels);
    readPos.resize  (spec.numChannels);

    v.resize (spec.numChannels);
    sampleRate = spec.sampleRate;

    reset();
}

template <typename SampleType, typename InterpolationType>
void DelayLine<SampleType, InterpolationType>::reset()
{
    for (auto vec : { &writePos, &readPos })
        std::fill (vec->begin(), vec->end(), 0);

    std::fill (v.begin(), v.end(), static_cast<SampleType> (0));

    bufferData.clear();
}

//==============================================================================
template <typename SampleType, typename InterpolationType>
void DelayLine<SampleType, InterpolationType>::pushSample (int channel, SampleType sample)
{
    bufferData.setSample (channel, writePos[(size_t) channel], sample);
    writePos[(size_t) channel] = (writePos[(size_t) channel] + totalSize - 1) % totalSize;
}

template <typename SampleType, typename InterpolationType>
SampleType DelayLine<SampleType, InterpolationType>::popSample (int channel, SampleType delayInSamples, bool updateReadPointer)
{
    if (delayInSamples >= 0)
        setDelay(delayInSamples);

    auto result = interpolateSample (channel);

    if (updateReadPointer)
        readPos[(size_t) channel] = (readPos[(size_t) channel] + totalSize - 1) % totalSize;

    return result;
}

//==============================================================================
template class DelayLine<float,  DelayLineInterpolationTypes::None>;
template class DelayLine<double, DelayLineInterpolationTypes::None>;
template class DelayLine<float,  DelayLineInterpolationTypes::Linear>;
template class DelayLine<double, DelayLineInterpolationTypes::Linear>;
template class DelayLine<float,  DelayLineInterpolationTypes::Lagrange3rd>;
template class DelayLine<double, DelayLineInterpolationTypes::Lagrange3rd>;
template class DelayLine<float,  DelayLineInterpolationTypes::Thiran>;
template class DelayLine<double, DelayLineInterpolationTypes::Thiran>;

} // namespace dsp
} // namespace juce
