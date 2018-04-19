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

ShapeFX::ShapeFX(MainController *mc, const String &uid):
	MasterEffectProcessor(mc, uid),
	JavascriptProcessor(mc),
	ProcessorWithScriptingContent(mc),
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
	table(new SampleLookupTable()),
	tableUpdater(new TableUpdater(*this)),
	functionCode(new SnippetDocument("shape", "input")),
	dryBuffer(2, 0)
{
	initContent();

	//functionCode.replaceAllContent(c);

	memset(displayTable, 0, sizeof(float)*SAMPLE_LOOKUP_TABLE_SIZE);

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

	updateMode();
	updateOversampling();
	updateGain();
	updateMix();
}


ShapeFX::~ShapeFX()
{
	cleanupEngine();
	clearExternalWindows();

	functionCode = nullptr;
	
#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif

	tableUpdater = nullptr;
	table = nullptr;
	
}

void ShapeFX::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case BiasLeft: biasLeft = newValue; break;
	case BiasRight: biasRight = newValue; break;
	case HighPass: highpass = newValue; updateFilter(false); break;
	case LowPass: lowpass = newValue; updateFilter(true); break;
	case Mode: mode = (ShapeMode)(int)newValue; updateMode(); break;
	case Oversampling: oversampleFactor = (int)newValue; updateOversampling(); break;
	case Gain: gain = Decibels::decibelsToGain(newValue); updateGain(); updateMode(); break;
	case Reduce: reduce = newValue; break;
	case Autogain: autogain = newValue > 0.5f; break;
	case Drive: drive = newValue; break;
	case Mix: mix = newValue; updateMix(); break;
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
	case Drive: return drive;
	case Mix: return mix;
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
	case Reduce: return 1.0f;
	case Autogain: return true;
	case Drive: return 0.0f;
	case Mix: return 1.0f;
	default:  return 0.0f;
	}
}

juce::ValueTree ShapeFX::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveScript(v);

	saveAttribute(BiasLeft, "BiasLeft");
	saveAttribute(BiasRight, "BiasRight");
	
	saveAttribute(HighPass, "HighPass");
	saveAttribute(LowPass, "LowPass");
	saveAttribute(Mode, "Mode");
	saveAttribute(Oversampling, "Oversampling");
	saveAttribute(Gain, "Gain");
	saveAttribute(Reduce, "Reduce");
	saveAttribute(Autogain, "Autogain");
	saveAttribute(Drive, "Drive");
	saveAttribute(Mix, "Mix");

	return v;
}

void ShapeFX::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	restoreScript(v);

	loadAttribute(BiasLeft, "BiasLeft");
	loadAttribute(BiasRight, "BiasRight");
	
	loadAttribute(HighPass, "HighPass");
	loadAttribute(LowPass, "LowPass");
	loadAttribute(Mode, "Mode");
	loadAttribute(Oversampling, "Oversampling");
	loadAttribute(Gain, "Gain");
	loadAttribute(Reduce, "Reduce");
	loadAttribute(Autogain, "Autogain");
	loadAttribute(Drive, "Drive");
	loadAttribute(Mix, "Mix");
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


void ShapeFX::registerApiClasses()
{
	content = new ScriptingApi::Content(this);
	auto engineObject = new ScriptingApi::Engine(this);
	
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
}

void ShapeFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	ProcessorHelpers::increaseBufferIfNeeded(dryBuffer, samplesPerBlock);

	lgain.reset(sampleRate, 0.04);
	rgain.reset(sampleRate, 0.04);
	
	lgain_inv.reset(sampleRate, 0.04);
	rgain_inv.reset(sampleRate, 0.04);

	mixSmootherL.reset(sampleRate, 0.04);
	mixSmoother_invL.reset(sampleRate, 0.04);

	mixSmootherR.reset(sampleRate, 0.04);
	mixSmoother_invR.reset(sampleRate, 0.04);

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

void ShapeFX::updateMode()
{
	std::function<float(float)> f;

	switch (mode)
	{
	case hise::ShapeFX::Linear:
		f = ShapeFunctions::linear;
		break;
	case hise::ShapeFX::Atan:
		f = ShapeFunctions::atan;
		break;
	case hise::ShapeFX::Tanh:
		f = ShapeFunctions::tanh;
		break;
	case hise::ShapeFX::Square:
		f = ShapeFunctions::square;
		break;
	case hise::ShapeFX::SquareRoot:
		f = ShapeFunctions::squareRoot;
		break;
	case hise::ShapeFX::Curve:
	{
		auto tmp = table->getReadPointer();
		f = [tmp](float input) { return ShapeFunctions::symetricLookup(tmp, input); };
		break;
	}
	case hise::ShapeFX::Function:
	{
		if (!lastCompileWasOK)
		{
			f = ShapeFunctions::linear;
			break;
		}

		HiseJavascriptEngine* tmp = scriptEngine;

		f = [tmp](float input)
		{
			
			Result r = Result::ok();
			tmp->setCallbackParameter(0, 0, input);
			auto value = jlimit<float>(-1.0f, 1.0f, (float)tmp->executeCallback(0, &r));
			value = FloatSanitizers::sanitizeFloatNumber(value);

			if (r.wasOk())
				return value;

			return input;
		};
		break;
	}
	case hise::ShapeFX::numModes:
		break;
	default:
		break;
	}

	if (f)
	{
		for (int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
		{
			float inputValue = (float)i / (float)SAMPLE_LOOKUP_TABLE_SIZE;

			inputValue = 2.0f * inputValue - 1.0f;

			inputValue *= gain;

			displayTable[i] = f(inputValue);
		}
	}

	triggerWaveformUpdate();
}

void ShapeFX::updateGain()
{
	lgain.setValue(gain);
	rgain.setValue(gain);

	lgain_inv.setValue(1.0f / gain);
	rgain_inv.setValue(1.0f / gain);

}

void ShapeFX::getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue)
{
	*tableValues = displayTable;
	numValues = SAMPLE_LOOKUP_TABLE_SIZE;
	normalizeValue = 0.5f;
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

	lgain.applyGain(wetL, numSamples);
	rgain.applyGain(wetR, numSamples);

	lHighPass.processSamples(wetL, numSamples);
	rHighPass.processSamples(wetR, numSamples);
	
	lLowPass.processSamples(wetL, numSamples);
	rLowPass.processSamples(wetR, numSamples);
	
	// For the lookup-table based shapers we need a brickwall limiter to avoid
	// nasty hard-clipping
	const bool useLimiter = mode == ShapeMode::Curve || ShapeMode::Function;

	if (useLimiter)
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

	inPeakValueL = b.getMagnitude(0, startSample, numSamples);
	inPeakValueR = b.getMagnitude(1, startSample, numSamples);

	dsp::AudioBlock<float> block(b, startSample);

	ScopedLock sl(oversamplerLock);

	dsp::AudioBlock<float> oversampledData = oversampler->processSamplesUp(block);

	auto numOversampled = oversampledData.getNumSamples();

	float* o_l = oversampledData.getChannelPointer(0);
	float* o_r = oversampledData.getChannelPointer(1);

	FloatVectorOperations::add(o_l, biasLeft, numOversampled);
	FloatVectorOperations::add(o_r, biasRight, numOversampled);

	switch (mode)
	{
	case hise::ShapeFX::Linear:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::linear(o_l[i]);
			o_r[i] = ShapeFunctions::linear(o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::Atan:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::atan(o_l[i]);
			o_r[i] = ShapeFunctions::atan(o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::Tanh:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::tanh(o_l[i]);
			o_r[i] = ShapeFunctions::tanh(o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::Square:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::square(o_l[i]);
			o_r[i] = ShapeFunctions::square(o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::SquareRoot:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::squareRoot(o_l[i]);
			o_r[i] = ShapeFunctions::squareRoot(o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::Curve:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::symetricLookup(table->getReadPointer(), o_l[i]);
			o_r[i] = ShapeFunctions::symetricLookup(table->getReadPointer(), o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::Function:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			o_l[i] = ShapeFunctions::asymetricLookup(displayTable, o_l[i]);
			o_r[i] = ShapeFunctions::asymetricLookup(displayTable, o_r[i]);
		}

		break;
	}
	case hise::ShapeFX::numModes:
	default:
		break;
	}
	
	oversampler->processSamplesDown(block);

	lDcRemover.processSamples(wetL, numSamples);
	rDcRemover.processSamples(wetR, numSamples);

	

	if (autogain)
	{
		lgain_inv.applyGain(wetL, numSamples);
		rgain_inv.applyGain(wetR, numSamples);
	}

	outPeakValueL = b.getMagnitude(0, startSample, numSamples);
	outPeakValueR = b.getMagnitude(1, startSample, numSamples);

	mixSmootherL.applyGain(wetL, numSamples);
	mixSmootherR.applyGain(wetR, numSamples);

	FloatVectorOperations::add(wetL, dryL, numSamples);
	FloatVectorOperations::add(wetR, dryR, numSamples);
}

float ShapeFX::getCurveValue(float input)
{
	auto v = abs((double)input);
	auto sign = (0.f < input) - (input < 0.0f);
	return (float)sign * table->getInterpolatedValue(v * 512.0);
}


void ShapeFX::updateMix()
{
	mixSmootherL.setValue(mix);
	mixSmoother_invL.setValue(1.0f - mix);

	mixSmootherR.setValue(mix);
	mixSmoother_invR.setValue(1.0f - mix);
}

}
