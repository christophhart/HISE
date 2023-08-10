/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


	void EffectProcessor::SuspensionState::reset()
	{
		numSilentBuffers = 0;
		currentlySuspended = false;
	}

	EffectProcessor::EffectProcessor(MainController* mc, const String& uid, int numVoices): 
		Processor(mc, uid, numVoices),	
		isTailing(false)
	{
		
	}

	EffectProcessor::~EffectProcessor()
	{

		modChains.clear();
	}

	void EffectProcessor::renderAllChains(int startSample, int numSamples)
	{
		for (auto& mb : modChains)
		{
			if (!mb.getChain()->shouldBeProcessedAtAll())
			{
				mb.clear();
				continue;
			}

			mb.calculateMonophonicModulationValues(startSample, numSamples);
			mb.calculateModulationValuesForCurrentVoice(0, startSample, numSamples);

			if (mb.isAudioRateModulation())
				mb.expandVoiceValuesToAudioRate(0, startSample, numSamples);
		}
	}

	Colour EffectProcessor::getColour() const
	{
		return Colour(EFFECT_PROCESSOR_COLOUR);
	}

	void EffectProcessor::voicesKilled()
	{
		jassert(!hasTail());
	}

	bool EffectProcessor::isSuspendedOnSilence() const
	{ return false; }

	bool EffectProcessor::isCurrentlySuspended() const
	{ return false; }

	bool EffectProcessor::isTailingOff() const
	{	return isTailing; }

	void EffectProcessor::handleHiseEvent(const HiseEvent& m)
	{
		for (auto& mc : modChains)
			mc.handleHiseEvent(m);
	}

	bool EffectProcessor::isInSendContainer() const noexcept
	{ return isInSend; }

bool EffectProcessor::isSilent(AudioSampleBuffer& b, int startSample, int numSamples)
{
	float* stereo[2] = { b.getWritePointer(0, startSample), b.getWritePointer(jmin(1, b.getNumChannels()-1), startSample) };

	return ProcessData<2>(stereo, numSamples).isSilent();
}

void EffectProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	jassert(finalised);

	Processor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate >= 0.0)
	{
		auto callbackDurationMs = jmax(1.0, (double)samplesPerBlock / sampleRate * 1000.0);
		numSilentCallbacksToWait = roundToInt((double)HISE_SUSPENSION_TAIL_MS / callbackDurationMs);
	}

	isInSend = dynamic_cast<SendContainer*>(getParentProcessor(true, false)) != nullptr;

	for (auto& mc : modChains)
		mc.prepareToPlay(sampleRate, samplesPerBlock);
}

void EffectProcessor::finaliseModChains()
{
	modChains.finalise();

	for (auto& mb : modChains)
		mb.getChain()->setParentProcessor(this);

	finalised = true;
}

MasterEffectProcessor::MasterEffectProcessor(MainController* mc, const String& uid): EffectProcessor(mc, uid, 1)
{
	softBypassRamper.setValueWithoutSmoothing(1.0f);

	getMatrix().init();
	getMatrix().setOnlyEnablingAllowed(true);
	getMatrix().setNumAllowedConnections(2);
}

MasterEffectProcessor::~MasterEffectProcessor()
{}

Path MasterEffectProcessor::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData (HiBinaryData::SpecialSymbols::masterEffect, SIZE_OF_PATH (HiBinaryData::SpecialSymbols::masterEffect));

	return path;
}

ValueTree MasterEffectProcessor::exportAsValueTree() const
{
	ValueTree v = Processor::exportAsValueTree();
	v.addChild(getMatrix().exportAsValueTree(), -1, nullptr);

	return v;
}

void MasterEffectProcessor::restoreFromValueTree(const ValueTree& v)
{
	Processor::restoreFromValueTree(v);

	ValueTree r = v.getChildWithName("RoutingMatrix");

	if (r.isValid())
	{
		getMatrix().restoreFromValueTree(r);
	}
}

void MasterEffectProcessor::setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler) noexcept
{
	Processor::setBypassed(shouldBeBypassed, notifyChangeHandler);
	setSoftBypass(shouldBeBypassed, getMainController()->shouldUseSoftBypassRamps());
}

void MasterEffectProcessor::updateSoftBypass()
{
	const bool shouldBeBypassed = isBypassed();

	setSoftBypass(isBypassed(), !shouldBeBypassed);
}

bool MasterEffectProcessor::isSoftBypassed() const noexcept
{ return softBypassState == Bypassed; }

void MasterEffectProcessor::numDestinationChannelsChanged()
{

}

void MasterEffectProcessor::numSourceChannelsChanged()
{
		
}

void MasterEffectProcessor::startMonophonicVoice()
{
	for (auto& mb : modChains)
		mb.startVoice(0);
}

void MasterEffectProcessor::stopMonophonicVoice()
{
	for (auto& mb : modChains)
		mb.stopVoice(0);
}

void MasterEffectProcessor::setKillBuffer(AudioSampleBuffer& b)
{
	killBuffer = &b;
}

void MasterEffectProcessor::resetMonophonicVoice()
{
	for (auto& mb : modChains)
		mb.resetVoice(0);
}

bool MasterEffectProcessor::isCurrentlySuspended() const
{
	return masterState.currentlySuspended;
}

void MasterEffectProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0.0 && samplesPerBlock > 0)
		softBypassRamper.reset(sampleRate / (double)samplesPerBlock, 0.1);

	masterState.reset();

		
}

void MasterEffectProcessor::setEventBuffer(HiseEventBuffer* eventBufferFromSynth)
{
	eventBuffer = eventBufferFromSynth;
}

void MasterEffectProcessor::renderNextBlock(AudioSampleBuffer&, int startSample, int numSamples)
{
	jassert(isOnAir());

	renderAllChains(startSample, numSamples);
}

void MasterEffectProcessor::renderWholeBuffer(AudioSampleBuffer& buffer)
{
	if (softBypassState == Bypassed)
		return;

	auto leftChannel = getLeftSourceChannel();
	auto rightChannel = getRightSourceChannel();
	auto numAllowed = getMatrix().getNumAllowedConnections();
	auto numMax = getMatrix().getNumDestinationChannels();

	auto ok = (leftChannel != -1 && rightChannel != -1) ||
		(numAllowed != 2 && (leftChannel != -1 || rightChannel != -1));

	ok &= leftChannel < numMax &&
		rightChannel < numMax;

	if (ok)
	{
		auto isStereo = rightChannel != -1;

		float *samples[2] = { buffer.getWritePointer(leftChannel), isStereo ? buffer.getWritePointer(rightChannel) : nullptr };

		const int samplesToUse = buffer.getNumSamples();

		AudioSampleBuffer stereoBuffer(samples, isStereo ? 2 : 1, samplesToUse);

		if (softBypassState == Pending)
		{
			masterState.reset();

			jassert(stereoBuffer.getNumChannels() <= killBuffer->getNumChannels());
			jassert(stereoBuffer.getNumSamples() <= killBuffer->getNumSamples());

			int numSamples = stereoBuffer.getNumSamples();
			int numChannels = isStereo ? 2 : 1;

			float start = jmin<float>(1.0f, softBypassRamper.getCurrentValue());
			float end = jmax<float>(0.0f, softBypassRamper.getNextValue());

			float start_inv = 1.0f - start;
			float end_inv = 1.0f - end;

			// We don't want to fade to the input signal in a AUX send context
			// so in this case we'll skip this loop
			int numChannelsToFadeIn = numChannels * (int)!isInSendContainer();

			for (int i = 0; i < numChannelsToFadeIn; i++)
				killBuffer->copyFromWithRamp(i, 0, stereoBuffer.getReadPointer(i), numSamples, start_inv, end_inv);

			applyEffect(stereoBuffer, 0, samplesToUse);
			isTailing = !isSilent(stereoBuffer, 0, samplesToUse);

			stereoBuffer.applyGainRamp(0, numSamples, start, end);

			for (int i = 0; i < numChannelsToFadeIn; i++)
				stereoBuffer.addFrom(i, 0, killBuffer->getReadPointer(i), numSamples);

			if (!softBypassRamper.isSmoothing())
			{
				if (end < 0.5f)
				{
					voicesKilled();
					softBypassState = Bypassed;
				}
				else
				{
					softBypassState = Inactive;
				}
			}

			currentValues.outL = softBypassState == Bypassed ? 0.0f : stereoBuffer.getMagnitude(0, 0, samplesToUse);

			if(isStereo)
				currentValues.outR = softBypassState == Bypassed ? 0.0f : stereoBuffer.getMagnitude(1, 0, samplesToUse);
		}
		else
		{
			auto suspendAtSilence = isSuspendedOnSilence();

			if (suspendAtSilence && masterState.numSilentBuffers > numSilentCallbacksToWait)
			{
				if (isSilent(stereoBuffer, 0, samplesToUse))
				{
					if (getMatrix().anyChannelActive())
					{
						float gainValues[NUM_MAX_CHANNELS];

						memset(gainValues, 0, getMatrix().getNumSourceChannels() * sizeof(float));

						getMatrix().setGainValues(gainValues, true);
						getMatrix().setGainValues(gainValues, false);
					}

#if ENABLE_ALL_PEAK_METERS
					currentValues.outL = 0.0f;
					currentValues.outR = 0.0f;
#endif

					masterState.currentlySuspended = true;
					return;
				}
						
			}

			masterState.currentlySuspended = false;
			applyEffect(stereoBuffer, 0, samplesToUse);

			if (suspendAtSilence)
			{
				isTailing = !isSilent(stereoBuffer, 0, samplesToUse);

				if (!isTailing)
					masterState.numSilentBuffers++;
				else
					masterState.numSilentBuffers = 0;
			}
			else
			{
				isTailing = hasTail() && !isSilent(stereoBuffer, 0, samplesToUse);
				masterState.numSilentBuffers = 0;
			}

				

#if ENABLE_ALL_PEAK_METERS
			currentValues.outL = stereoBuffer.getMagnitude(0, 0, samplesToUse);

			if(isStereo)
				currentValues.outR = stereoBuffer.getMagnitude(1, 0, samplesToUse);
#endif
		}

		if (getMatrix().anyChannelActive())
		{
			float gainValues[NUM_MAX_CHANNELS];

			jassert(getMatrix().getNumSourceChannels() == buffer.getNumChannels());

			for (int i = 0; i < buffer.getNumChannels(); i++)
			{
				if (getMatrix().isEditorShown(i))
					gainValues[i] = buffer.getMagnitude(i, 0, samplesToUse);
				else
					gainValues[i] = 0.0f;
			}

			getMatrix().setGainValues(gainValues, true);
			getMatrix().setGainValues(gainValues, false);
		}
	}

		
}

MonophonicEffectProcessor::MonophonicEffectProcessor(MainController* mc, const String& uid): EffectProcessor(mc, uid, 1)
{}

MonophonicEffectProcessor::~MonophonicEffectProcessor()
{}

Path MonophonicEffectProcessor::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath, sizeof (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath));

	return path;
}

void MonophonicEffectProcessor::startMonophonicVoice(const HiseEvent& e)
{
	ignoreUnused(e);

	for (auto& mb : modChains)
		mb.startVoice(0);
}

void MonophonicEffectProcessor::stopMonophonicVoice()
{
	for (auto& mb : modChains)
		mb.stopVoice(0);
}

void MonophonicEffectProcessor::resetMonophonicVoice()
{
	for (auto& mb : modChains)
		mb.resetVoice(0);
}

void MonophonicEffectProcessor::renderNextBlock(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	jassert(isOnAir());

	renderAllChains(startSample, numSamples);

	constexpr int stepSize = 64;

	while(numSamples >= stepSize)
	{
		applyEffect(buffer, startSample, stepSize);

		startSample += stepSize;
		numSamples  -= stepSize;
	}

	if(numSamples != 0)
	{
		applyEffect(buffer, startSample, numSamples);
	}

#if ENABLE_ALL_PEAK_METERS
	currentValues.outL = buffer.getMagnitude(0, startSample, numSamples);
	currentValues.outR = buffer.getMagnitude(1, startSample, numSamples);
#endif
}

VoiceEffectProcessor::VoiceEffectProcessor(MainController* mc, const String& uid, int numVoices_): 
	EffectProcessor(mc, uid, numVoices_)
{
	for (int i = 0; i < numVoices_; i++)
		polyState.add({});
}

VoiceEffectProcessor::~VoiceEffectProcessor()
{}

void VoiceEffectProcessor::preRenderCallback(int startSample, int numSamples)
{
	for (auto& mb : modChains)
		mb.calculateMonophonicModulationValues(startSample, numSamples);

	if (forceMono)
	{
		for (auto& mb : modChains)
		{
			mb.calculateModulationValuesForCurrentVoice(0, startSample, numSamples);
		}
	}
}

void VoiceEffectProcessor::preVoiceRendering(int voiceIndex, int startSample, int numSamples)
{
	for (auto& mb : modChains)
	{
		mb.calculateModulationValuesForCurrentVoice(voiceIndex, startSample, numSamples);
		if (mb.isAudioRateModulation())
			mb.expandVoiceValuesToAudioRate(voiceIndex, startSample, numSamples);
	}
}

void VoiceEffectProcessor::renderVoice(int voiceIndex, AudioSampleBuffer& b, int startSample, int numSamples)
{
	jassert(isOnAir());

	preVoiceRendering(voiceIndex, startSample, numSamples);

	constexpr int stepSize = 64;

	while(numSamples >= stepSize)
	{
		applyEffect(voiceIndex, b, startSample, stepSize);

		startSample += stepSize;
		numSamples  -= stepSize;
	}

	if(numSamples != 0)
	{
		applyEffect(voiceIndex, b, startSample, numSamples);
	}

}

bool VoiceEffectProcessor::checkPreSuspension(int voiceIndex, ProcessDataDyn& d)
{
	if (isSuspendedOnSilence())
	{
		jassert(isPositiveAndBelow(voiceIndex, polyState.size()));

		auto& s = polyState.getReference(voiceIndex);

		if (s.numSilentBuffers > numSilentCallbacksToWait)
		{
			if (d.isSilent())
			{
				s.currentlySuspended = true;
				return true;
			}
					
		}
		else
		{
			s.currentlySuspended = false;
		}
	}

	return false;
}

void VoiceEffectProcessor::checkPostSuspension(int voiceIndex, ProcessDataDyn& data)
{
	if (hasTail() || isSuspendedOnSilence())
	{
		isTailing = !data.isSilent();

		if (isTailing)
			polyState.getReference(voiceIndex).numSilentBuffers = 0;
		else
			polyState.getReference(voiceIndex).numSilentBuffers++;
	}
}

bool VoiceEffectProcessor::isCurrentlySuspended() const
{
	if (!isSuspendedOnSilence())
		return false;

		

	for (const auto& s : polyState)
	{
		if (s.playing && !s.currentlySuspended)
			return false;
	}

	return true;
}

void VoiceEffectProcessor::startVoice(int voiceIndex, const HiseEvent& e)
{
	ignoreUnused(e);

	for (auto& mb : modChains)
		mb.startVoice(voiceIndex);

	if (isSuspendedOnSilence())
	{
		auto& s = polyState.getReference(voiceIndex);
		s.playing = true;
		s.reset();
	}
}

void VoiceEffectProcessor::stopVoice(int voiceIndex)
{
	for (auto& mb : modChains)
		mb.stopVoice(voiceIndex);
}

void VoiceEffectProcessor::reset(int voiceIndex)
{
	for (auto& mb : modChains)
		mb.resetVoice(voiceIndex);

	if (isSuspendedOnSilence())
	{
		polyState.getReference(voiceIndex).playing = false;
	}
}

void VoiceEffectProcessor::handleHiseEvent(const HiseEvent& m)
{
	for (auto& mb : modChains)
		mb.handleHiseEvent(m);
}

void VoiceEffectProcessor::setForceMonoMode(bool shouldUseMonoMode)
{
	forceMono = shouldUseMonoMode;
}

juce::Path VoiceEffectProcessor::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData(ProcessorIcons::polyFX, sizeof(ProcessorIcons::polyFX));
	return path;
}

bool MasterEffectProcessor::isFadeOutPending() const noexcept
{
	return softBypassState == Pending && softBypassRamper.getTargetValue() < 0.5f;
}

void MasterEffectProcessor::setSoftBypass(bool shouldBeSoftBypassed, bool useRamp/*=true*/)
{
	masterState.reset();

	if (useRamp)
	{
		softBypassRamper.setValue(shouldBeSoftBypassed ? 0.0f : 1.0f);

		if ((shouldBeSoftBypassed && softBypassState != Bypassed) ||
			(!shouldBeSoftBypassed && softBypassState != Inactive))
		{


			softBypassState = Pending;
		}

		if (softBypassRamper.getCurrentValue() == softBypassRamper.getTargetValue())
		{
			softBypassState = shouldBeSoftBypassed ? Bypassed : Inactive;
		}
	}
	else
	{
		softBypassState = shouldBeSoftBypassed ? Bypassed : Inactive;
		softBypassRamper.setValueWithoutSmoothing(shouldBeSoftBypassed ? 0.0f : 1.0f);
	}
}

} // namespace hise
