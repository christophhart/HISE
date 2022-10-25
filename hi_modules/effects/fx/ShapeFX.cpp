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


namespace hise {
using namespace juce;

ShapeFX::ShapeFX(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	LookupTableProcessor(mc, 1),
#if HI_USE_SHAPE_FX_SCRIPTING
	JavascriptProcessor(mc),
	ProcessorWithScriptingContent(mc),
#endif
	biasLeft(getDefaultValue(BiasLeft)),
	biasRight(getDefaultValue(BiasRight)),
	gain(1.0f),
	drive(getDefaultValue(Drive)),
	reduce(getDefaultValue(Reduce)),
	autogain(getDefaultValue(Autogain)),
	mix(getDefaultValue(Mix)),
	mode(Linear),
	limitInput(getDefaultValue(LimitInput)),
	highpass(getDefaultValue(HighPass)),
	lowpass(getDefaultValue(LowPass)),
#if HI_USE_SHAPE_FX_SCRIPTING
	functionCode(new SnippetDocument("shape", "input")),
	shapeResult(Result::ok()),
#endif
	dryBuffer(2, 0)
{
	
	initShapers();

	finaliseModChains();

#if HI_USE_SHAPE_FX_SCRIPTING
	initContent();

	String s;
	s << "function shape(input)\n";
	s << "{\n";
	s << "\treturn input;\n";
	s << "}\n";

	functionCode->replaceAllContent(s);
#endif

	tableUpdater = new TableUpdater(*this);

	memset(displayTable, 0, sizeof(float)*SAMPLE_LOOKUP_TABLE_SIZE);
    memset(unusedTable, 0, sizeof(float)*SAMPLE_LOOKUP_TABLE_SIZE);
	
	parameterNames.add("BiasLeft");
	parameterNames.add("BiasRight");
	parameterNames.add("HighPass");
	parameterNames.add("LowPass");
	parameterNames.add("Mode");
	parameterNames.add("Oversampling");
	parameterNames.add("Gain");
	parameterNames.add("Reduce");
	parameterNames.add("Autogain");
	parameterNames.add("LimitInput");
	parameterNames.add("Drive");
	parameterNames.add("Mix");
	parameterNames.add("BypassFilters");

#if HI_USE_SHAPE_FX_SCRIPTING
	setupApi();
#endif

	updateMode();
	updateOversampling();
	updateGain();
	updateMix();
}


ShapeFX::~ShapeFX()
{
	tableUpdater = nullptr;

#if HI_USE_SHAPE_FX_SCRIPTING
	cleanupEngine();
	clearExternalWindows();

	shapers.clear();

	functionCode = nullptr;
	
#if USE_BACKEND 
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif
#endif
}

void ShapeFX::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case BiasLeft: biasLeft = newValue; break;
	case BiasRight: biasRight = newValue; break;
	case HighPass: highpass = jmax<float>(20.0f, newValue); updateFilter(false); break;
	case LowPass: lowpass = jmax<float>(20.0f, newValue); updateFilter(true); break;
	case Mode: mode = (ShapeMode)(int)newValue; updateMode(); break;
	case Oversampling: 
		if (oversampleFactor != (int)newValue)
		{
			oversampleFactor = (int)newValue;
			updateOversampling(); 
			break;
		}
	case Gain: gain = Decibels::decibelsToGain(newValue); updateMode(); break;
	case Reduce: reduce = newValue; break;
	case Autogain: autogain = newValue > 0.5f; updateMode(); break;
	case LimitInput: limitInput = newValue > 0.5f; break;
	case Drive: drive = newValue; break;
	case Mix: mix = newValue; updateMix(); break;
	case BypassFilters:	bypassFilters = newValue > 0.5f; break;
	default:  jassertfalse;
	}
}

float ShapeFX::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case BiasLeft: return biasLeft;
	case BiasRight: return biasRight;
	case LowPass: return lowpass;
	case HighPass: return highpass;
	case Mode: return (float)mode;
	case Oversampling: return (float)oversampleFactor;
	case Gain: return Decibels::gainToDecibels(gain);
	case Reduce: return reduce;
	case Autogain: return autogain ? 1.0f : 0.0f;
	case LimitInput: return limitInput ? 1.0f : 0.0f;
	case Drive: return drive;
	case Mix: return mix;
	case BypassFilters: return bypassFilters ? 1.0f : 0.0f;
	default:  return 0.0f;
	}
}

float ShapeFX::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case BiasLeft: return 0.0f;
	case BiasRight: return 0.0f;
	case LowPass: return 20000.0f;
	case HighPass: return 20.0f;
	case Mode: return (float)Linear;
	case Oversampling: return (float)1;
	case Gain: return 0.0f;
	case Reduce: return 0.0f;
	case Autogain: return (float)true;
	case LimitInput: return (float)true;
	case Drive: return 0.0f;
	case Mix: return 1.0f;
	case BypassFilters: return 0.0f;
	default:  return 0.0f;
	}
}

juce::ValueTree ShapeFX::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

#if HI_USE_SHAPE_FX_SCRIPTING
	saveScript(v);
	saveContent(v);
#endif

	saveTable(getTableUnchecked(0), "Curve");

	saveAttribute(BiasLeft, "BiasLeft");
	saveAttribute(BiasRight, "BiasRight");
	saveAttribute(HighPass, "HighPass");
	saveAttribute(LowPass, "LowPass");
	saveAttribute(Mode, "Mode");
	saveAttribute(Oversampling, "Oversampling");
	saveAttribute(Gain, "Gain");
	saveAttribute(Reduce, "Reduce");
	saveAttribute(Autogain, "Autogain");
	saveAttribute(LimitInput, "LimitInput");
	saveAttribute(Drive, "Drive");
	saveAttribute(Mix, "Mix");
	saveAttribute(BypassFilters, "BypassFilters");

	return v;
}

void ShapeFX::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

#if HI_USE_SHAPE_FX_SCRIPTING
	restoreScript(v);
	restoreContent(v);
#endif

	loadTable(getTableUnchecked(0), "Curve");
	

	loadAttribute(BiasLeft, "BiasLeft");
	loadAttribute(BiasRight, "BiasRight");
	loadAttribute(HighPass, "HighPass");
	loadAttribute(LowPass, "LowPass");
	loadAttribute(Mode, "Mode");
	loadAttribute(Oversampling, "Oversampling");
	loadAttribute(Gain, "Gain");
	loadAttribute(Reduce, "Reduce");
	loadAttribute(Autogain, "Autogain");
	loadAttribute(LimitInput, "LimitInput");
	loadAttribute(Drive, "Drive");
	loadAttribute(Mix, "Mix");
	loadAttributeWithDefault(BypassFilters);
}

hise::ProcessorEditorBody * ShapeFX::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ShapeFXEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

#if HI_USE_SHAPE_FX_SCRIPTING
void ShapeFX::registerApiClasses()
{

	content = new ScriptingApi::Content(this);
	auto engineObject = new ScriptingApi::Engine(this);
	
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
}
#endif

void ShapeFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	ProcessorHelpers::increaseBufferIfNeeded(dryBuffer, samplesPerBlock);

	gainer.prepareToPlay(sampleRate, 0.04);
	autogainer.prepareToPlay(sampleRate, 0.04);

	mixSmootherL.reset(sampleRate, 0.04);
	mixSmoother_invL.reset(sampleRate, 0.04);

	mixSmootherR.reset(sampleRate, 0.04);
	mixSmoother_invR.reset(sampleRate, 0.04);

	lDelay.prepareToPlay(sampleRate);
	rDelay.prepareToPlay(sampleRate);

	updateOversampling();
	updateFilter(true);
	updateFilter(false);

	lHighPass.reset();
	rHighPass.reset();

	lLowPass.reset();
	rLowPass.reset();

	lDcRemover.reset();
	rDcRemover.reset();

	auto dcCoeffs = IIRCoefficients::makeHighPass(sampleRate, 30.0f);

	lDcRemover.setCoefficients(dcCoeffs);
	rDcRemover.setCoefficients(dcCoeffs);

	limiter.setSampleRate(sampleRate);
	limiter.setAttack(0.03);
	limiter.setRelease(100.0);
	limiter.setThresh(-0.5);
	limiter.initRuntime();
}

void ShapeFX::updateFilter(bool updateLowPass)
{
	if (getSampleRate() > 0)
	{
		if (updateLowPass)
		{
			auto coeff = IIRCoefficients::makeLowPass(getSampleRate(), lowpass);
			lLowPass.setCoefficients(coeff);
			rLowPass.setCoefficients(coeff);
		}
		else
		{
			auto coeff = IIRCoefficients::makeHighPass(getSampleRate(), highpass);
			lHighPass.setCoefficients(coeff);
			rHighPass.setCoefficients(coeff);
		}
	}
}

hise::ShapeFX::TableShaper * ShapeFX::getTableShaper()
{
	return static_cast<TableShaper*>(shapers[ShapeMode::Curve]);
}

const hise::ShapeFX::TableShaper * ShapeFX::getTableShaper() const
{
	return static_cast<const TableShaper*>(shapers[ShapeMode::Curve]);
}



void ShapeFX::processBitcrushedValues(float* l, float* r, int numSamples)
{
	if (reduce != 0.0f)
	{
		const float invStepSize = pow(2.0f, 16.0f - reduce);
		const float stepSize = 1.0f / invStepSize;

		for (int i = 0; i < numSamples; i++)
		{
			const float gainValue = bitCrushSmoother.getNextValue();



			l[i] = (stepSize * ceil(l[i] * gainValue * invStepSize) - 0.5f * stepSize) / gainValue;
			r[i] = (stepSize * ceil(r[i] * gainValue * invStepSize) - 0.5f * stepSize) / gainValue;
		}
	}
}

#if HI_USE_SHAPE_FX_SCRIPTING
void ShapeFX::rebuildScriptedTable()
{
	float sum = 0.0f;

	generateRampForDisplayValue(displayTable, gain);

	if (mode == ShapeMode::CachedScript)
	{
		// Lock the uncached script shaper because of race condition when switching between modes.
		// In normal mode, this should never occur.
		auto& lock = static_cast<ScriptShaper*>(shapers[ShapeMode::Script])->scriptLock;
		SpinLock::ScopedLockType sl(lock);

		for (int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
		{
			displayTable[i] = shapers[mode]->getSingleValue(displayTable[i]);
			sum += fabsf(displayTable[i]);
		}
	}
	else
	{
		for (int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
		{
			displayTable[i] = shapers[mode]->getSingleValue(displayTable[i]);
			sum += fabsf(displayTable[i]);
		}
	}

	if (mode == ShapeMode::CachedScript)
		static_cast<CachedScriptShaper*>(shapers[ShapeMode::CachedScript])->updateLookupTable(displayTable, gain);

	if (autogain)
	{
		sum /= 256.0f;
		autogainValue = 1.0f / sum;
		autogainValue = FloatSanitizers::sanitizeFloatNumber(autogainValue);
	}
	else
	{
		autogainValue = 1.0f;
	}

	updateGainSmoothers();

	graphNormalizeValue = autogainValue;
	triggerWaveformUpdate();
}
#endif

void ShapeFX::recalculateDisplayTable()
{
	generateRampForDisplayValue(displayTable, gain);


	shapers[mode]->processBlock(displayTable, unusedTable, SAMPLE_LOOKUP_TABLE_SIZE);

	graphNormalizeValue = autogainValue;
	triggerWaveformUpdate();
}

void ShapeFX::generateRampForDisplayValue(float* data, float gainToUse)
{
	for (int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
	{
		float inputValue = (float)i / (float)SAMPLE_LOOKUP_TABLE_SIZE;
		inputValue = 2.0f * inputValue - 1.0f;
		inputValue *= gainToUse;

		data[i] = inputValue;
	}
}

void ShapeFX::updateMode()
{
	updateGain();
}

void ShapeFX::updateOversampling()
{
    
#if HI_ENABLE_SHAPE_FX_OVERSAMPLER
	auto factor = roundToInt(log2((double)oversampleFactor));
#else
    auto factor = 0;
#endif

    ScopedPointer<Oversampler> newOverSampler = new Oversampler(2, factor, Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

	if (getLargestBlockSize() > 0)
		newOverSampler->initProcessing(getLargestBlockSize());

	int latency = roundToInt(newOverSampler->getLatencyInSamples());

	lDelay.setDelayTimeSamples(latency);
	rDelay.setDelayTimeSamples(latency);
    
	{
		SpinLock::ScopedLockType sl(oversamplerLock);

		oversampler.swapWith(newOverSampler);

		if (getSampleRate() > 0.0)
			bitCrushSmoother.reset(getSampleRate() * oversampleFactor, 0.04);
	}
}

void ShapeFX::updateGain()
{
	if (mode == Saturate)
		static_cast<InternalSaturator*>(shapers[Saturate])->updateAmount(gain);

	if (autogain)
	{
		float sum = 0.0f;

		for (int i = 0; i < 128; i++)
		{
			float in = (float)i / 127.0f * gain;
			sum += fabsf(shapers[mode]->getSingleValue(in));
		}

		sum /= 64.0f;

		autogainValue = 1.0f / sum;
		FloatSanitizers::sanitizeFloatNumber(autogainValue);
	}
	else
	{
		autogainValue = 1.0f;
	}

	updateGainSmoothers();
}

void ShapeFX::updateGainSmoothers()
{
	ScopedLock sl(getMainController()->getLock());

	gainer.setTargetValue(gain);
	autogainer.setTargetValue(autogainValue);
	bitCrushSmoother.setValue(autogainValue);
}

void ShapeFX::getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue)
{
	*tableValues = displayTable;
	numValues = SAMPLE_LOOKUP_TABLE_SIZE;
	normalizeValue = graphNormalizeValue;
}

void ShapeFX::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	float* dryL = dryBuffer.getWritePointer(0, startSample);
	float* dryR = dryBuffer.getWritePointer(1, startSample);

	float* wetL = b.getWritePointer(0, startSample);
	float* wetR = b.getWritePointer(1, startSample);
	
	FloatVectorOperations::copy(dryL, wetL, numSamples);
	FloatVectorOperations::copy(dryR, wetR, numSamples);

	mixSmoother_invL.applyGain(dryL, numSamples);
	mixSmoother_invR.applyGain(dryR, numSamples);

	if (!bypassFilters)
	{
		lHighPass.processSamples(wetL, numSamples);
		rHighPass.processSamples(wetR, numSamples);

		lLowPass.processSamples(wetL, numSamples);
		rLowPass.processSamples(wetR, numSamples);
	}
	
	gainer.processBlock(wetL, wetR, numSamples);

	inPeakValueL = b.getMagnitude(0, startSample, numSamples) * autogainValue - biasLeft;
	inPeakValueR = b.getMagnitude(1, startSample, numSamples) * autogainValue;

	FloatVectorOperations::add(wetL, biasLeft, numSamples);
	FloatVectorOperations::add(wetR, biasRight, numSamples);

	if (limitInput)
	{
		for (int i = 0; i < numSamples; i++)
		{
			double l = (double)wetL[i];
			double r = (double)wetR[i];

			limiter.process(l, r);

			wetL[i] = (float)l;
			wetR[i] = (float)r;
		}
	}

	if (oversampleFactor != 1)
	{
		dsp::AudioBlock<float> block(b, startSample);
		
		SpinLock::ScopedLockType sl(oversamplerLock);
		dsp::AudioBlock<float> oversampledData = oversampler->processSamplesUp(block);
		auto numOversampled = (int)oversampledData.getNumSamples();

		float* data_l = oversampledData.getChannelPointer(0);
		float* data_r = oversampledData.getChannelPointer(1);

		shapers[mode]->processBlock(data_l, data_r, numOversampled);
		processBitcrushedValues(data_l, data_r, numOversampled);
		oversampler->processSamplesDown(block);

		if (oversampler->getLatencyInSamples() > 0.0f)
		{
			lDelay.processBlock(dryL, numSamples);
			rDelay.processBlock(dryR, numSamples);
		}
	}
	else
	{
		SpinLock::ScopedLockType sl(oversamplerLock);

		shapers[mode]->processBlock(wetL, wetR, numSamples);
		processBitcrushedValues(wetL, wetR, numSamples);
	}

	if (!bypassFilters)
	{
		lDcRemover.processSamples(wetL, numSamples);
		rDcRemover.processSamples(wetR, numSamples);
	}

	if (autogain)
		autogainer.processBlock(wetL, wetR, numSamples);

	outPeakValueL = b.getMagnitude(0, startSample, numSamples);
	outPeakValueR = b.getMagnitude(1, startSample, numSamples);

	mixSmootherL.applyGain(wetL, numSamples);
	mixSmootherR.applyGain(wetR, numSamples);

	FloatVectorOperations::add(wetL, dryL, numSamples);
	FloatVectorOperations::add(wetR, dryR, numSamples);
}


void ShapeFX::updateMix()
{
	mixSmootherL.setValue(mix);
	mixSmoother_invL.setValue(1.0f - mix);

	mixSmootherR.setValue(mix);
	mixSmoother_invR.setValue(1.0f - mix);
}

PolyshapeFX::PolyshapeFX(MainController *mc, const String &uid, int numVoices):
	VoiceEffectProcessor(mc, uid, numVoices),
	ProcessorWithStaticExternalData(mc, 2, 0, 0, 1),
	polyUpdater(*this),
	dcRemovers(numVoices)
{
	modChains += { this, "Drive Modulation" };

	finaliseModChains();

	modChains[InternalChains::DriveModulation].setExpandToAudioRate(true);

	connectWaveformUpdaterToComplexUI(getDisplayBuffer(0), true);

	for (int i = 0; i < numVoices; i++)
	{
#if HI_ENABLE_SHAPE_FX_OVERSAMPLER
        auto factor = 2;
#else
        auto factor = 0;
#endif
        
		oversamplers.add(new ShapeFX::Oversampler(2, factor, ShapeFX::Oversampler::FilterType::filterHalfBandPolyphaseIIR, false));
		driveSmoothers[i] = LinearSmoothedValue<float>(0.0f);
	}

	initShapers();

    memset(unusedTable, 0, sizeof(float)*SAMPLE_LOOKUP_TABLE_SIZE);
    
	tableUpdater = new TableUpdater(*this);

	parameterNames.add("Drive");
	parameterNames.add("Mode");
	parameterNames.add("Oversampling");
	parameterNames.add("Bias");

	recalculateDisplayTable();
}

PolyshapeFX::~PolyshapeFX()
{
	tableUpdater = nullptr;
	shapers.clear();
	
	oversamplers.clear();
}

float PolyshapeFX::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Drive: return Decibels::gainToDecibels(drive);
	case Mode: return (float)mode;
	case Oversampling: return oversampling ? 1.0f : 0.0f;
	case Bias: return bias;
	default: break;
	}

	return 0.0f;
}

void PolyshapeFX::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Drive: drive = Decibels::decibelsToGain(newValue); recalculateDisplayTable(); break;
	case Mode: mode = (int)newValue; recalculateDisplayTable(); break;
	case Oversampling: oversampling = newValue > 0.5f; break;
	case Bias: bias = newValue; break;
	}
}

float PolyshapeFX::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Drive: return 1.0f;
	case Mode: return (float)ShapeFX::ShapeMode::Linear;
	case Oversampling: return false;
	case Bias: return 0.0f;
	default: break;
	}

	return 0.0f;
}

void PolyshapeFX::restoreFromValueTree(const ValueTree &v)
{
	VoiceEffectProcessor::restoreFromValueTree(v);

	loadTable(getTable(0), "Curve");
	loadTable(getTable(1), "AsymetricalCurve");

	loadAttribute(Drive, "Drive");
	loadAttribute(Mode, "Mode");
	loadAttribute(Oversampling, "Oversampling");
}

juce::ValueTree PolyshapeFX::exportAsValueTree() const
{
	auto v = VoiceEffectProcessor::exportAsValueTree();

	saveTable(getTableUnchecked(0), "Curve");
	saveTable(getTableUnchecked(1), "AsymetricalCurve");

	saveAttribute(Drive, "Drive");
	saveAttribute(Mode, "Mode");
	saveAttribute(Oversampling, "Oversampling");

	return v;
}

hise::ProcessorEditorBody * PolyshapeFX::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new PolyShapeFXEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


void PolyshapeFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	for (auto&mb : modChains)
		mb.prepareToPlay(sampleRate, samplesPerBlock);

	for (int i = 0; i < NUM_POLYPHONIC_VOICES; i++)
	{
		driveSmoothers[i].reset(sampleRate, 0.05);
	}

	for (auto os : oversamplers)
	{
		os->initProcessing(samplesPerBlock);
		os->reset();
	}

	for (auto& dc : dcRemovers)
	{
		dc.setFrequency(20.0);
		dc.setSampleRate(sampleRate);
		dc.setType(SimpleOnePoleSubType::FilterType::HP);
		dc.setNumChannels(2);
		dc.reset();
	}
}

void PolyshapeFX::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	auto& driveChain = modChains[DriveModulation];

	if (voiceIndex >= NUM_POLYPHONIC_VOICES)
		return;

	
	auto scratch = (float*)alloca(sizeof(float) * numSamples);

	auto smoother = &driveSmoothers[voiceIndex];

	smoother->setValue(drive - 1.0f);

	if (auto compressedDriveValues = driveChain.getReadPointerForVoiceValues(startSample))
	{
		FloatVectorOperations::copy(scratch, compressedDriveValues, numSamples);
	}
	else
	{
		auto constantValue = driveChain.getConstantModulationValue();
		FloatVectorOperations::fill(scratch, constantValue, numSamples);
	}

	smoother->applyGain(scratch, numSamples);

	FloatVectorOperations::add(scratch, 1.0f, numSamples);

	float* l = b.getWritePointer(0, startSample);
	float* r = b.getWritePointer(1, startSample);

	if (mode == ShapeFX::ShapeMode::Sin || mode == ShapeFX::ShapeMode::TanCos)
	{
		if (bias != 0.0f)
		{
			FloatVectorOperations::multiply(l, scratch, numSamples);
			FloatVectorOperations::add(l, bias, numSamples);
			FloatVectorOperations::multiply(r, scratch, numSamples);
			FloatVectorOperations::add(r, bias, numSamples);
		}
		else
		{
			FloatVectorOperations::multiply(l, scratch, numSamples);
			FloatVectorOperations::multiply(r, scratch, numSamples);
		}
	}
	else
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = (l[i] + bias) * (1.0f + scratch[i]);
			r[i] = (r[i] + bias) * (1.0f + scratch[i]);
		}
	}

	if (oversampling)
	{
		dsp::AudioBlock<float> block(b.getArrayOfWritePointers(), 2, startSample, numSamples);

		auto os = oversamplers[voiceIndex];
		
		dsp::AudioBlock<float> oversampledData = os->processSamplesUp(block);
		auto numOversampled = oversampledData.getNumSamples();

		float* o_l = oversampledData.getChannelPointer(0);
		float* o_r = oversampledData.getChannelPointer(1);

		shapers[mode]->processBlock(o_l, o_r, (int)numOversampled);
		
		os->processSamplesDown(block);
	}
	else
	{
		shapers[mode]->processBlock(l, r, numSamples);
	}

	const float attenuation = 0.03162f;

	for (int i = 0; i < numSamples; i++)
	{
		l[i] /= (attenuation * scratch[i] + 1.0f);
		r[i] /= (attenuation * scratch[i] + 1.0f);
	}

	if (bias != 0.0f || mode == ShapeFX::ShapeMode::AsymetricalCurve)
	{
		FilterHelpers::RenderData renderData(b, startSample, numSamples);
		dcRemovers[voiceIndex].render(renderData);
	}
		

}

void PolyshapeFX::getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue)
{
	const float driveValue = 1.0f + (modChains[DriveModulation].getChain()->getOutputValue() * (drive-1.0f));

	displayPeak = driveValue;

	ShapeFX::generateRampForDisplayValue(displayTable, displayPeak);

	if (auto s = shapers[mode])
	{
		shapers[mode]->processBlock(displayTable, unusedTable, SAMPLE_LOOKUP_TABLE_SIZE);
	}

	*tableValues = displayTable;
	numValues = SAMPLE_LOOKUP_TABLE_SIZE;

	switch (mode)
	{
	case ShapeFX::ShapeMode::Atan:				normalizeValue = 1.0f / atanf(displayPeak); break;
	case ShapeFX::ShapeMode::Asinh:				normalizeValue = 1.0f / asinhf(displayPeak); break;
	case ShapeFX::ShapeMode::AsymetricalCurve:	normalizeValue = 1.0f; break;
	default:									normalizeValue = 1.0f;	break;
	}
}

void PolyshapeFX::recalculateDisplayTable()
{
	
}



void PolyshapeFX::startVoice(int voiceIndex, const HiseEvent& e)
{
	VoiceEffectProcessor::startVoice(voiceIndex, e);

	driveSmoothers[voiceIndex].setValueWithoutSmoothing(drive-1.0f);

}

}
