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
		targetValue = FloatSanitizers::sanitizeFloatNumber(newTargetValue);
	}

	Smoother s;
	float targetValue = 1.0f;
};

/** A general purpose waveshaper effect. */
class ShapeFX : public MasterEffectProcessor,
				public WaveformComponent::Broadcaster,
				public LookupTableProcessor,
				public JavascriptProcessor,
				public ProcessorWithScriptingContent
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
		CachedScript,
		Script,
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
		numParameters
	};

	

	SET_PROCESSOR_NAME("ShapeFX", "Shape FX")

	ShapeFX(MainController *mc, const String &uid);;

	~ShapeFX();;

	void setInternalAttribute(int parameterIndex, float newValue) override;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;
	int getNumScriptParameters() const override { return numParameters; }

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	Table* getTable(int /*tableIndex*/) const override;
	
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

		void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
		{
			parent.recalculateDisplayTable();
		}

		ShapeFX& parent;
	};

	void rebuildScriptedTable();

	void recalculateDisplayTable();

	

	

	CriticalSection oversamplerLock;

	SpinLock scriptLock;

	Result shapeResult;

	ScopedPointer<Oversampler> oversampler;
	
	ShapeMode mode;

	bool autogain;

	float biasLeft, biasRight, drive, lowpass, highpass, reduce, mix, gain, autogainValue;

	int oversampleFactor = 1;

	float displayTable[SAMPLE_LOOKUP_TABLE_SIZE];
	float unusedTable[SAMPLE_LOOKUP_TABLE_SIZE];
	
	float graphNormalizeValue = 0.0f;

	DelayLine lDelay;
	DelayLine rDelay;

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

	

	ScopedPointer<SafeChangeBroadcaster> tableBroadcaster;

	ScopedPointer<TableUpdater> tableUpdater;

	ScopedPointer<SnippetDocument> functionCode;

	void updateMix();
};


/** A polyphonic waveshaper
*	@ingroup effectTypes
*
*/
class PolyshapeFX : public VoiceEffectProcessor,
					public WaveformComponent::Broadcaster,
					public LookupTableProcessor
{
public:

	class PolytableShaper;
	class PolytableAsymetricalShaper;

	SET_PROCESSOR_NAME("PolyshapeFX", "Polyshape FX");

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

	Processor *getChildProcessor(int /*processorIndex*/) override { return driveChain; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return driveChain; };
	int getNumChildProcessors() const override { return numInternalChains; };
	int getNumInternalChains() const override { return numInternalChains; };

	Table* getTable(int tableIndex) const override;

	int getNumTables() const override { return 2; }

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	AudioSampleBuffer &getBufferForChain(int /*index*/) override;

	void preVoiceRendering(int voiceIndex, int startSample, int numSamples);

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

	const StringArray& getShapeNames() const { return shapeNames; }

	void getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue) override;

	void recalculateDisplayTable();

private:

	class TableUpdater : public SafeChangeListener
	{
	public:
		TableUpdater(PolyshapeFX& parent_) :
			parent(parent_)
		{
			parent.getTable(0)->addChangeListener(this);
			parent.getTable(1)->addChangeListener(this);
		}

		void changeListenerCallback(SafeChangeBroadcaster*) override
		{
			parent.recalculateDisplayTable();
		}

		~TableUpdater()
		{
			parent.getTable(0)->removeChangeListener(this);
			parent.getTable(1)->removeChangeListener(this);
		}

		PolyshapeFX& parent;
	};

	void initShapers();

	StringArray shapeNames;

	OwnedArray<ShapeFX::ShaperBase> shapers;
	OwnedArray<ShapeFX::Oversampler> oversamplers;
	float drive = 1.0f;
	int mode = ShapeFX::ShapeMode::Linear;
	bool oversampling = false;
	ScopedPointer<ModulatorChain> driveChain;
	AudioSampleBuffer driveBuffer;

	OwnedArray<SimpleOnePole> dcRemovers;

	ScopedPointer<TableUpdater> tableUpdater;

	float bias = 0.0f;

	float displayTable[SAMPLE_LOOKUP_TABLE_SIZE];
	float unusedTable[SAMPLE_LOOKUP_TABLE_SIZE];
};


}

#endif