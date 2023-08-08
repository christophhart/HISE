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


#ifndef STANDALONEPOPUPCOMPONENTS_H_INCLUDED
#define STANDALONEPOPUPCOMPONENTS_H_INCLUDED

namespace hise { using namespace juce;

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

	ToggleButtonList(const StringArray& names, Listener* listener_);

	~ToggleButtonList()
	{
		buttons.clear();
	}

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

	

	BlackTextButtonLookAndFeel btblaf;

	OwnedArray<ToggleButton> buttons;

	Listener* listener;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButtonList);
};


/** A component that is showing application-wide settings (audio drivers etc.)
	@ingroup hise_ui

	In order to use it, create and hide the Properties you don't want to show.
*/
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
		Driver = 7, ///< The audio driver (ASIO, CoreAudio etc)
		Device, ///< the audio device (your soundcard)
		Output, ///< the output (normally a stereo channel pair)
		BufferSize, ///< the buffer size
		SampleRate, ///< the sample rate
		GlobalBPM, ///< the global BPM if you don't want to tempo sync.
		ScaleFactor, ///< the global scale factor for the UI
		UseOpenGL, ///< enables OpenGL
		StreamingMode, ///< Sets the streaming settings
		VoiceAmountMultiplier, ///< the max voice amount per sound generator
		ClearMidiCC, /// removes all MIDI learn information
		SampleLocation, /// shows the sample location
		DebugMode, /// toggles the Debug mode
		ScaleFactorList, ///< the list of scale factors as Array<var> containing doubles.
		numProperties
	};

	CustomSettingsWindow(MainController* mc_, bool buildMenus=true);

	~CustomSettingsWindow();

	void rebuildMenus(bool rebuildDeviceTypes, bool rebuildDevices);

	void buttonClicked(Button* /*b*/) override;

	static void flipEnablement(AudioDeviceManager* manager, const int row);

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void paint(Graphics& g) override;

	void resized() override;

	/** Shows / hides the given property. */
	void setProperty(Properties id, bool shouldBeOn)
	{
		properties[(int)id] = shouldBeOn;
	}

	bool isOn(Properties p) const { return properties[(int)p]; }

	void refreshSizeFromProperties();

    void rebuildScaleFactorList();

	void setFont(Font f)
	{
		font = f;
	}

private:

	GlobalHiseLookAndFeel glaf;
	friend class CustomSettingsWindowPanel;

    Font font;

	bool properties[(int)Properties::numProperties];
	Array<Identifier> propIds;

	Array<var> scaleFactorList;

	BlackTextButtonLookAndFeel blaf;

	MainController* mc;

	ScopedPointer<LookAndFeel> slaf;

	ScopedPointer<ComboBox> deviceSelector;
	ScopedPointer<ComboBox> soundCardSelector;
	ScopedPointer<ComboBox> outputSelector;
	ScopedPointer<ComboBox> bufferSelector;
	ScopedPointer<ComboBox> sampleRateSelector;
	ScopedPointer<ComboBox> bpmSelector;
	ScopedPointer<ComboBox> diskModeSelector;
	ScopedPointer<ComboBox> scaleFactorSelector;
	ScopedPointer<ComboBox> voiceAmountMultiplier;
	ScopedPointer<ComboBox> openGLSelector;
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

	GlobalHiseLookAndFeel klaf;
	

	int numMidiDevices = 0;

	MainController* mc;

	ScopedPointer<CustomSettingsWindow> settings;
	ScopedPointer < ToggleButtonList> midiSources;
	ScopedPointer<ShapeButton> closeButton;
};

} // namespace hise

#endif  // STANDALONEPOPUPCOMPONENTS_H_INCLUDED
