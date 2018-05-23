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

class MPEPanel : public Component,
				 public MidiKeyboardStateListener,
				 public Timer,
				 public ButtonListener,
				 public KeyboardBase
{
public:

	enum ColourIds
	{
		bgColour,
		waveColour,
		keyOnColour,
		dragColour,
		numColoursIds
	};

	MPEPanel(MidiKeyboardState& state_):
		state(state_),
		pendingMessages(1024)
	{
		state.addListener(this);
		startTimer(30);

		setColour(bgColour, Colours::black);
		setColour(waveColour, Colours::white.withAlpha(0.5f));
		setColour(keyOnColour, Colours::white);
		setColour(dragColour, Colour(SIGNAL_COLOUR));
	}

	~MPEPanel()
	{
		state.removeListener(this);
	}

	void timerCallback() override;

	void handleNoteOn(MidiKeyboardState* /*source*/,
		int midiChannel, int midiNoteNumber, float velocity);

	void handleNoteOff(MidiKeyboardState* /*source*/,
		int midiChannel, int midiNoteNumber, float velocity);

	void handleMessage(const MidiMessage& m) override;

	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& event) override
	{
		for (auto& s : pressedNotes)
			s.updateNote(*this, event);

		repaint();
	}

	void paint(Graphics& g) override;

	Rectangle<float> getPositionForNote(int noteNumber) const;

	int getNoteForPosition(Point<int> pos) const
	{
		return lowKey + int((float)pos.getX() / getWidthForNote());
	}

	float getWidthForNote() const
	{
		return (float)getWidth() / 24.0f;
	}

	void buttonClicked(Button* b) override
	{
		if (b == &octaveUp)
		{
			lowKey = jmin<int>(108, lowKey + 12);
		}
		else
		{
			lowKey = jmax<int>(0, lowKey - 12);
		}

		repaint();
	}

	bool isMPEKeyboard() const override { return true; }

	bool isUsingCustomGraphics() const noexcept override { return false; };
	void setUseCustomGraphics(bool shouldUseCustomGraphics) override {};

	void setShowOctaveNumber(bool shouldDisplayOctaveNumber) override { }
	bool isShowingOctaveNumbers() const override { return false; }

	void setLowestKeyBase(int lowKey_) override { lowKey = lowKey_; }

	int getKeyWidthBase() const override { return (int)getWidthForNote(); };
	void setKeyWidthBase(float w) override {  }

	int getRangeStartBase() const override { return lowKey; };
	int getRangeEndBase() const override { return lowKey+24; };

	int getMidiChannelBase() const override { return -1; }
	void setMidiChannelBase(int newChannel) override {  }

	void setRangeBase(int min, int max) override { lowKey = min; }


	void setBlackNoteLengthProportionBase(float ratio) override {  }
	double getBlackNoteLengthProportionBase() const override { return 0.5; }

	bool isToggleModeEnabled() const override { return false; };
	void setEnableToggleMode(bool /*shouldBeEnabled*/) override {  }

private:

	hise::LockfreeQueue<MidiMessage> pendingMessages;

	struct Note
	{
		static Note fromMouseEvent(const MPEPanel& p, const MouseEvent& e, int channelIndex);

		static Note fromMidiMessage(const MPEPanel& p, const MidiMessage& m);

		bool operator ==(const Note& other)  const
		{
			return assignedMidiChannel == other.assignedMidiChannel;
		}

		bool operator !=(const Note& other)  const
		{
			return assignedMidiChannel != other.assignedMidiChannel;
		}

		bool operator ==(const MidiMessage& m) const
		{
			return assignedMidiChannel == m.getChannel();
		}

		bool operator ==(const MouseEvent& e) const
		{
			return fingerIndex == e.source.getIndex();
		}

		bool operator !=(const MidiMessage& m) const
		{
			return assignedMidiChannel != m.getChannel();
		}

		bool operator !=(const MouseEvent& e) const
		{
			return fingerIndex != e.source.getIndex();
		}

		void updateNote(const MPEPanel& p, const MidiMessage& m);

		void updateNote(const MPEPanel& p, const MouseEvent& e);

		void draw(const MPEPanel& p, Graphics& g) const;

		bool isVisible(const MPEPanel& p) const
		{
			return !p.getPositionForNote(noteNumber).isEmpty();
		}

		void sendNoteOn(MidiKeyboardState& state) const
		{
			state.noteOn(assignedMidiChannel, noteNumber, (float)strokeValue / 127.0f);
		}

		void sendNoteOff(MidiKeyboardState& state) const
		{
			state.noteOff(assignedMidiChannel, noteNumber, (float)liftValue / 127.0f);
		}

	private:

		bool isArtificial;
		int fingerIndex;
		int assignedMidiChannel;
		int noteNumber;

		int glideValue;
		int slideValue;
		int strokeValue;
		int liftValue;
		int pressureValue;
		Point<int> startPoint;
		Point<int> dragPoint;

	};

	TextButton octaveUp;
	TextButton octaveDown;

	UnorderedStack<Note> pressedNotes;
	int nextChannelIndex = 1;
	MidiKeyboardState& state;
	int lowKey = 36;
};



}

#endif