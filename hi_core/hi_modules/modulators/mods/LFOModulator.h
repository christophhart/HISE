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

#ifndef LFOMODULATOR_H_INCLUDED
#define LFOMODULATOR_H_INCLUDED

namespace hise { using namespace juce;


struct WaveformLookupTables
{
	static void init();

	static bool initialised;

	static float sineTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float triangleTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float sawTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float squareTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float randomTable[SAMPLE_LOOKUP_TABLE_SIZE];


};

#define LFO_DOWNSAMPLING_FACTOR 32

/** A LFO Modulator modulates the signal with a low frequency
*
*	@ingroup modulatorTypes
*
*	It is not polyphonic, so every voice gets treated the same.
*
*/
class LfoModulator: public TimeVariantModulator,
					public TempoListener,
					public ProcessorWithStaticExternalData,
					public WaveformComponent::Broadcaster
{
public:

	SET_PROCESSOR_NAME("LFO", "LFO Modulator", "A LFO Modulator modulates the signal with a low frequency")

	LfoModulator(MainController *mc, const String &id, Modulation::Mode m);

	~LfoModulator();

	enum Waveform
	{
		Sine = 1,
		Triangle,
		Saw,
		Square,
		Random,
		Custom,
		Steps,
		numWaveforms
	};

	/** Special Parameters for the LfoModulator. */
	enum Parameters
	{
		Frequency = 0, ///< the modulation frequency.
		FadeIn, ///< a fade in time after each note on
		WaveFormType, ///< the waveform for the oscillator
		Legato, ///< if enabled multiple keys are pressed, it will not retrigger the LFO
		TempoSync, ///< enable sync to Host Tempo
		SmoothingTime, ///< smoothes hard edges of the oscillator
		NumSteps,
		LoopEnabled, ///< enables loop mode for the LFO
		PhaseOffset, ///< the initial phase of the LFO
		SyncToMasterClock, ///< sync the LFO to the master clock
        IgnoreNoteOn, ///< does not reset the LFO phase on incoming notes (free run mode)
		numParameters
	};

	enum InternalChains
	{
		IntensityChain = 0,
		FrequencyChain,
		numInternalChains
	};

	enum EditorStates
	{
		IntensityChainShown = Processor::numEditorStates,
		FrequencyChainShown,
		numEditorStates
	};

    void referenceShared(ExternalData::DataType, int) override
    {
        data = getSliderPackDataUnchecked(0);
        customTable = getTableUnchecked(0);
        customTable->setXTextConverter(Modulation::getDomainAsMidiRange);
    }
    
	int getNumInternalChains() const override {return numInternalChains;};

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	int getNumChildProcessors() const override
	{
		return numInternalChains;
	};

	Processor *getChildProcessor(int i) override
	{
		switch(i)
		{
		case IntensityChain:	return intensityChain;
		case FrequencyChain:	return frequencyChain;
		default:				jassertfalse; return nullptr;
		}
	};

	const Processor *getChildProcessor(int i) const override
	{
		switch(i)
		{
		case IntensityChain:	return intensityChain;
		case FrequencyChain:	return frequencyChain;
		default:				jassertfalse; return nullptr;
		}
	};

	/** Returns a new ControlEditor */
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	float getDefaultValue(int parameterIndex) const override; 
	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;

	void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler=dontSendNotification) noexcept override;

	/** Updates the tempo. */
	void tempoChanged(double /*newTempo*/) override
	{
		calcAngleDelta();
		debugToConsole(this, "NewTempo");
	};

	/** Ignores midi for now. */
	void handleHiseEvent(const HiseEvent &m) override;

	/** sets up the smoothing filter. */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void calculateBlock(int startSample, int numSamples) override;;

    void onTransportChange(bool isPlaying, double ppqPosition) override
    {
        if(isPlaying)
            resyncInternal(ppqPosition);
    }
    
    void onResync(double ppqPosition) override
    {
        resyncInternal(ppqPosition);
    }
    
    
private:

    void resyncInternal(double ppqPosition);
    
	/** Calculates the oscillator value of the LFO
	*	Don't use this for GUI stuff, since it advances the LFO
	*/
	float calculateNewValue ();

	void setCurrentWaveform() 
	{
		switch(currentWaveform)
		{
		case Sine:		currentTable = WaveformLookupTables::sineTable; break;
		case Triangle:	currentTable = WaveformLookupTables::triangleTable; break;
		case Saw:		currentTable = WaveformLookupTables::sawTable; break;
		case Square:	currentTable = WaveformLookupTables::squareTable; break;
		case Random:	currentTable = nullptr; break;
		case Custom:	currentTable = getTableUnchecked(0)->getReadPointer(); break;
		default:		currentTable = WaveformLookupTables::sineTable; break;
		}

		triggerWaveformUpdate();
	}



	void setTargetRatioA(float targetRatio) 
	{
		if (targetRatio < 0.0000001f)
			targetRatio = 0.0000001f;  // -180 dB
		targetRatioA = targetRatio;
		attackBase = (1.0f + targetRatioA) * (1.0f - attackCoef);
	}

	float calcCoef(float rate, float targetRatio) const
	{
		const float factor = (float)getControlRate() * 0.001f;

		rate = jmax<float>(0.000001f, rate * factor);

		float returnValue = expf(-logf((1.0f + targetRatio) / targetRatio) / rate);

		return returnValue;
	}

	void setAttackRate(float rate) 
	{
		attack = rate;

		if (rate != 0.0f)
		{
			attackCoef = calcCoef(attack, targetRatioA);
			attackBase = (1.0f + targetRatioA) * (1.0f - attackCoef);
		}
		else
		{
			attackCoef = 0.0f;
			attackBase = 1.0f;
		}

		

	}

	void setFadeInTime(float newFadeInTime)
	{
		if(newFadeInTime != attack)
		{
			setAttackRate(newFadeInTime);
		}
	}

	void resetFadeIn() noexcept
	{
		attackValue = 0.0f;
	}

    void resetPhase();
    
	void calcAngleDelta();;
    
    bool tempoSync = false;

	ModulatorChain::Collection modChains;


	//AudioSampleBuffer intensityBuffer;

	//AudioSampleBuffer frequencyBuffer;

	float voiceIntensityValue;

	float intensityModulationValue;

	SampleLookupTable* customTable;

	SliderPackData* data;

	int currentSliderIndex = 0;
	float currentSliderValue = 0.0f;
	int lastSwapIndex = 0;

	float currentRandomValue;

    bool ignoreNoteOn = false;
    
	float const *currentTable;

	Ramper intensityInterpolator;

	ExecutionLimiter<DummyCriticalSection> frequencyUpdater;
	ExecutionLimiter<DummyCriticalSection> valueUpdater;

	float rampValue = 1.0f;

	float frequencyModulationValue;
    
	float frequency;

	float currentValue;
	float loopEndValue = -1.0f;

	double phaseOffset = 0.0f;

	double angleDelta;

	double uptime;

	bool run;

	juce::Random randomGenerator;

	UpdateMerger inputMerger;

	float attack;
	float attackCoef;
	float attackBase;
	float targetRatioA;
	float attackValue;

	ModulatorChain* intensityChain;
	ModulatorChain* frequencyChain;

	Waveform currentWaveform;

	int keysPressed;

	Smoother smoother;
	float smoothingTime;

	bool loopEnabled;
	bool legato;

    

	int lastCycleIndex = 0;
	int lastIndex = 0;

	TempoSyncer::Tempo currentTempo;
	
	heap<float> stepData;

	bool syncToMasterClock = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(LfoModulator);
};



} // namespace hise

#endif  // LFOMODULATOR_H_INCLUDED
