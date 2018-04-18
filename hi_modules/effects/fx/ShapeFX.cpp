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
	biasLeft(getDefaultValue(BiasLeft)),
	biasRight(getDefaultValue(BiasRight)),
	gain(getDefaultValue(Gain)),
	drive(getDefaultValue(Drive)),
	reduce(getDefaultValue(Reduce)),
	autogain(getDefaultValue(Autogain)),
	mix(getDefaultValue(Mix)),
	mode(Linear),
	highpass(getDefaultValue(HighPass)),
	table(new SampleLookupTable())
{
	memset(displayTable, 0, sizeof(float)*SAMPLE_LOOKUP_TABLE_SIZE);

	parameterNames.add("BiasLeft");
	parameterNames.add("BiasRight");
	parameterNames.add("HighPass");
	parameterNames.add("Mode");
	parameterNames.add("Oversampling");
	parameterNames.add("Gain");
	parameterNames.add("Reduce");
	parameterNames.add("Autogain");
	parameterNames.add("Drive");
	parameterNames.add("Mix");

	

	updateMode();
	updateOversampling();
}

void ShapeFX::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case BiasLeft: biasLeft = newValue; break;
	case BiasRight: biasRight = newValue; break;
	case HighPass: highpass = newValue; updateFilter(); break;
	case Mode: mode = (ShapeMode)(int)newValue; updateMode(); break;
	case Oversampling: oversampleFactor = (int)newValue; updateOversampling(); break;
	case Gain: gain = Decibels::decibelsToGain(newValue); updateGain(); break;
	case Reduce: reduce = newValue; break;
	case Autogain: autogain = newValue > 0.5f; break;
	case Drive: drive = newValue; break;
	case Mix: mix = newValue; break;
	default:  jassertfalse;
	}
}

float ShapeFX::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case BiasLeft: return biasLeft;
	case BiasRight: return biasRight;
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

	saveAttribute(BiasLeft, "BiasLeft");
	saveAttribute(BiasRight, "BiasRight");
	saveAttribute(HighPass, "HighPass");
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

	loadAttribute(BiasLeft, "BiasLeft");
	loadAttribute(BiasRight, "BiasRight");
	loadAttribute(HighPass, "HighPass");
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

void ShapeFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	lgain.reset(sampleRate, 0.04);
	rgain.reset(sampleRate, 0.04);
	
	lgain_inv.reset(sampleRate, 0.04);
	rgain_inv.reset(sampleRate, 0.04);

	updateOversampling();
	updateFilter();

	lFilter.reset();
	rFilter.reset();
}

void ShapeFX::updateFilter()
{
	if (getSampleRate() > 0)
	{
		auto coeff = IIRCoefficients::makeHighPass(getSampleRate(), highpass);
		lFilter.setCoefficients(coeff);
		rFilter.setCoefficients(coeff);
	}
	
}

void ShapeFX::updateMode()
{
	std::function<float(float)> f;

	switch (mode)
	{
	case hise::ShapeFX::Linear:
		f = [](float input) { return input; };
		break;
	case hise::ShapeFX::Atan:
		f = std::atanf;
		break;
	case hise::ShapeFX::Tanh:
		f = std::tanhf;
		break;
	case hise::ShapeFX::Square:
		f = [](float input) { return input * input; };
		break;
	case hise::ShapeFX::SquareRoot:
		f = [](float input) { return sqrtf(input); };
		break;
	case hise::ShapeFX::Curve:
	case hise::ShapeFX::Function:
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
	*tableValues = mode == Curve ? table->getReadPointer() : displayTable;
	numValues = SAMPLE_LOOKUP_TABLE_SIZE;
	normalizeValue = 0.5f;
}

void ShapeFX::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	dsp::AudioBlock<float> block(b, startSample);

	lgain.applyGain(b.getWritePointer(0, startSample), numSamples);
	rgain.applyGain(b.getWritePointer(1, startSample), numSamples);

	lFilter.processSamples(b.getWritePointer(0, startSample), numSamples);
	rFilter.processSamples(b.getWritePointer(1, startSample), numSamples);
	
	ScopedLock sl(oversamplerLock);

	dsp::AudioBlock<float>& oversampledData = oversampler->processSamplesUp(block);

	auto numOversampled = oversampledData.getNumSamples();

	float* l = oversampledData.getChannelPointer(0);
	float* r = oversampledData.getChannelPointer(1);

	

	for (int i = 0; i < numOversampled; i++)
	{
		l[i] += biasLeft;
		r[i] += biasRight;
	}

	switch (mode)
	{
	case hise::ShapeFX::Linear:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			l[i] = (l[i]);
			r[i] = (r[i]);
		}

		break;
	}
	case hise::ShapeFX::Atan:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			l[i] = std::atanf(l[i]);
			r[i] = std::atanf(r[i]);
		}

		break;
	}
	case hise::ShapeFX::Tanh:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			l[i] = std::tanhf(l[i]);
			r[i] = std::tanhf(r[i]);
		}

		break;
	}
	case hise::ShapeFX::Square:
		break;
	case hise::ShapeFX::SquareRoot:
		break;
	case hise::ShapeFX::Curve:
	{
		for (int i = 0; i < numOversampled; i++)
		{
			l[i] = getCurveValue(l[i]);
			r[i] = getCurveValue(r[i]);
		}

		break;
	}
	case hise::ShapeFX::Function:
	case hise::ShapeFX::numModes:
	default:
		break;
	}
	

	for (int i = 0; i < numOversampled; i++)
	{
		l[i] -= biasLeft;
		r[i] -= biasRight;
	}

	oversampler->processSamplesDown(block);

	if (autogain)
	{
		lgain_inv.applyGain(b.getWritePointer(0, startSample), numSamples);
		rgain_inv.applyGain(b.getWritePointer(1, startSample), numSamples);
	}
}

float ShapeFX::getCurveValue(float input)
{
	auto v = abs((double)input);
	auto sign = (0.f < input) - (input < 0.0f);
	return (float)sign * table->getInterpolatedValue(v * 512.0);
}

}