/*
  ==============================================================================

    Dynamics.cpp
    Created: 27 Oct 2017 6:06:20pm
    Author:  Christoph

  ==============================================================================
*/

namespace hise { using namespace juce;

DynamicsEffect::DynamicsEffect(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	gateEnabled(false),
	compressorEnabled(false),
	limiterEnabled(false),
	limiterPending(false),
	limiterMakeupGain(1.0f),
	compressorMakeupGain(1.0f),
	limiterMakeup(false),
	compressorMakeup(false)
{
	finaliseModChains();

	parameterNames.add("GateEnabled");
	parameterNames.add("GateThreshold");
	parameterNames.add("GateAttack");
	parameterNames.add("GateRelease");
	parameterNames.add("GateReduction");
	parameterNames.add("CompressorEnabled");
	parameterNames.add("CompressorThreshold");
	parameterNames.add("CompressorRatio");
	parameterNames.add("CompressorAttack");
	parameterNames.add("CompressorRelease");
	parameterNames.add("CompressorReduction");
	parameterNames.add("CompressorMakeup");
	parameterNames.add("LimiterEnabled");
	parameterNames.add("LimiterThreshold");
	parameterNames.add("LimiterAttack");
	parameterNames.add("LimiterRelease");
	parameterNames.add("LimiterReduction");
	parameterNames.add("LimiterMakeup");

	
}

void DynamicsEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	auto p = (Parameters)parameterIndex;

	switch (p)
	{
	case GateEnabled:			gateEnabled = newValue > 0.5f; break;
	case CompressorEnabled:		compressorEnabled = newValue > 0.5f; break;
	case LimiterEnabled:
	{
		const bool isEnabled = newValue > 0.5f;
		limiterPending = isEnabled != limiterEnabled;
		limiterEnabled = isEnabled;
		break;
	}
	case GateThreshold:			gate.setThresh((SimpleDataType)newValue); break;
	case CompressorThreshold:	compressor.setThresh((SimpleDataType)newValue); updateMakeupValues(false); break;
	case LimiterThreshold:		limiter.setThresh((SimpleDataType)newValue); updateMakeupValues(true); break;
	case GateAttack:			gate.setAttack((SimpleDataType)newValue); break;
	case CompressorAttack:		compressor.setAttack((SimpleDataType)newValue); break;
	case LimiterAttack:			limiter.setAttack((SimpleDataType)newValue); break;
	case GateRelease:			gate.setRelease((SimpleDataType)newValue); break;
	case CompressorRelease:		compressor.setRelease((SimpleDataType)newValue); break;
	case LimiterRelease:		limiter.setRelease((SimpleDataType)newValue); break;
	case CompressorRatio:		compressor.setRatio((SimpleDataType)(1.0f / newValue)); updateMakeupValues(false); break;
	case CompressorMakeup:		compressorMakeup = newValue > 0.5f; updateMakeupValues(false); break;
	case LimiterMakeup:			limiterMakeup = newValue > 0.5f; updateMakeupValues(true); break;
	case GateReduction:
	case CompressorReduction:
	case LimiterReduction:		break;
	case numParameters:			jassertfalse;
		break;
	default:
		break;
	}
}

float DynamicsEffect::getAttribute(int parameterIndex) const
{
	auto p = (Parameters)parameterIndex;

	switch (p)
	{
	case GateEnabled:			return gateEnabled ? 1.0f : 0.0f;
	case CompressorEnabled:		return compressorEnabled ? 1.0f : 0.0f;
	case LimiterEnabled:		return limiterEnabled ? 1.0f : 0.0f;
	case GateThreshold:			return (float)gate.getThresh();
	case CompressorThreshold:	return (float)compressor.getThresh(); 
	case LimiterThreshold:		return (float)limiter.getThresh();
	case GateAttack:			return (float)gate.getAttack();
	case CompressorAttack:		return (float)compressor.getAttack();
	case LimiterAttack:			return (float)limiter.getAttack();
	case GateRelease:			return (float)gate.getRelease();
	case CompressorRelease:		return (float)compressor.getRelease();
	case LimiterRelease:		return (float)limiter.getRelease();
	case CompressorRatio:		return 1.0f / (float)compressor.getRatio();
	case GateReduction:			return gateReduction;
	case CompressorReduction:	return compressorReduction;
	case LimiterReduction:		return limiterReduction;
	case CompressorMakeup:		return compressorMakeup ? 1.0f : 0.0f;
	case LimiterMakeup:			return limiterMakeup ? 1.0f : 0.0f;
	default:
		break;
	}

	return 0.0f;
}

float DynamicsEffect::getDefaultValue(int parameterIndex) const
{
	auto p = (Parameters)parameterIndex;

	switch (p)
	{
	case GateEnabled:			return false;
	case CompressorEnabled:		return false;
	case LimiterEnabled:		return false;
	case GateThreshold:			return -100.0f;
	case CompressorThreshold:	return 0.0f;
	case LimiterThreshold:		return 0.0f;
	case GateAttack:			return 10.0f;
	case CompressorAttack:		return 10.0f;
	case LimiterAttack:			return 10.0f;
	case GateRelease:			return 40.0f;
	case CompressorRelease:		return 40.0f;
	case LimiterRelease:		return 40.0f;
	case CompressorRatio:		return 1.0f;
	case GateReduction:			return 0.f;
	case CompressorReduction:	return 0.f;
	case LimiterReduction:		return 0.f;
	case LimiterMakeup:			return false;
	case CompressorMakeup:		return false;
	case numParameters:			jassertfalse;
		
	default:
		break;
	}

	return 0.0f;
}

void DynamicsEffect::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	loadAttribute(GateEnabled, "GateEnabled");
	loadAttribute(GateThreshold, "GateThreshold");
	loadAttribute(GateAttack, "GateAttack");
	loadAttribute(GateRelease, "GateRelease");
	loadAttribute(CompressorEnabled, "CompressorEnabled");
	loadAttribute(CompressorThreshold, "CompressorThreshold");
	loadAttribute(CompressorRatio, "CompressorRatio");
	loadAttribute(CompressorAttack, "CompressorAttack");
	loadAttribute(CompressorRelease, "CompressorRelease");
	loadAttribute(LimiterEnabled, "LimiterEnabled");
	loadAttribute(LimiterThreshold, "LimiterThreshold");
	loadAttribute(LimiterAttack, "LimiterAttack");
	loadAttribute(LimiterRelease, "LimiterRelease");
	loadAttribute(CompressorMakeup, "CompressorMakeup");
	loadAttribute(LimiterMakeup, "LimiterMakeup");
}

ValueTree DynamicsEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(GateEnabled, "GateEnabled");
	saveAttribute(GateThreshold, "GateThreshold");
	saveAttribute(GateAttack, "GateAttack");
	saveAttribute(GateRelease, "GateRelease");
	saveAttribute(CompressorEnabled, "CompressorEnabled");
	saveAttribute(CompressorThreshold, "CompressorThreshold");
	saveAttribute(CompressorRatio, "CompressorRatio");
	saveAttribute(CompressorAttack, "CompressorAttack");
	saveAttribute(CompressorRelease, "CompressorRelease");
	saveAttribute(LimiterEnabled, "LimiterEnabled");
	saveAttribute(LimiterThreshold, "LimiterThreshold");
	saveAttribute(LimiterAttack, "LimiterAttack");
	saveAttribute(LimiterRelease, "LimiterRelease");
	saveAttribute(CompressorMakeup, "CompressorMakeup");
	saveAttribute(LimiterMakeup, "LimiterMakeup");

	return v;
}

ProcessorEditorBody * DynamicsEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new DynamicsEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

void DynamicsEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	const int numToProcess = numSamples - startSample;

	if (gateEnabled)
	{
		float* l = buffer.getWritePointer(0, startSample);
		float* r = buffer.getWritePointer(1, startSample);

		for (int i = 0; i < numToProcess; i++)
		{
			SimpleDataType l_ = (SimpleDataType)*l;
			SimpleDataType r_ = (SimpleDataType)*r;

			gate.process(l_, r_);

			const float gR = (float)gate.getGainReduction();

			if (gR > gateReduction)
				gateReduction = gR;
			else
				gateReduction = gateReduction * 0.9999f;

			*l++ = (float)l_;
			*r++ = (float)r_;
		}
	}

	if (compressorEnabled)
	{
		float* l = buffer.getWritePointer(0, startSample);
		float* r = buffer.getWritePointer(1, startSample);

		for (int i = 0; i < numToProcess; i++)
		{
			SimpleDataType l_ = (SimpleDataType)*l;
			SimpleDataType r_ = (SimpleDataType)*r;

			compressor.process(l_, r_);

			const float gR = (float)compressor.getGainReduction();

			if (gR > compressorReduction)
				compressorReduction = gR;
			else
				compressorReduction = compressorReduction * 0.9999f;

			*l++ = (float)l_;
			*r++ = (float)r_;
		}

		if (compressorMakeup)
		{
			FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), compressorMakeupGain, numToProcess);
			FloatVectorOperations::multiply(buffer.getWritePointer(1, startSample), compressorMakeupGain, numToProcess);
		}
	}

	if (limiterEnabled || limiterPending)
	{
		if (limiterPending)
		{
			float* temp[2];

			temp[0] = (float*)alloca(sizeof(float) * numToProcess);
			temp[1] = (float*)alloca(sizeof(float) * numToProcess);

			const float dryRampStart = limiterEnabled ? 1.0f : 0.0f;
			const float dryRampEnd = 1.0f - dryRampStart;
			const float wetRampStart = dryRampEnd;
			const float wetRampEnd = dryRampStart;

			AudioSampleBuffer tempBuffer(temp, 2, numToProcess);
			tempBuffer.clear();
			tempBuffer.copyFromWithRamp(0, 0, buffer.getReadPointer(0, startSample), numToProcess, dryRampStart, dryRampEnd);
			tempBuffer.copyFromWithRamp(1, 0, buffer.getReadPointer(1, startSample), numToProcess, dryRampStart, dryRampEnd);

			applyLimiter(buffer, startSample, numToProcess);

			buffer.applyGainRamp(startSample, numToProcess, wetRampStart, wetRampEnd);
			
			FloatVectorOperations::add(buffer.getWritePointer(0, startSample), temp[0], numToProcess);
			FloatVectorOperations::add(buffer.getWritePointer(1, startSample), temp[1], numToProcess);

			limiterPending = false;
		}
		else
		{
			applyLimiter(buffer, startSample, numToProcess);
		}
	}
}

void DynamicsEffect::applyLimiter(AudioSampleBuffer &buffer, int startSample, const int numToProcess)
{
	float* l = buffer.getWritePointer(0, startSample);
	float* r = buffer.getWritePointer(1, startSample);



	for (int i = 0; i < numToProcess; i++)
	{
		SimpleDataType l_ = (SimpleDataType)*l;
		SimpleDataType r_ = (SimpleDataType)*r;

		limiter.process(l_, r_);

		const float gR = (float)limiter.getGainReduction();

		if (gR > limiterReduction)
			limiterReduction = gR;
		else
			limiterReduction = limiterReduction * 0.9999f;

		*l++ = (float)l_;
		*r++ = (float)r_;
	}

	if (limiterMakeup)
	{
		FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), limiterMakeupGain, numToProcess);
		FloatVectorOperations::multiply(buffer.getWritePointer(1, startSample), limiterMakeupGain, numToProcess);
	}
}

void DynamicsEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	gate.initRuntime();
	compressor.initRuntime();
	limiter.initRuntime();

	gate.setSampleRate(sampleRate);
	compressor.setSampleRate(sampleRate);
	limiter.setSampleRate(sampleRate);
}


void DynamicsEffect::updateMakeupValues(bool updateLimiter)
{
	if (updateLimiter)
	{
		if (limiterMakeup)
			limiterMakeupGain = (float)Decibels::decibelsToGain(limiter.getThresh() * -1.0);
		else
			limiterMakeupGain = 1.0f;
	}
	else
	{
		if (compressorMakeup)
		{
			auto attenuation = compressor.getThresh();
			auto ratio = compressor.getRatio();
			auto gainDb = (1.0 - ratio) * attenuation * -1.0;

			compressorMakeupGain = (float)Decibels::decibelsToGain(gainDb);
		}
		else
			compressorMakeupGain = 1.0f;
	}
}

} // namespace hise
