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
				public LookupTableProcessor,
				public JavascriptProcessor,
				public ProcessorWithScriptingContent
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

	struct ShapeFunctions
	{
		static float linear(float input) { return input; };
		static float atan(float input) { return std::atanf(input); };
		static float tanh(float input) { return std::tanh(input); };
		static float square(float input) 
		{ 
			auto sign = (0.f < input) - (input < 0.0f);

			return jlimit<float>(-1.0f, 1.0f, (float)sign * input * input); 
		};
		static float squareRoot(float input)
		{ 
			auto v = fabsf(input);
			auto sign = (0.f < input) - (input < 0.0f);

			return (float)sign * sqrtf(v); 
		};
		static float symetricLookup(const float* table, float normalizedIndex)
		{
			auto sign = (0.f < normalizedIndex) - (normalizedIndex < 0.0f);
			auto v = jlimit<float>(0.0f, 1.0f, fabsf(normalizedIndex)) * ((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f);
			
			const float i1 = floor(v);
			const float i2 = jmin<float>((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f, i1 + 1.0f);
			
			const float delta = v - i1;

			return (float)sign * Interpolator::interpolateLinear(table[(int)i1], table[(int)i2], delta);
		}

		static float asymetricLookup(const float* table, float normalizedIndex)
		{
			auto v = jlimit<float>(0.0, 511.0f, (normalizedIndex + 1.0f) / 2.0f * (float)SAMPLE_LOOKUP_TABLE_SIZE);

			const float i1 = floor(v);
			const float i2 = jmin<float>((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f, i1 + 1.0f);

			const float delta = v - i1;

			return Interpolator::interpolateLinear(table[(int)i1], table[(int)i2], delta);
		}
	};

	enum SpecialParameters
	{
		BiasLeft = MasterEffectProcessor::SpecialParameters::numParameters,
		BiasRight,
		HighPass,
		LowPass,
		Mode,
		Oversampling,
		Gain,
		Reduce,
		Autogain,
		LimitInput,
		Drive,
		Mix,
		numParameters
	};

	

	SET_PROCESSOR_NAME("ShapeFX", "Shape FX")

	ShapeFX(MainController *mc, const String &uid);;

	~ShapeFX();;

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

	void postCompileCallback() override
	{
		debugToConsole(this, "Update shape function");
		updateMode();
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	SnippetDocument *getSnippet(int /*c*/) { return functionCode; }
	const SnippetDocument *getSnippet(int /*c*/) const { return functionCode; }
	int getNumSnippets() const { return 1; }

	void registerApiClasses() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	Rectangle<float> getPeakValues() const { return { inPeakValueL, inPeakValueR, outPeakValueL, outPeakValueR }; }

	void updateOversampling()
	{
		ScopedLock sl(oversamplerLock);

		auto factor = roundDoubleToInt(log2((double)oversampleFactor));

		oversampler = new Oversampler(2, factor, Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

		if(getBlockSize() > 0)
			oversampler->initProcessing(getBlockSize());
	}

	void updateFilter(bool updateLowPass);

	void updateMode();

	void updateGain();

	void getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue) override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples);

	virtual int getControlCallbackIndex() const { return 0; };

	private:

	struct TableUpdater : public SafeChangeListener
	{
		TableUpdater(ShapeFX& parent_) :
			parent(parent_)
		{
			parent.getTable(0)->addChangeListener(this);
		};

		~TableUpdater()
		{
			parent.getTable(0)->removeChangeListener(this);
		}

		void changeListenerCallback(SafeChangeBroadcaster *b) override
		{
			parent.updateMode();
		}

		ShapeFX& parent;
	};

	float getCurveValue(float input);

	CriticalSection oversamplerLock;

	ScopedPointer<Oversampler> oversampler;
	
	ShapeMode mode;

	bool autogain;

	float biasLeft, biasRight, drive, lowpass, highpass, reduce, mix, gain;

	int oversampleFactor = 1;

	float displayTable[SAMPLE_LOOKUP_TABLE_SIZE];

	LinearSmoothedValue<float> lgain;
	LinearSmoothedValue<float> rgain;

	LinearSmoothedValue<float> lgain_inv;
	LinearSmoothedValue<float> rgain_inv;

	LinearSmoothedValue<float> mixSmootherL;
	LinearSmoothedValue<float> mixSmoother_invL;

	LinearSmoothedValue<float> mixSmootherR;
	LinearSmoothedValue<float> mixSmoother_invR;

	AudioSampleBuffer dryBuffer;

	float inPeakValueL = 0.0f;
	float inPeakValueR = 0.0f;
	float outPeakValueL = 0.0f;
	float outPeakValueR = 0.0f;

	IIRFilter lHighPass;
	IIRFilter rHighPass;
	
	IIRFilter lLowPass;
	IIRFilter rLowPass;

	IIRFilter lDcRemover;
	IIRFilter rDcRemover;

	bool limitInput;

	chunkware_simple::SimpleLimit limiter;

	ScopedPointer<SampleLookupTable> table;

	ScopedPointer<TableUpdater> tableUpdater;

	ScopedPointer<SnippetDocument> functionCode;

	void updateMix();
};


}

#endif