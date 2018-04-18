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

#ifndef SHAPEFX_H_INCLUDED
#define SHAPEFX_H_INCLUDED

namespace hise {
using namespace juce;


/** A general purpose waveshaper effect. */
class ShapeFX : public MasterEffectProcessor,
				public WaveformComponent::Broadcaster,
				public LookupTableProcessor
{
public:

	using Oversampler = juce::dsp::Oversampling<float>;


	enum ShapeMode
	{
		Linear=1,
		Atan,
		Tanh,
		Square,
		SquareRoot,
		Curve,
		Function,
		numModes
	};

	enum SpecialParameters
	{
		BiasLeft = MasterEffectProcessor::SpecialParameters::numParameters,
		BiasRight,
		HighPass,
		Mode,
		Oversampling,
		Gain,
		Reduce,
		Autogain,
		Drive,
		Mix,
		numParameters
	};

	SET_PROCESSOR_NAME("ShapeFX", "Shape FX")

	ShapeFX(MainController *mc, const String &uid);;

	~ShapeFX()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	Table* getTable(int /*tableIndex*/) const override { return table; }
	
	int getNumTables() const override { return 1; }

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return nullptr;
	};

	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return nullptr;
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	void updateOversampling()
	{
		ScopedLock sl(oversamplerLock);

		auto factor = roundDoubleToInt(log2((double)oversampleFactor));

		oversampler = new Oversampler(2, factor, Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

		if(getBlockSize() > 0)
			oversampler->initProcessing(getBlockSize());
	}

	void updateFilter();

	void updateMode();

	void updateGain();

	void getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue) override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples);

	float getCurveValue(float input);

	CriticalSection oversamplerLock;

	ScopedPointer<Oversampler> oversampler;
	
	ShapeMode mode;

	bool autogain;

	float biasLeft, biasRight, drive, highpass, reduce, mix, gain;

	int oversampleFactor = 1;

	float displayTable[SAMPLE_LOOKUP_TABLE_SIZE];

	LinearSmoothedValue<float> lgain;
	LinearSmoothedValue<float> rgain;

	LinearSmoothedValue<float> lgain_inv;
	LinearSmoothedValue<float> rgain_inv;

	IIRFilter lFilter;
	IIRFilter rFilter;
	
	ScopedPointer<SampleLookupTable> table;


};


}

#endif