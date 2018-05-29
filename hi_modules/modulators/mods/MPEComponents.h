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

#ifndef MPE_COMPONENTS_H_INCLUDED
#define MPE_COMPONENTS_H_INCLUDED

namespace hise {
using namespace juce;


class MPEPanel : public FloatingTileContent,
	public Component,
	public MidiControllerAutomationHandler::MPEData::Listener,
	public ButtonListener
{
public:

	enum Properties
	{
		ShowTable = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		ShowPlotte
	};

	struct LookAndFeel : public PopupLookAndFeel
	{
		LookAndFeel();

		Font getFont() const
		{
			return font;
		}

		Font getComboBoxFont(ComboBox &) override
		{
			return font;
		}

		void positionComboBoxText(ComboBox &c, Label &labelToPosition) override;
		void drawComboBox(Graphics &g, int width, int height, bool isButtonDown, int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, ComboBox &c) override;
		void drawButtonBackground(Graphics& g, Button&b, const Colour& /*backgroundColour*/, bool isMouseOverButton, bool /*isButtonDown*/);
		void drawButtonText(Graphics& g, TextButton& b, bool /*isMouseOverButton*/, bool /*isButtonDown*/);
		void drawLinearSlider(Graphics &g, int /*x*/, int /*y*/, int width,
			int height, float /*sliderPos*/, float /*minSliderPos*/,
			float /*maxSliderPos*/, const Slider::SliderStyle, Slider &s) override;

		Font font;
		Colour bgColour;
		Colour textColour;
		Colour fillColour;
		Colour lineColour;
	};

	class Factory : public PathFactory
	{
	public:

		Path createPath(const String& id) const override;
	};

	SET_PANEL_NAME("MPEPanel");

	MPEPanel(FloatingTile* parent);;

	~MPEPanel()
	{
		getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
	}

	void updateTableColours();

	void fromDynamicObject(const var& object) override;

	bool keyPressed(const KeyPress& key) override;

	void resized() override;

	void paint(Graphics& g) override;

	void setCurrentMod(MPEModulator* newMod);

	void mpeModeChanged(bool isEnabled) override;
	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override;
	void mpeDataReloaded() override;
	void mpeModulatorAmountChanged() override;

	void buttonClicked(Button* b) override;



private:

	struct Notifier : public Timer
	{
		Notifier(MPEPanel& parent_);

		void timerCallback() override;

		void refresh()
		{
			refreshPanel = true;
		}

	private:

		MPEPanel & parent;
		bool refreshPanel = false;
	};

	class Model : public ListBoxModel
	{
	private:

		struct LastRow : public Component,
			public ButtonListener
		{
			LastRow(MPEPanel& parent_);
			~LastRow();

			void resized() override;
			void buttonClicked(Button*) override;

			MPEPanel& parent;
			TextButton addButton;
		};

		class Row : public Component,
			public ButtonListener,
			public SafeChangeListener,
			public ComboBoxListener,
			public Timer
		{
		public:
			Row(MPEModulator* mod_, LookAndFeel& laf_);

			~Row();

			void resized() override;
			void timerCallback() override;
			void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override;
			void paint(Graphics& g) override;
			void buttonClicked(Button* b) override;
			void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

			MPEModulator* getMod() { return mod; }
			const MPEModulator* getMod() const { return mod; }

			void deleteThisRow();

		private:

			void updateEnableState();

			WeakReference<MPEModulator> mod;
			TableEditor curvePreview;
			HiComboBox selector;

			ShapeButton deleteButton;

			ComboBox modeSelector;

			HiSlider smoothingTime;
			HiSlider defaultValue;
			HiSlider intensity;
			Slider output;

			MidiControllerAutomationHandler::MPEData& data;
			LookAndFeel& laf;
		};

	public:
		Model(MPEPanel& parent_);;

		int getNumRows() override;
		void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;
		Component* refreshComponentForRow(int rowNumber, bool /*isRowSelected*/, Component* existingComponentToUpdate) override;
		void deleteKeyPressed(int lastRowSelected) override;
		void listBoxItemClicked(int row, const MouseEvent& e) override;

	private:



		MPEPanel & parent;
		MidiControllerAutomationHandler::MPEData& data;
	};

	void updateRectangles();

	LookAndFeel laf;

	WeakReference<MPEModulator> currentlyEditedMod;

	Notifier notifier;
	Model m;

	

	ListBox listbox;
	TextButton enableMPEButton;

	ScopedPointer<MarkdownHelpButton> helpButton;

	TableEditor currentTable;
	ScopedPointer<Plotter> currentPlotter;

	Rectangle<int> topBar;
	Rectangle<int> tableHeader;
	Rectangle<int> topArea;
	Rectangle<int> bottomArea;
	Rectangle<int> bottomBar;
};



class MPEKeyboard : public Component,
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

	MPEKeyboard(MidiKeyboardState& state_);

	~MPEKeyboard();

	void timerCallback() override;

	void handleNoteOn(MidiKeyboardState* /*source*/,
		int midiChannel, int midiNoteNumber, float velocity);

	void handleNoteOff(MidiKeyboardState* /*source*/,
		int midiChannel, int midiNoteNumber, float velocity);

	void handleMessage(const MidiMessage& m) override;

	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& event) override;

	void paint(Graphics& g) override;

	Rectangle<float> getPositionForNote(int noteNumber) const;

	int getNoteForPosition(Point<int> pos) const
	{
		return lowKey + int((float)pos.getX() / getWidthForNote());
	}

	float getWidthForNote() const { return (float)getWidth() / 24.0f; }

	void buttonClicked(Button* b) override;

	bool isMPEKeyboard() const override { return true; }

	bool isUsingCustomGraphics() const noexcept override { return false; };
	void setUseCustomGraphics(bool /*shouldUseCustomGraphics*/) override {};

	void setShowOctaveNumber(bool /*shouldDisplayOctaveNumber*/) override { }
	bool isShowingOctaveNumbers() const override { return false; }

	void setLowestKeyBase(int lowKey_) override { lowKey = lowKey_; }

	float getKeyWidthBase() const override { return getWidthForNote(); };
	void setKeyWidthBase(float /*w*/) override {  }

	int getRangeStartBase() const override { return lowKey; };
	int getRangeEndBase() const override { return lowKey + 24; };

	int getMidiChannelBase() const override { return -1; }
	void setMidiChannelBase(int /*newChannel*/) override {  }

	void setRangeBase(int min, int /*max*/) override { lowKey = min; }


	void setBlackNoteLengthProportionBase(float /*ratio*/) override {  }
	double getBlackNoteLengthProportionBase() const override { return 0.5; }

	bool isToggleModeEnabled() const override { return false; };
	void setEnableToggleMode(bool /*shouldBeEnabled*/) override {  }

private:

	hise::LockfreeQueue<MidiMessage> pendingMessages;

	struct Note
	{
		static Note fromMouseEvent(const MPEKeyboard& p, const MouseEvent& e, int channelIndex);

		static Note fromMidiMessage(const MPEKeyboard& p, const MidiMessage& m);

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

		void updateNote(const MPEKeyboard& p, const MidiMessage& m);

		void updateNote(const MPEKeyboard& p, const MouseEvent& e);

		void draw(const MPEKeyboard& p, Graphics& g) const;

		bool isVisible(const MPEKeyboard& p) const
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