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

/** A Lfo Modulator modulates the signal with a low frequency
*
*	@ingroup modulatorTypes
*
*	It is not polyphonic, so every voice gets treated the same.
*
*/
class LfoModulator: public TimeVariantModulator,
					public TempoListener,
					public LookupTableProcessor,
					public SliderPackProcessor
{
public:

	SET_PROCESSOR_NAME("LFO", "LFO Modulator")

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

	int getNumInternalChains() const override {return numInternalChains;};

	void restoreFromValueTree(const ValueTree &v) override
	{
		TimeVariantModulator::restoreFromValueTree(v);

		loadAttribute(TempoSync, "TempoSync");

		loadAttribute(Frequency, "Frequency");
		loadAttribute(FadeIn, "FadeIn");
		loadAttribute(WaveFormType, "WaveformType");
		loadAttribute(Legato, "Legato");
		
		loadAttribute(SmoothingTime, "SmoothingTime");

		loadTable(customTable, "CustomWaveform");

		data->fromBase64(v.getProperty("StepData"));
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = TimeVariantModulator::exportAsValueTree();

		saveAttribute(Frequency, "Frequency");
		saveAttribute(FadeIn, "FadeIn");
		saveAttribute(WaveFormType, "WaveformType");
		saveAttribute(Legato, "Legato");
		saveAttribute(TempoSync, "TempoSync");
		saveAttribute(SmoothingTime, "SmoothingTime");
		
		saveTable(customTable, "CustomWaveform");
		
		v.setProperty("StepData", data->toBase64(), nullptr);

		return v;
	}

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

	SliderPackData *getSliderPackData(int /*index*/) override { return data; };

	const SliderPackData *getSliderPackData(int /*index*/) const override { return data; };

	/** Returns a new ControlEditor */
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;


	float getDefaultValue(int parameterIndex) const override; 
	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	/** Updates the tempo. */
	void tempoChanged(double /*newTempo*/) override
	{
		calcAngleDelta();
		debugToConsole(this, "NewTempo");
	};

	/** Ignores midi for now. */
	void handleHiseEvent(const HiseEvent &m) override;

	/** sets up the smoothing filter. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void calculateBlock(int startSample, int numSamples) override
	{
#if ENABLE_ALL_PEAK_METERS
		if(--numSamples >= 0)
		{
			const float value = calculateNewValue();
			internalBuffer.setSample(0, startSample, value);
			++startSample;
			setOutputValue(value); 
		}
#endif

		while(--numSamples >= 0)
		{
			internalBuffer.setSample(0, startSample, calculateNewValue());
			++startSample;
		}

		const float newInputValue = ((int)(uptime) % SAMPLE_LOOKUP_TABLE_SIZE) / (float)SAMPLE_LOOKUP_TABLE_SIZE;

		if (inputMerger.shouldUpdate() && currentWaveform == Custom) sendTableIndexChangeMessage(false, customTable, newInputValue);
	};

	/** This overwrites the TimeModulation callback to render the intensity chain. */
	virtual void applyTimeModulation(AudioSampleBuffer &b, int startSamples, int numSamples) override;


	/** Returns the modulated intensity value. */
	virtual float getIntensity() const noexcept
	{
		switch(getMode())
		{
			case Modulation::GainMode:		return intensityModulationValue * Modulation::getIntensity();
			case Modulation::PitchMode:		return (Modulation::getIntensity() - 1.0f)*intensityModulationValue + 1.0f;
			default:						jassertfalse; return -1.0f;
		}		
	};

	/** Returns the table that is used for the LFO waveform. */
	Table *getTable(int=0) const override 
	{
		return customTable;
	};

private:

	/** Calculates the oscillator value of the LFO
	*	Don't use this for GUI stuff, since it advances the LFO
	*/
	float calculateNewValue ();

	void setCurrentWaveform() 
	{
		switch(currentWaveform)
		{
		case Sine:		currentTable = sineTable; break;
		case Triangle:	currentTable = triangleTable; break;
		case Saw:		currentTable = sawTable; break;
		case Square:	currentTable = squareTable; break;
		case Random:	currentTable = nullptr; break;
		case Custom:	currentTable = customTable->getReadPointer(); break;
		default:		currentTable = sineTable; break;
			

		}

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
		const float factor = (float)getSampleRate() * 0.001f;

		rate *= factor;

		float returnValue = expf(-logf((1.0f + targetRatio) / targetRatio) / rate);

		jassert(returnValue > 0.0f);

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

	void calcAngleDelta()
	{
		const double sr = getSampleRate();

		const float frequencyToUse = tempoSync ? TempoSyncer::getTempoInHertz(getMainController()->getBpm(), currentTempo):
												 frequency;

		const float cyclesPerSecond = frequencyToUse * frequencyModulationValue;
        const double cyclesPerSample = (double)cyclesPerSecond / sr;

		angleDelta = cyclesPerSample * (double)SAMPLE_LOOKUP_TABLE_SIZE;
	};

	static void initSampleTables()
	{
		const float max = (float)SAMPLE_LOOKUP_TABLE_SIZE;
		const float half = SAMPLE_LOOKUP_TABLE_SIZE / 2;

		for(int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
		{
			sineTable[i] = 0.5f *cosf(i * float_Pi / half) + 0.5f;

			triangleTable[i] = i >= half ? (float)(2.0 * i) / max -1.0f:
										  (float)( - 2.0 * i) / max + 1.0f;

			sawTable[i] = (float)(1.0f * i) / max;

			squareTable[i] = i >= half ? 0.0f : 1.0f;
		}
	}

	AudioSampleBuffer intensityBuffer;

	AudioSampleBuffer frequencyBuffer;

	

	float voiceIntensityValue;

	float intensityModulationValue;

	static float sineTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float triangleTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float sawTable[SAMPLE_LOOKUP_TABLE_SIZE];
	static float squareTable[SAMPLE_LOOKUP_TABLE_SIZE];
	ScopedPointer<SampleLookupTable> customTable;

	ScopedPointer<SliderPackData> data;

	int currentSliderIndex = 0;
	float currentSliderValue = 0.0f;
	int lastSwapIndex = 0;

	float currentRandomValue;

	float const *currentTable;

	Ramper intensityInterpolator;

	UpdateMerger frequencyUpdater;

	float frequencyModulationValue;

	float frequency;

	float currentValue;

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

	ScopedPointer<ModulatorChain> intensityChain;
	ScopedPointer<ModulatorChain> frequencyChain;

	Waveform currentWaveform;

	int keysPressed;

	Smoother smoother;
	float smoothingTime;


	bool legato;

	bool tempoSync;

	TempoSyncer::Tempo currentTempo;
};



} // namespace hise

#endif  // LFOMODULATOR_H_INCLUDED
