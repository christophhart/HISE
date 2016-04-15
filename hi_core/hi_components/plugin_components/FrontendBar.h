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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef __FRONTENDBAR_H_INCLUDED
#define __FRONTENDBAR_H_INCLUDED


class PresetComboBox : public ComboBox
{
public:

	enum Offset
	{
		FactoryOffset = 0,
		UserPresetOffset = 8192
	};

	void addItemsToMenu(PopupMenu& m) const override
	{
		m.setLookAndFeel(&plaf);

		m.addSectionHeader("Factory Presets");
		m.addSeparator();
		for (int i = 0; i < factoryPresets.size(); i++)
		{
			m.addItem(factoryPresets[i].id, factoryPresets[i].name, true, factoryPresets[i].id == getSelectedId());
		}

		m.addSectionHeader("User Presets");
		m.addSeparator();
		for (int i = 0; i < userPresets.size(); i++)
		{
			m.addItem(userPresets[i].id, userPresets[i].name, true, userPresets[i].id == getSelectedId());
		}
	}

	/** This adds a factory preset. The ID will be internally adjusted.*/
	void addFactoryPreset(const String &name, int id)
	{
		factoryPresets.add(Entry(name, id + FactoryOffset));

		addItem(name, id + FactoryOffset);
	}

	void addUserPreset(const String &name, int id)
	{
		userPresets.add(Entry(name, id + UserPresetOffset));

		addItem(name, id + UserPresetOffset);
	}



	void clearPresets()
	{
		clear(dontSendNotification);
		factoryPresets.clear();
		userPresets.clear();
	}

	bool isFactoryPresetSelected() const { return getSelectedId() < UserPresetOffset; }

private:

	struct Entry
	{
		Entry(String name_, int id_) :
			name(name_),
			id(id_)
		{};

		Entry() : name(""), id(-1) {};

		String name;
		int id;
	};

	mutable PopupLookAndFeel plaf;

	Array<Entry> factoryPresets;
	Array<Entry> userPresets;
};


/** The bar that is displayed for every FrontendProcessorEditor */
class FrontendBar  : public Component,
                     public Timer,
                     public ButtonListener,
					 public SliderListener,
					 public SettableTooltipClient,
                     public ComboBox::Listener
{
public:
    
	// ================================================================================================================

    FrontendBar (MainController *p);
    ~FrontendBar();

	void buttonClicked(Button *b) override;
	void sliderValueChanged(Slider* slider) override;
    void comboBoxChanged(ComboBox *cb) override;
	
	void refreshPresetFileList();

	// ================================================================================================================

	void timerCallback();
    void paint (Graphics& g);
    void resized();

private:

	// ================================================================================================================

	MainController *mc;

	FrontendKnobLookAndFeel fklaf;

	ScopedPointer<Label> volumeSliderLabel;
	ScopedPointer<Label> pitchSliderLabel;
	ScopedPointer<Label> balanceSliderLabel;

	ScopedPointer<Slider> volumeSlider;
	ScopedPointer<Slider> balanceSlider;
	ScopedPointer<Slider> pitchSlider;

	ScopedPointer<VoiceCpuBpmComponent> voiceCpuComponent;

	UpdateMerger cpuUpdater;
	ScopedPointer<TooltipBar> tooltipBar;

	ScopedPointer<PresetComboBox> presetSelector;
	ScopedPointer<ShapeButton> presetSaveButton;

	ScopedPointer<VuMeter> outMeter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrontendBar)
};


#endif  
