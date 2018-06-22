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

#ifndef MPE_MODULATORS_H_INCLUDED
#define MPE_MODULATORS_H_INCLUDED

namespace hise {
using namespace juce;




class MPEModulator : public EnvelopeModulator,
					 public LookupTableProcessor,
					 public MidiControllerAutomationHandler::MPEData::Listener
{
public:

	SET_PROCESSOR_NAME("MPEModulator", "MPE Modulator");

	enum Gesture
	{
		Press = 1,
		Slide,
		Glide,
		Stroke,
		Lift,
		numGestures
	};

	enum SpecialParameters
	{
		GestureCC = EnvelopeModulator::Parameters::numParameters,
		SmoothingTime,
		DefaultValue,
		SmoothedIntensity,
		numTotalParameters
	};

	MPEModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~MPEModulator();

	void mpeModeChanged(bool isEnabled) override;
	void mpeModulatorAssigned(MPEModulator* m, bool wasAssigned) override;
	void mpeDataReloaded() override;

	void setInternalAttribute(int parameter_index, float newValue) override;
	float getDefaultValue(int parameterIndex) const override;
	float getAttribute(int parameter_index) const;

	int getNumTables() const override { return 1; }
	Table* getTable(int /*index*/) const override { return table; }
	
	void resetToDefault();
	
	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int) override { return nullptr; };
	const Processor *getChildProcessor(int) const override { return nullptr; };

	void startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;
	void handleHiseEvent(const HiseEvent& m) override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	/** The container for the envelope state. */
	struct MPEState : public EnvelopeModulator::ModulatorState
	{
	public:

		MPEState(int voiceIndex) :
			ModulatorState(voiceIndex)
		{
			smoother.setDefaultValue(0.0f);
		};

		int midiChannel = -1;
		bool isPressed = false;
		bool isRingingOff = false;

		void startVoice(float initialValue, float targetValue_)
		{
			const bool useSmoother = smoother.getSmoothingTime() > 0.0f;

			smoother.setDefaultValue(initialValue);

			currentRampValue = useSmoother ? initialValue : targetValue_;
			currentRampTarget = -1.0f;
			currentRampDelta = -1.0f;
			targetValue = targetValue_;
			blockDivider.reset();
		}

		void stopVoice()
		{
			isPressed = false;
			isRingingOff = targetValue == 0.0f;
		}

		void reset()
		{
			targetValue = -1.0f;
			isPressed = false;
		}

		bool isPlaying() const noexcept
		{
			if (isRingingOff && currentRampValue == 0)
				return false;

			return true;
		}

		void setTargetValue(float newTargetValue)
		{
			targetValue = newTargetValue;
		}

		void process(float* data, int numSamples);
		
		void setSmoothingTime(float smoothingTime)
		{
			smoother.setSmoothingTime(smoothingTime);
		}

		void prepareToPlay(double sampleRate)
		{
			smoother.prepareToPlay(sampleRate);
		}

	private:

		Smoother smoother;
		BlockDivider<HISE_EVENT_RASTER> blockDivider;

		float targetValue = 0.0f;
		
		float currentRampValue = 0.0f;
		float currentRampTarget = 0.0f;
		float currentRampDelta = 0.0f;
		
		int numCalls = 0;
		int sumCalls = 0;
	};

	ModulatorState *createSubclassedState(int voiceIndex) const override { return new MPEState(voiceIndex); };

private:

	void updateSmoothingTime(float newTime);
	MPEState * getState(int voiceIndex);
	const MPEState * getState(int voiceIndex) const;

	struct MPEValues
	{
		MPEValues()
		{
			reset();
		}

		void reset();

		float storeAndGetMaxValue(Gesture g, int channel, float value);

		float pressValues[16];
		float strokeValues[16];
		float slideValues[16];
		float glideValues[16];
		float liftValues[16];
	};

	MPEValues mpeValues;
	MPEState monoState;

	bool isActive = true;
	int monophonicVoiceCounter = 0;

	int midiChannelForMonophonicMode = 1;
	

	UnorderedStack<MPEState*> activeStates;

	int unsavedChannel = -1;
	float unsavedStrokeValue = 0.0f;
	float defaultValue = 0.0f;
	float smoothingTime = -1.0f;

	int ccNumber = 0;
	Gesture g;
	float smoothedIntensity;

	ScopedPointer<SampleLookupTable> table;

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPEModulator);

	JUCE_DECLARE_WEAK_REFERENCEABLE(MPEModulator);
};








}

#endif