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

// Set this to 1 to enable the shape FX module to be a workbench for finding cool waveshaping functions
// This is disabled by default to prevent glitches with it being a script processor when it doesn't need to be in 99% of all cases
#ifndef HI_USE_SHAPE_FX_SCRIPTING
#define HI_USE_SHAPE_FX_SCRIPTING 0
#endif


#ifndef HI_ENABLE_SHAPE_FX_OVERSAMPLER
#define HI_ENABLE_SHAPE_FX_OVERSAMPLER 1
#endif
    
class LowpassSmoothedValue
{
public:

	void processBlock(float* l, float* r, int numSamples)
	{
		for (int i = 0; i < numSamples; i++)
		{
			const float smoothedValue = s.smooth(targetValue);
			l[i] *= smoothedValue;
			r[i] *= smoothedValue;
		}
	}

	void prepareToPlay(double sampleRate, double smoothTimeSeconds)
	{
		s.prepareToPlay(sampleRate);
		s.setSmoothingTime((float)smoothTimeSeconds * 1000.0f);
	};

	void setTargetValue(float newTargetValue)
	{
		FloatSanitizers::sanitizeFloatNumber(newTargetValue);
		targetValue = newTargetValue;
	}

	Smoother s;
	float targetValue = 1.0f;
};

/** A general purpose waveshaper effect. 
	@ingroup effectTypes.
*/
class ShapeFX : public MasterEffectProcessor,
				public LookupTableProcessor,
				public WaveformComponent::Broadcaster
{
public:

	using Oversampler = juce::dsp::Oversampling<float>;
    
	using ShapeFunction = std::function<float(float)>;

	enum ShapeMode
	{
		Linear=1,
		Atan,
		Tanh,
		Sin,
		Asinh,
		Saturate,
		Square,
		SquareRoot,
		TanCos,
		Chebichev1,
		Chebichev2,
		Chebichev3,
		Curve=32,
		AsymetricalCurve,
		numModes
	};

	struct ShapeFunctions;
	

	class ShaperBase
	{
	public:

		ShaperBase() {};

	public:

		virtual ~ShaperBase() {};

		virtual void processBlock(float* l, float* r, int numSamples) = 0;
		
		virtual float getSingleValue(float input) = 0;
	};

	template <class ShapeFunction> class FuncShaper : public ShaperBase
	{
	public:

		FuncShaper(): ShaperBase() {};

		void processBlock(float* l, float* r, int numSamples) override
		{
			for (int i = 0; i < numSamples; i++)
			{
				l[i] = ShapeFunction::shape(l[i]);
				r[i] = ShapeFunction::shape(r[i]);
			}
		}

		float getSingleValue(float input) { return ShapeFunction::shape(input); };
	};

	class InternalSaturator;
	class TableShaper;
	
	class CachedScriptShaper;
	class ScriptShaper;

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
		BypassFilters,
		numParameters
	};

	SET_PROCESSOR_NAME("ShapeFX", "Shape FX", "A general purpose waveshaper effect. ");

	ShapeFX(MainController *mc, const String &uid);;

	~ShapeFX();;

	void setInternalAttribute(int parameterIndex, float newValue) override;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

#if HI_USE_SHAPE_FX_SCRIPTING
	int getNumScriptParameters() const override { return numParameters; }

	void postCompileCallback() override
	{
		debugToConsole(this, "Update shape function");
		updateMode();
	}

	SnippetDocument *getSnippet(int /*c*/) { return functionCode; }
	const SnippetDocument *getSnippet(int /*c*/) const { return functionCode; }
	int getNumSnippets() const { return 1; }

	void registerApiClasses() override;
#endif

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override
	{
		// Don't use the content controls for the parameters here...
		return Processor::getIdentifierForParameterIndex(parameterIndex);
	}

	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

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

	Rectangle<float> getPeakValues() const { return { inPeakValueL, inPeakValueR, outPeakValueL, outPeakValueR }; }

	void updateOversampling();
	

	void updateFilter(bool updateLowPass);

	void updateMode();

	void updateGain();

	void updateGainSmoothers();

	void getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue) override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples);

	virtual int getControlCallbackIndex() const { return 0; };

	const StringArray& getShapeNames() const { return shapeNames; };
	static void generateRampForDisplayValue(float* data, float gainToUse);
	
	

private:
	
	

	TableShaper * getTableShaper();
	const TableShaper * getTableShaper() const;

	void processBitcrushedValues(float* l, float* r, int numSamples);


	OwnedArray<ShaperBase> shapers;
	StringArray shapeNames;

	void initShapers();

	struct TableUpdater : public Table::Listener
	{
		TableUpdater(ShapeFX& parent_) :
			parent(parent_)
		{
			parent.getTable(0)->addRulerListener(this);
		};

		~TableUpdater()
		{
			parent.getTable(0)->removeRulerListener(this);
		}

		void graphHasChanged(int index) override
		{
			parent.recalculateDisplayTable();
		}

		ShapeFX& parent;
	};

	void recalculateDisplayTable();

	SpinLock oversamplerLock;
	ScopedPointer<Oversampler> oversampler;
	
	ShapeMode mode;

	bool autogain;

	float biasLeft, biasRight, drive, lowpass, highpass, reduce, mix, gain, autogainValue;

	bool bypassFilters = false;

	int oversampleFactor = 1;

	float displayTable[SAMPLE_LOOKUP_TABLE_SIZE];
	float unusedTable[SAMPLE_LOOKUP_TABLE_SIZE];
	
	float graphNormalizeValue = 0.0f;

	DelayLine<1024> lDelay;
	DelayLine<1024> rDelay;

	LowpassSmoothedValue gainer;
	LowpassSmoothedValue autogainer;

	LinearSmoothedValue<float> mixSmootherL;
	LinearSmoothedValue<float> mixSmoother_invL;

	LinearSmoothedValue<float> mixSmootherR;
	LinearSmoothedValue<float> mixSmoother_invR;
	LinearSmoothedValue<float> bitCrushSmoother;

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

	ScopedPointer<TableUpdater> tableUpdater;

	void updateMix();
};


/** A polyphonic wave shaper.
*	@ingroup effectTypes
*
*/
class PolyshapeFX : public VoiceEffectProcessor,
					//public LookupTableProcessor,
					public ProcessorWithStaticExternalData,
					public WaveformComponent::Broadcaster
{
public:

	class PolytableShaper;
	class PolytableAsymetricalShaper;

	SET_PROCESSOR_NAME("PolyshapeFX", "Polyshape FX", "A polyphonic wave shaper.");

	enum InternalChains
	{
		DriveModulation = 0,
		numInternalChains
	};

	/** The parameters */
	enum SpecialParameters
	{
		Drive = VoiceEffectProcessor::numParameters,
		Mode,
		Oversampling,
		Bias,
		numParameters
	};

	enum EditorStates
	{
		DriveModulationShown = Processor::numEditorStates,
		numEditorStates
	};

	PolyshapeFX(MainController *mc, const String &uid, int numVoices);;

	~PolyshapeFX();

	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void renderNextBlock(AudioSampleBuffer &/*buffer*/, int /*startSample*/, int /*numSamples*/) override
	{

	}

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return modChains[DriveModulation].getChain(); };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return modChains[DriveModulation].getChain(); };
	int getNumChildProcessors() const override { return numInternalChains; };
	int getNumInternalChains() const override { return numInternalChains; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

	const StringArray& getShapeNames() const { return shapeNames; }

	void getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue) override;

	void recalculateDisplayTable();

	void startVoice(int voiceIndex, const HiseEvent& e) override;

private:

	struct PolyUpdater : public Timer
	{
		PolyUpdater(PolyshapeFX& parent_) :
			parent(parent_)
		{
			startTimer(50);
		};

		~PolyUpdater()
		{
			stopTimer();
		}

		void timerCallback() override
		{
			parent.triggerWaveformUpdate();
		}

		PolyshapeFX& parent;
	};

	void updateSmoothedGainers();

	class TableUpdater : public Table::Listener
	{
	public:
		TableUpdater(PolyshapeFX& parent_) :
			parent(parent_)
		{
			parent.getTable(0)->addRulerListener(this);
			parent.getTable(1)->addRulerListener(this);
		}
 
		void graphHasChanged(int point) override
		{
			parent.recalculateDisplayTable();
		}

		~TableUpdater()
		{
			parent.getTable(0)->removeRulerListener(this);
			parent.getTable(1)->removeRulerListener(this);
		}

		PolyshapeFX& parent;
	};

	void initShapers();

	PolyUpdater polyUpdater;

	StringArray shapeNames;

	OwnedArray<ShapeFX::ShaperBase> shapers;
	OwnedArray<ShapeFX::Oversampler> oversamplers;
	float drive = 1.0f;

	LinearSmoothedValue<float> driveSmoothers[NUM_POLYPHONIC_VOICES];

	int mode = ShapeFX::ShapeMode::Linear;
	bool oversampling = false;

	FixedVoiceAmountArray<SimpleOnePole> dcRemovers;

	ScopedPointer<TableUpdater> tableUpdater;

	float displayPeak = 0.0f;

	

	float bias = 0.0f;

	float displayTable[SAMPLE_LOOKUP_TABLE_SIZE];
	float unusedTable[SAMPLE_LOOKUP_TABLE_SIZE];
};


}

#endif
