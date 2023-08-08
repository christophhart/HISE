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

ConvolutionEffect::ConvolutionEffect(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id),
AudioSampleProcessor(mc)
{
	getBuffer().addListener(this);

	finaliseModChains();

	parameterNames.add("DryGain");
	parameterNames.add("WetGain");
	parameterNames.add("Latency");
	parameterNames.add("ImpulseLength");
	parameterNames.add("ProcessInput");
	parameterNames.add("UseBackgroundThread");
	parameterNames.add("Predelay");
	parameterNames.add("HiCut");
	parameterNames.add("Damping");
	parameterNames.add("FFTType");
}

ConvolutionEffect::~ConvolutionEffect()
{
	getBuffer().removeListener(this);

	
}




float ConvolutionEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DryGain:		return Decibels::gainToDecibels(dryGain);
	case WetGain:		return Decibels::gainToDecibels(wetGain);
	case Latency:		return (float)latency;
	case ImpulseLength:	return 1.0f;
	case ProcessInput:	return processingEnabled ? 1.0f : 0.0f;
	case UseBackgroundThread:	return convolverL->isUsingBackgroundThread() ? 1.0f : 0.0f;
	case Predelay:		return predelayMs;
	case HiCut:			return (float)cutoffFrequency;
	case Damping:		return Decibels::gainToDecibels(damping);
	case FFTType:		return (float)(int)currentType;
	default:			jassertfalse; return 1.0f;
	}
}

void ConvolutionEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case DryGain:		dryGain = Decibels::decibelsToGain(newValue); 
						smoothedGainerDry.setParameter((int)GainSmoother::Parameters::Gain, dryGain); 
						break;
	case WetGain:		wetGain = Decibels::decibelsToGain(newValue);
						smoothedGainerWet.setParameter((int)GainSmoother::Parameters::Gain, wetGain);
						break;
	case Latency:		latency = (int)newValue;
		jassert(isPowerOfTwo(latency));
		setImpulse(sendNotificationAsync);
		break;
	case ImpulseLength:	setImpulse(sendNotificationAsync);
		break;
	case ProcessInput:	processingEnabled = newValue >= 0.5f;
						enableProcessing(processingEnabled); 
						break;
	case UseBackgroundThread:	
	{
		useBackgroundThread = newValue > 0.5f;

		{
			SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
            
            auto tToUse = useBackgroundThread && !nonRealtime ? &backgroundThread : nullptr;
            
			convolverL->setUseBackgroundThread(tToUse);
			convolverR->setUseBackgroundThread(tToUse);
		}
		
		break;
	}
	case Predelay:		predelayMs = newValue;
						calcPredelay();
						break;
	case HiCut:			cutoffFrequency = (double)newValue; 
						calcCutoff();
						
						break;
	case Damping:		damping = Decibels::decibelsToGain(newValue); 
						setImpulse(sendNotificationAsync);
						break;
	case FFTType:		
	{
		auto newType = (audiofft::ImplementationType)(int)newValue;

		if (newType != audiofft::ImplementationType::numImplementationTypes)
		{
			currentType = newType;
			setImpulse(sendNotificationSync);
		}
		
		break;
	}
	default:			jassertfalse; return;
	}
}

float ConvolutionEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DryGain:		return -100.0f;
	case WetGain:		return 0.0f;
	case Latency:		return 0.0f;
	case ImpulseLength:	return 1.0f;
	case ProcessInput:	return true;
	case UseBackgroundThread:	return false;
	case Predelay:		return 0.0f;
	case HiCut:			return 20000.0f;
	case Damping:		return 0.0f;
	case FFTType:		return (float)(int)audiofft::ImplementationType::BestAvailable;
	default:			jassertfalse; return 1.0f;
	}
}

void ConvolutionEffect::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	loadAttribute(DryGain, "DryGain");
	loadAttribute(WetGain, "WetGain");
	loadAttribute(Latency, "Latency");
	loadAttribute(ImpulseLength, "ImpulseLength");
	loadAttribute(ProcessInput, "ProcessInput");
	loadAttribute(UseBackgroundThread, "UseBackgroundThread");
	loadAttributeWithDefault(Predelay);
	loadAttributeWithDefault(HiCut);
	loadAttribute(Damping, "Damping");
	loadAttributeWithDefault(FFTType);

	AudioSampleProcessor::restoreFromValueTree(v);
}

ValueTree ConvolutionEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(DryGain, "DryGain");
	saveAttribute(WetGain, "WetGain");
	saveAttribute(Latency, "Latency");
	saveAttribute(ImpulseLength, "ImpulseLength");
	saveAttribute(ProcessInput, "ProcessInput");
	saveAttribute(UseBackgroundThread, "UseBackgroundThread");
	saveAttribute(Predelay, "Predelay");
	saveAttribute(HiCut, "HiCut");
	saveAttribute(Damping, "Damping");
	saveAttribute(FFTType, "FFTType");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

void ConvolutionEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	prepareBase(sampleRate, samplesPerBlock);
}

void ConvolutionEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	ADD_GLITCH_DETECTOR(this, DebugLogger::Location::ConvolutionRendering);

	if (startSample != 0)
	{
		debugError(this, "Buffer start not 0!");
	}


	auto numChannels = buffer.getNumChannels();
	auto channels = (float**)alloca(numChannels * sizeof(float*));
	
	for (int i = 0; i < numChannels; i++)
		channels[i] = buffer.getWritePointer(i, startSample);

	ProcessDataDyn d(channels, numSamples, numChannels);

#if ENABLE_ALL_PEAK_METERS
	currentValues.inL = FloatVectorOperations::findMaximum(channels[0], numSamples);
	currentValues.inR = FloatVectorOperations::findMaximum(channels[1], numSamples);
#endif

	processBase(d);

#if ENABLE_ALL_PEAK_METERS
	currentValues.outL = FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(0, startSample), numSamples);
	currentValues.outR = FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(1, startSample), numSamples);
#endif
}

void ConvolutionEffect::voicesKilled()
{
	resetBase();
}

ProcessorEditorBody *ConvolutionEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ConvolutionEditor(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}





} // namespace hise
