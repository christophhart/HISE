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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
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

private:

	//ToolbarButtonLookAndFeel tblaf;

	OwnedArray<ToggleButton> buttons;

	Listener* listener;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButtonList);
};



class CustomSettingsWindow : public Component,
	public ComboBox::Listener,
	public Button::Listener
{
public:

	CustomSettingsWindow(MainController* mc_);

	~CustomSettingsWindow();

	void rebuildMenus(bool rebuildDeviceTypes, bool rebuildDevices);

	void buttonClicked(Button* /*b*/) override;

	static void flipEnablement(AudioDeviceManager* manager, const int row);

	static String getNameForChannelPair(const String& name1, const String& name2);

	static StringArray getChannelPairs(AudioIODevice* currentDevice);

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void paint(Graphics& g) override;

	void resized() override;

	void enableScaleFactorSelector(bool shouldBeEnabled)
	{
		if (shouldBeEnabled)
		{
			scaleFactorSelector->setVisible(true);
			setSize(getWidth(), getHeight() + 40);
			resized();
		}
		else
		{
			scaleFactorSelector->setVisible(false);
			setSize(getWidth(), getHeight() - 40);
			resized();
		}
	}

private:

	PopupLookAndFeel plaf;
	BlackTextButtonLookAndFeel blaf;

	MainController* mc;

	ScopedPointer<ComboBox> deviceSelector;
	ScopedPointer<ComboBox> soundCardSelector;
	ScopedPointer<ComboBox> outputSelector;
	ScopedPointer<ComboBox> bufferSelector;
	ScopedPointer<ComboBox> sampleRateSelector;
	ScopedPointer<ComboBox> diskModeSelector;
	ScopedPointer<ComboBox> scaleFactorSelector;
	ScopedPointer<ComboBox> ccSustainSelector;
	ScopedPointer<TextButton> clearMidiLearn;
	ScopedPointer<TextButton> relocateButton;
	ScopedPointer<TextButton> debugButton;
	
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
