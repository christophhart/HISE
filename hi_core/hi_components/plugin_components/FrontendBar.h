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

	void showPopup()
	{
		PopupMenu m;

		addItemsToMenu(m);

		const int result = m.show();

		setSelectedId(result, sendNotification);
	}

	void addItemsToMenu(PopupMenu& m) const override
	{
		m.setLookAndFeel(&plaf);
		
		m.addSectionHeader("Factory Presets");
		
		PopupMenu sub1;

		sub1.addItem(12334, "tut");
		sub1.addItem(23623, "TUT2");
		m.addSubMenu("Menu", sub1);

		for (int i = 0; i < unsortedFactoryPresets.size(); i++)
		{
			m.addItem(unsortedFactoryPresets[i].id,
				      unsortedFactoryPresets[i].name, 
					  true, 
					  unsortedFactoryPresets[i].id == getSelectedId());
		}

		for (int i = 0; i < factoryPresetCategories.size(); i++)
		{
			PopupMenu sub;

			for (int j = 0; j < factoryPresetCategories[i]->presets.size(); j++)
			{
				Entry *e = &factoryPresetCategories[i]->presets[j];

				sub.addItem(e->id, e->name, true, e->id == getSelectedId());
			}

			m.addSubMenu(factoryPresetCategories[i]->name, sub);
		}
		m.addSeparator();
		m.addSectionHeader("User Presets");
		m.addSeparator();
		for (int i = 0; i < userPresets.size(); i++)
		{
			m.addItem(userPresets[i].id, userPresets[i].name, true, userPresets[i].id == getSelectedId());
		}
	}

	/** This adds a factory preset. The ID will be internally adjusted.*/
	void addFactoryPreset(const String &name, const String &category, int id)
	{
		if (category.isEmpty())
		{
			unsortedFactoryPresets.add(Entry(name, id + FactoryOffset));
		}
		else
		{
			int index = -1;

			for (int i = 0; i < factoryPresetCategories.size(); i++)
			{
				if (factoryPresetCategories[i]->name == category)
				{
					index = i;
					break;
				}
			}

			if (index == -1)
			{
				PresetCategory *newCategory = new PresetCategory(category);
				newCategory->presets.add(Entry(name, id + FactoryOffset));
				factoryPresetCategories.add(newCategory);
			}
			else
			{
				factoryPresetCategories[index]->presets.add(Entry(name, id + FactoryOffset));
			}
		}
		

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
		factoryPresetCategories.clear();
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

	struct PresetCategory
	{
		PresetCategory(String name_):
			name(name_)
		{};

		PresetCategory() : name("") {};

		String name;
		Array<Entry> presets;
	};

	mutable PopupLookAndFeel plaf;

	Array<Entry> unsortedFactoryPresets;
	OwnedArray<PresetCategory> factoryPresetCategories;
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

	void setProperties(DynamicObject *p);
	static String createJSONString(DynamicObject *p=nullptr);
	static DynamicObject *createDefaultProperties();

	// ================================================================================================================

	void timerCallback();
    void paint (Graphics& g);
    void resized();

	bool isOverlaying() const { return overlaying; }

private:

	const Image *getFilmStripImageFromString(const String &fileReference) const;

	// ================================================================================================================

	
	MainController *mc;

	bool overlaying;

	int height;

	Colour bgColour;

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
