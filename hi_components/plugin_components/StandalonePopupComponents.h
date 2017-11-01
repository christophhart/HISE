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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef STANDALONEPOPUPCOMPONENTS_H_INCLUDED
#define STANDALONEPOPUPCOMPONENTS_H_INCLUDED


class ToggleButtonList : public Component,
	public ButtonListener,
	public Timer
{
public:
	class Listener
	{
	public:

		virtual ~Listener() {};

		virtual void toggleButtonWasClicked(ToggleButtonList* list, int index, bool value) = 0;

		virtual void periodicCheckCallback(ToggleButtonList* list) = 0;
	};

	ToggleButtonList(StringArray& names, Listener* listener_);

	void rebuildList(const StringArray &names);

	void buttonClicked(Button* b) override;
	void resized();

	void timerCallback() override { if (listener != nullptr) listener->periodicCheckCallback(this); };

	void setValue(int index, bool value, NotificationType notify = dontSendNotification);

	void setColourAndFont(Colour c, Font f)
	{
		btblaf.textColour = c;
		btblaf.f = f;
	}

private:

	//ToolbarButtonLookAndFeel tblaf;

	BlackTextButtonLookAndFeel btblaf;

	OwnedArray<ToggleButton> buttons;

	Listener* listener;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButtonList);
};


class TuningWindow : public Component,
					 public ComboBox::Listener
{
public:

	TuningWindow(MainController* mc_) :
		mc(mc_)
	{
		addAndMakeVisible(transposeSelector = new ComboBox("Transpose"));
		addAndMakeVisible(microTuningSelector = new ComboBox("MicroTuning"));

		transposeSelector->addListener(this);
		microTuningSelector->addListener(this);

		transposeSelector->setLookAndFeel(&plaf);
		microTuningSelector->setLookAndFeel(&plaf);

		for (int i = -12; i < 13; i++)
		{
			transposeSelector->addItem(String(-i) + " semitones", i + 13);
		}

		transposeSelector->setSelectedId(getIndexForTransposeValue(dynamic_cast<GlobalSettingManager*>(mc)->transposeValue), dontSendNotification);

		microTuningSelector->addItem("+25 cent", 1);
		microTuningSelector->addItem("+20 cent", 2);
		microTuningSelector->addItem("+15 cent", 3);
		microTuningSelector->addItem("+10 cent", 4);
		microTuningSelector->addItem("+5 cent", 5);
		microTuningSelector->addItem("0 cent", 6);
		microTuningSelector->addItem("-5 cent", 7);
		microTuningSelector->addItem("-10 cent", 8);
		microTuningSelector->addItem("-15 cent", 9);
		microTuningSelector->addItem("-20 cent", 10);
		microTuningSelector->addItem("-25 cent", 11);

		microTuningSelector->setSelectedId(getIndexForPitchFactor(mc->getGlobalPitchFactorSemiTones()), dontSendNotification);

		setSize(250, 90);
	}

	void paint(Graphics& g) override
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);

		int y = 10;

		g.drawText("Transpose:", 0, y, getWidth() / 2 - 20, 30, Justification::centredRight);

		y += 40;

		g.drawText("Microtuning:", 0, y, getWidth() / 2 - 20, 30, Justification::centredRight);
	}

	void resized() override
	{
		int x = getWidth() / 2 - 10;
		int w = getWidth() - x - 10;

		int y = 10;

		transposeSelector->setBounds(x, y, w, 30);

		y += 40;

		microTuningSelector->setBounds(x, y, w, 30);
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		if (comboBoxThatHasChanged == transposeSelector)
		{
			int transposeValue = comboBoxThatHasChanged->getText().getIntValue();

			mc->allNotesOff();
			mc->getEventHandler().setGlobalTransposeValue(transposeValue);
			dynamic_cast<AudioProcessorDriver*>(mc)->transposeValue = transposeValue;
		}
		else if (comboBoxThatHasChanged == microTuningSelector)
		{
			int cent = microTuningSelector->getText().getIntValue();

			double semiTones = (double)cent / 100.0;

			dynamic_cast<AudioProcessorDriver*>(mc)->microTuning = semiTones;

			mc->setGlobalPitchFactor(semiTones);
		}
	}

	class Panel;

	

private:

	int getIndexForPitchFactor(double pitchFactorInSemiTones)
	{
		int cent = roundDoubleToInt(pitchFactorInSemiTones * 100.0);

		switch (cent)
		{
		case 25: return 1;
		case 20: return 2;
		case 15: return 3;
		case 10: return 4;
		case 5: return 5;
		case 0: return 6;
		case -5: return 7;
		case -10: return 8;
		case -15: return 9;
		case -20: return 10;
		case -25: return 11;
		default: return 6;
		}
	}

	int getIndexForTransposeValue(int transposeValue)
	{
		return -transposeValue + 13;
	}


	PopupLookAndFeel plaf;

	MainController* mc;

	ScopedPointer<ComboBox> transposeSelector;
	ScopedPointer<ComboBox> microTuningSelector;
};


class CustomSettingsWindow : public Component,
	public ComboBox::Listener,
	public Button::Listener
{
public:

    enum ColourIds
    {
        backgroundColour = 0xF1242,
        itemColour1,
        textColour,
        numColourIds
    };
    
	enum class Properties
	{
		Driver = 7, // sloppy, update this when the amount FloatingTileContent::Properties change...
		Device,
		Output,
		BufferSize,
		SampleRate,
		GlobalBPM,
		ScaleFactor,
		GraphicRendering,
		StreamingMode,
		SustainCC,
		ClearMidiCC,
		SampleLocation,
		DebugMode,
		ScaleFactorList,
		numProperties
	};

	CustomSettingsWindow(MainController* mc_, bool buildMenus=true);

	~CustomSettingsWindow();

	void rebuildMenus(bool rebuildDeviceTypes, bool rebuildDevices);

	void buttonClicked(Button* /*b*/) override;

	static void flipEnablement(AudioDeviceManager* manager, const int row);

	static String getNameForChannelPair(const String& name1, const String& name2);

	static StringArray getChannelPairs(AudioIODevice* currentDevice);

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void paint(Graphics& g) override;

	void resized() override;

	

	void setProperty(Properties id, bool shouldBeOn)
	{
		properties[(int)id] = shouldBeOn;
	}

	bool isOn(Properties p) const { return properties[(int)p]; }

	void refreshSizeFromProperties()
	{
		int height = 0;

		for (int i = (int)Properties::Driver; i <= (int)Properties::DebugMode; i++)
		{
			if (properties[i])
				height += 40;
		}

		if (properties[(int)Properties::SampleLocation])
			height += 40;

		setSize(320, height);
	}

	void rebuildScaleFactorList();

	void setFont(Font f)
	{
		font = f;
	}

private:

	friend class CustomSettingsWindowPanel;

    Font font;

	bool properties[(int)Properties::numProperties];
	Array<Identifier> propIds;

	Array<var> scaleFactorList;

	PopupLookAndFeel plaf;
	BlackTextButtonLookAndFeel blaf;

	MainController* mc;

	ScopedPointer<ComboBox> deviceSelector;
	ScopedPointer<ComboBox> soundCardSelector;
	ScopedPointer<ComboBox> outputSelector;
	ScopedPointer<ComboBox> bufferSelector;
	ScopedPointer<ComboBox> sampleRateSelector;
	ScopedPointer<ComboBox> bpmSelector;
	ScopedPointer<ComboBox> diskModeSelector;
	ScopedPointer<ComboBox> graphicRenderSelector;
	ScopedPointer<ComboBox> scaleFactorSelector;
	ScopedPointer<ComboBox> ccSustainSelector;
	ScopedPointer<TextButton> clearMidiLearn;
	ScopedPointer<TextButton> relocateButton;
	ScopedPointer<TextButton> debugButton;
	
    // Not the smartest solution, but works...
    bool loopProtection=false;
    
};

class CombinedSettingsWindow : public Component,
							   public ButtonListener,
							   public ToggleButtonList::Listener
	
{
public:

	CombinedSettingsWindow(MainController* mc);
	~CombinedSettingsWindow();

	void resized() override;
	void paint(Graphics &g) override;
	void toggleButtonWasClicked(ToggleButtonList* list, int index, bool value) override;
	void buttonClicked(Button* b) override;

	void periodicCheckCallback(ToggleButtonList* list) override;

private:

	KnobLookAndFeel klaf;
	

	int numMidiDevices = 0;

	MainController* mc;

	ScopedPointer<CustomSettingsWindow> settings;
	ScopedPointer < ToggleButtonList> midiSources;
	ScopedPointer<ShapeButton> closeButton;
};



#endif  // STANDALONEPOPUPCOMPONENTS_H_INCLUDED
