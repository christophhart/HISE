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
					 public LookupTableProcessor
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
		GestureCC,
		SmoothingTime,
		numTotalParameters
	};

	MPEModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~MPEModulator();

	void setInternalAttribute(int parameter_index, float newValue) override;
	float getDefaultValue(int parameterIndex) const override;
	float getAttribute(int parameter_index) const;

	int getNumTables() const override { return 1; }
	Table* getTable(int index) const override { return table; }
	
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

		Smoother smoother;
		int midiChannel = -1;
		float targetValue = 0.0f;
		bool active = false;
	};

	ModulatorState *createSubclassedState(int voiceIndex) const override { return new MPEState(voiceIndex); };



private:

	UnorderedStack<MPEState*> activeStates;

	void updateSmoothingTime(float newTime)
	{
		if (newTime != smoothingTime)
		{
			smoothingTime = newTime;

			for (int i = 0; i < states.size(); i++)
				getState(i)->smoother.setSmoothingTime(smoothingTime);
		}
	}

	int unsavedChannel = -1;
	float unsavedStrokeValue = 0.0f;

	float smoothingTime = 200.0f;

	MPEState * getState(int voiceIndex);
	const MPEState * getState(int voiceIndex) const;

	int ccNumber = 0;

	Gesture g = Press;

	ScopedPointer<SampleLookupTable> table;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPEModulator)
};



}

#endif