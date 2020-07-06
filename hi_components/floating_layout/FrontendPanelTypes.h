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

#ifndef FRONTENDPANELTYPES_H_INCLUDED
#define FRONTENDPANELTYPES_H_INCLUDED

namespace hise { using namespace juce;



/** A LED that will light up when an MIDI message was received.
*
*/
class ActivityLedPanel : public FloatingTileContent,
	public Timer,
	public Component
{
public:

	enum SpecialPanelIds
	{
		OnImage = (int)FloatingTileContent::PanelPropertyId::numPropertyIds, ///< The image for the **On** state. Use the same syntax as you when loading images via scripting: `{PROJECT_FOLDER}image.png`
		OffImage, ///< The image for the **Off** state.
		ShowMidiLabel, ///< Specifies whether the **MIDI** label should be shown.
		numSpecialPanelIds
	};

	SET_PANEL_NAME("ActivityLed");

	ActivityLedPanel(FloatingTile* parent);

	void timerCallback();

	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void paint(Graphics &g) override;
	void setOn(bool shouldBeOn);

private:

	Path midiShape;

	bool showMidiLabel = true;

	String onName;
	String offName;

	bool isOn = false;

	PooledImage on;
	PooledImage off;
};

/** The base class for every panel.
*
*	It contains some common properties that are used by the panels if adequate.
*/
class FloatingTileBase
{
public:

	enum SpecialPanelIds
	{
		Type = 0, ///< the ID of the panel that determines the actual type.
		Title, ///< can be used to show a better title in the popup
		ColourData, ///< an object that contains 5 basic colour IDs that can be used to customize the panel: `bgColour`, `textColour`, `itemColour1`, `itemColour2` and `itemColour3`
		LayoutData, ///< this object contains information about the layout within its parent container. You usually don't need to change this via scripting.
		Font, ///< if the panel is rendering text, this can be used to change the font. For changing the font-style, append ` Bold` or ` Italic` at the font name.
		FontSize ///< if the panel is rendering text, this can be used to change the font size.
	};


};

/** Type-ID: `Keyboard`
*
A virtual MIDI Keyboard that can be customized and filmstripped.

![Keyboard Screenshot](http://hise.audio/images/floating_tile_gifs/Keyboard.gif)

### Example JSON

```
const var data = {
"Type": "Keyboard",
"KeyWidth": 14,
"DisplayOctaveNumber": false,
"LowKey": 9,
"HiKey": 127,
"CustomGraphics": false,
"DefaultAppearance": true,
"BlackKeyRatio": 0.7,
"ToggleMode": false
};
```
### Using custom filmstrips

If you want to use your own filmstrips for the keyboard, you have to add your images with a fixed file name scheme
to the **Images** subfolder of your project and set `CustomGraphics` to true. The files must be put in a subfolder called **keyboard** and have these names:

```
{PROJECT_FOLDER}keyboard/up_0.png	  | the up state for every C key
{PROJECT_FOLDER}keyboard/down_0.png   | the down state for every C key
{PROJECT_FOLDER}keyboard/up_1.png	  | the up state for every C# key
{PROJECT_FOLDER}keyboard/down_1.png	  | the down state for every C# key
{PROJECT_FOLDER}[...]
{PROJECT_FOLDER}keyboard/up_11.png    | the up state for every B key
{PROJECT_FOLDER}keyboard/down_11.png  | the down state for every B key
```
*/
class MidiKeyboardPanel : public FloatingTileContent,
	public Component,
	public ComponentWithKeyboard,
	public MidiControllerAutomationHandler::MPEData::Listener
{
public:

	enum SpecialPanelIds
	{
		CustomGraphics = (int)FloatingTileContent::PanelPropertyId::numPropertyIds, ///< set to true if you want to use custom graphics for your keyboard.
		KeyWidth, ///< the width per key in logical pixels.
		LowKey, ///< the lowest visible key.
		HiKey, ///< the highest visible key.
		BlackKeyRatio, ///< the height of the black keys in proportion to the total height.
		DefaultAppearance, ///< set this to true to use the standard appearance in HISE.
		DisplayOctaveNumber, ///< set this to true to add octave numbers at each C note.
		ToggleMode, ///< if activated, then the notes will be held until clicked again
		MidiChannel, ///< which MIDI channel to use (1-16)
		MPEKeyboard,
		MPEStartChannel,
		MPEEndChannel,
		UseVectorGraphics,
		UseFlatStyle,
		numProperyIds
	};

	SET_PANEL_NAME("Keyboard");

	MidiKeyboardPanel(FloatingTile* parent);
	~MidiKeyboardPanel();

	bool showTitleInPresentationMode() const override;
	Component* getKeyboard() const override;

	void mpeModeChanged(bool isEnabled) override;

	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override {};

	void mpeDataReloaded() override {};

	int getNumDefaultableProperties() const override;
	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void paint(Graphics& g) override;
	void resized() override;

	int getFixedHeight() const override;

private:

	struct Updater : public AsyncUpdater
	{
		Updater(MidiKeyboardPanel& parent_) : parent(parent_)
		{}

		void handleAsyncUpdate() override;

		MidiKeyboardPanel& parent;
	};

	Updater updater;

	void restoreInternal(const var& data);

	var cachedData;

	bool mpeModeEnabled = false;

	bool shouldBeMpeKeyboard = false;

	bool defaultAppearance = true;
	ScopedPointer<KeyboardBase> keyboard;

	Range<int> mpeZone;
};

/** Type-ID: `Note`.
*
*	![Note Screenshot](https://github.com/christophhart/HISE/blob/master/tools/floating_tile_docs/gifs/Note.gif?raw=true)
*
*	A simple text editor that can be used to express thoughts / quick 'n dirty copy paste actions.
*
*	JSON Example:
*
```
const var data = {
"Type": "Note",
"Font": "Comic Sans MS",
"FontSize": 14,
"Text": "This is the content",
"ColourData": {
"textColour": "0xFFFF0000",
"bgColour": "0xFF222222"
}
}
```

### Used base properties

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour
`ColourData::bgColour`    | the background colour
`Font`					  | the font
`FontSize`				  | the font size
*/
class Note : public Component,
	public FloatingTileContent,
	public TextEditor::Listener
{
public:

	enum SpecialPanelIds
	{
		Text = (int)FloatingTileContent::PanelPropertyId::numPropertyIds, ///< the content of the text editor
		numSpecialPanelIds
	};
	/** Type-ID: `TypeID`

	Description

	![%TYPE% Screenshot](http://hise.audio/images/floating_tile_gifs/%TYPE%.gif)

	### Used base properties

	| ID | Description |
	| --- | --- |
	`ColourData::textColour`  | the text colour
	`ColourData::bgColour`    | the background colour
	`ColourData::itemColour1` | the first item colour
	`Font`					  | the font
	`FontSize`				  | the font size

	### Example JSON

	```
	const var data = {%EXAMPLE_JSON};
	```

	*/

	SET_PANEL_NAME("Note");

	Note(FloatingTile* p);

	void resized() override;

	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	int getNumDefaultableProperties() const override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void labelTextChanged(Label* l);
	int getFixedHeight() const override;

private:

	PopupLookAndFeel plaf;
	ScopedPointer<TextEditor> editor;
};

/**
Type-ID: `PerformanceLabel`

A simple text label that prints out system stats (voice count, CPU usage / memory usage). If you need more customization, use the scripting calls `Engine.getCpuUsage()`

![PerformanceLabel Screenshot](http://hise.audio/images/floating_tile_gifs/PerformanceLabel.gif)

### Used base properties:

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour
`ColourData::bgColour`    | the background colour
`Font`					  | the font
`FontSize`				  | the font size

### Example JSON

```
const var data = {
"Type": "PerformanceLabel",
"Font": "Oxygen Bold",
"FontSize": 18,
"ColourData": {
"textColour": "0xFFEEEEEE"
}
};
```

*/
class PerformanceLabelPanel : public Component,
	public Timer,
	public FloatingTileContent
{
public:

	PerformanceLabelPanel(FloatingTile* parent);

	SET_PANEL_NAME("PerformanceLabel");

	void timerCallback() override;
	void fromDynamicObject(const var& object) override;
	void resized() override;
	bool showTitleInPresentationMode() const override;

	void paint(Graphics& g) override
	{
		g.fillAll(findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	}

private:

	ScopedPointer<Label> statisticLabel;
};


/** Type-ID: `PresetBrowser`

The preset browser of HISE. This is a three column browser for Banks / Categories and Presets which will
load user presets for the instrument.

![PresetBrowser Screenshot](http://hise.audio/images/floating_tile_gifs/PresetBrowser.gif)

### Used base properties:

| ID | Description |
| --- | --- |
`ColourData::bgColour`    | the background colour
`ColourData::itemColour1` | the highlight colour
`Font`					  | the font used. The font size will be fixed

### Example JSON

```
const var data = {
"Type": "PresetBrowser",
"StyleData": {
},
"Font": "Consolas",

"ColourData": {
"bgColour": "0xFF222222",
"itemColour1": "0xFF555588"
}
};
```

*/
class PresetBrowserPanel : public FloatingTileContent,
	public Component
{
public:

	enum SpecialPanelIds
	{
		ShowFolderButton = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		ShowSaveButton,
		ShowNotes,
		ShowEditButtons,
		ShowFavoriteIcon,
		NumColumns,
		ColumnWidthRatio,
		numSpecialProperties
	};

	SET_PANEL_NAME("PresetBrowser");

	PresetBrowserPanel(FloatingTile* parent);
	~PresetBrowserPanel();

	int getNumDefaultableProperties() const override { return (int)numSpecialProperties; };

	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	bool showTitleInPresentationMode() const override;
	void resized() override;

	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

private:

	ScopedPointer<LookAndFeel> scriptlaf;
	PresetBrowser::Options options;

	ScopedPointer<PresetBrowser> presetBrowser;
};

/** Type-ID: `TooltipPanel`
*
*	Shows a descriptive text whenever the mouse hovers over a Component with a tooltip.
*
*	Use `Control.set("tooltip", "This is the tooltip text")` for each control that you want a tooltip for.
*	Then add this panel and it will automatically show and hide the tooltip.
*
*	![TooltipPanel Screenshot](http://hise.audio/images/floating_tile_gifs/TooltipPanel.gif)
*
*	### Used base properties:
*
*	- `ColourData::textColour`: the text colour
*	- `ColourData::bgColour`: the background colour *when something is shown*
*	- `ColourData::itemColour1`: the icon colour
*	- `Font`
*	- `FontSize`
*
*	### Example JSON
*
	```
	const var data = {
	  "Type": "TooltipPanel",
	  "Font": "Arial Italic",
	  "FontSize": 20,
	  "ColourData": {
		"bgColour": "0x22FF0000",
		"textColour": "0xFFFFFFFF",
		"itemColour1": "0xFF00FF00"
	  }
	};
	```
*
*/
class TooltipPanel : public FloatingTileContent,
					 public Component
{
public:
	SET_PANEL_NAME("TooltipPanel");

	TooltipPanel(FloatingTile* parent);
	~TooltipPanel();

	int getFixedHeight() const override;
	bool showTitleInPresentationMode() const override;
	void fromDynamicObject(const var& object) override;
	void resized() override;

private:

	String fontName;
	float fontSize = 14.0f;

	ScopedPointer<TooltipBar> tooltipBar;
};

/** Type-ID: `AboutPagePanel`
*
*	Shows a about page with some useful information regarding version, build date, licensed e-mail adress, etc.
*
*	
*	### Used base properties:
*
*	- `ColourData::textColour`: the text colour
*	- `ColourData::bgColour`: the background colour *when something is shown*
*	- `ColourData::itemColour1`: the icon colour
*	- `Font`
*	- `FontSize`
*
*	### Example JSON
*
```
const var data = {
"Type": "AboutPagePanel",
"Font": "Arial Italic",
"FontSize": 20,
"ColourData": {
"bgColour": "0x22FF0000",
"textColour": "0xFFFFFFFF",
"itemColour1": "0xFF00FF00"
}
};
```
*
*/
class AboutPagePanel : public FloatingTileContent,
						public Component
{
public:

	enum SpecialPanelIds
	{
		ShowProductName = (int)FloatingTileContent::PanelPropertyId::numPropertyIds, ///< the content of the text editor
		UseCustomImage,
		CopyrightNotice,
		ShowLicensedEmail ,
		ShowVersion,
		BuildDate,
		WebsiteURL,
		numSpecialPanelIds
	};

	SET_PANEL_NAME("AboutPagePanel");

	AboutPagePanel(FloatingTile* parent);

	~AboutPagePanel()
	{
		text.clear();
	}

	int getNumDefaultableProperties() const override { return (int)numSpecialPanelIds; };
	
	var toDynamicObject() const override;

	void fromDynamicObject(const var& object) override;
	
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void paint(Graphics& g) override;

private:

	PooledImage bgImage;

	AttributedString text;

	void rebuildText();

	String showCopyrightNotice;
	bool showLicensedEmail = true;
	bool showProductName = true;
	bool useCustomImage = false;
	bool showVersion = true;
	bool showBuildDate = true;
	String showWebsiteURL;

	String fontName;
	float fontSize = 14.0f;

	ScopedPointer<TooltipBar> tooltipBar;
};


class MidiLearnPanel : public FloatingTileContent,
					   public Component,
					   public TableListBoxModel,
					   public SafeChangeListener
{
public:

	enum ColumnId
	{
		CCNumber = 1,
		ParameterName,
		Inverted,
		Minimum,
		Maximum,
		numColumns,
		columnWidthRatio
	};

	MidiLearnPanel(FloatingTile* parent) :
		FloatingTileContent(parent),
		font(GLOBAL_FONT()),
		handler(*getMainController()->getMacroManager().getMidiControlAutomationHandler())
	{
		handler.addChangeListener(this);

		

		setName("MIDI Control List");

		// Create our table component and add it to this component..
		addAndMakeVisible(table);
		table.setModel(this);

		// give it a border


		setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
		setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);


		table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

		table.setOutlineThickness(0);

		laf = new TableHeaderLookAndFeel();

		

		table.getHeader().setLookAndFeel(laf);
		table.getHeader().setSize(getWidth(), 22);

		table.getViewport()->setScrollBarsShown(true, false, true, false);

		table.getHeader().setInterceptsMouseClicks(false, false);

		table.setMultipleSelectionEnabled(false);

		table.getHeader().addColumn("CC #", CCNumber, 40, 40, 40);
		table.getHeader().addColumn("Parameter", ParameterName, 70);
		table.getHeader().addColumn("Inverted", Inverted, 50, 50, 50);
		table.getHeader().addColumn("Min", Minimum, 70, 70, 70);
		table.getHeader().addColumn("Max", Maximum, 70, 70, 70);

		table.getHeader().setStretchToFitActive(true);
	}

	~MidiLearnPanel()
	{
		handler.removeChangeListener(this);
	}

	SET_PANEL_NAME("MidiLearnPanel");

	int getNumRows() override
	{
		return handler.getNumActiveConnections();
	};

	void updateContent()
	{
		table.updateContent();
	}


	void fromDynamicObject(const var& object)
	{
		FloatingTileContent::fromDynamicObject(object);

		table.setColour(ListBox::backgroundColourId, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
		
		itemColour1 = findPanelColour(FloatingTileContent::PanelColourId::itemColour1);
		itemColour2 = findPanelColour(FloatingTileContent::PanelColourId::itemColour2);
		textColour = findPanelColour(FloatingTileContent::PanelColourId::textColour);

		font = getFont();
		laf->f = font;
		laf->bgColour = itemColour1;
		laf->textColour = textColour;
	}


	void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
	{
		updateContent();
		repaint();
	}

	void paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected) override
	{
		if (rowIsSelected)
		{
			g.fillAll(Colours::white.withAlpha(0.2f));
		}
	}

	void deleteKeyPressed(int lastRowSelected) override
	{
		auto data = handler.getDataFromIndex(lastRowSelected);

		if (data.used)
		{
			handler.removeMidiControlledParameter(data.processor, data.attribute, sendNotification);
		}

		updateContent();
		repaint();
	}

	/** Tries to set the range and returns the actual value. Compare this agains newRangeValue and update your UI accordingly. */
	double setRangeValue(int row, ColumnId column, double newRangeValue)
	{
		auto data = handler.getDataFromIndex(row);

		if (data.used)
		{
			auto range = data.parameterRange;

			if (column == Minimum)
			{
				if (range.end <= newRangeValue)
				{
					return range.end;
				}
				else
				{
					range.start = newRangeValue;

					const bool ok = handler.setNewRangeForParameter(row, range);
					jassert(ok);
                    ignoreUnused(ok);

					return newRangeValue;
				}
			}
			else if (column == Maximum)
			{
				if (range.start >= newRangeValue)
				{
					return range.start;
				}
				else
				{
					range.end = newRangeValue;

					const bool ok = handler.setNewRangeForParameter(row, range);
					jassert(ok);
                    ignoreUnused(ok);

					return newRangeValue;
				}
			}

			else jassertfalse;
		}

		return -1.0 * newRangeValue;
	};

	void setInverted(int row, bool value)
	{
		auto data = handler.getDataFromIndex(row);

		if (data.used)
		{
			const bool ok = handler.setParameterInverted(row, value);
			jassert(ok);
            ignoreUnused(ok);
		}

	}

	void selectedRowsChanged(int /*lastRowSelected*/) {};

	Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
		Component* existingComponentToUpdate) override
	{
		auto data = handler.getDataFromIndex(rowNumber);

		if (columnId == Minimum || columnId == Maximum)
		{
			ValueSliderColumn* slider = dynamic_cast<ValueSliderColumn*> (existingComponentToUpdate);


			if (slider == nullptr)
				slider = new ValueSliderColumn(*this);

			

			const double value = columnId == Maximum ? data.parameterRange.end : data.parameterRange.start;

			slider->slider->setColour(Slider::ColourIds::backgroundColourId, Colours::transparentBlack);
			slider->slider->setColour(Slider::ColourIds::thumbColourId, itemColour1);
			slider->slider->setColour(Slider::ColourIds::textBoxTextColourId, textColour);

			slider->setRowAndColumn(rowNumber, (ColumnId)columnId, value, data.fullRange);

			

			return slider;
		}
		else if (columnId == Inverted)
		{
			InvertedButton* b = dynamic_cast<InvertedButton*> (existingComponentToUpdate);

			if (b == nullptr)
				b = new InvertedButton(*this);

			b->t->setColour(TextButton::buttonOnColourId, itemColour1);
			b->t->setColour(TextButton::textColourOnId, textColour);
			b->t->setColour(TextButton::buttonColourId, Colours::transparentBlack);
			b->t->setColour(TextButton::textColourOffId, textColour);

			b->setRowAndColumn(rowNumber, data.inverted);

			return b;
		}
		{
			// for any other column, just return 0, as we'll be painting these columns directly.

			jassert(existingComponentToUpdate == nullptr);
			return nullptr;
		}
	}

	void paintCell(Graphics& g, int rowNumber, int columnId,
		int width, int height, bool /*rowIsSelected*/) override
	{
		g.setColour(textColour);
		g.setFont(font);

		auto data = handler.getDataFromIndex(rowNumber);

		if (data.processor == nullptr)
		{
			return;
		}

		String text;
		
		if (columnId == ColumnId::ParameterName)
			text = ProcessorHelpers::getPrettyNameForAutomatedParameter(data.processor, data.attribute);
		else if (columnId == ColumnId::CCNumber)
			text = String(data.ccNumber);
		
		g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);

	}


	//==============================================================================
	void resized() override
	{
		table.setBounds(getLocalBounds());
	}


private:

	Colour textColour;
	Colour itemColour1;
	Colour itemColour2;

	class ValueSliderColumn : public Component,
		public SliderListener
	{
	public:
		ValueSliderColumn(MidiLearnPanel &table) :
			owner(table)
		{
			addAndMakeVisible(slider = new Slider());

			laf.setFontForAll(table.font);

			slider->setLookAndFeel(&laf);
			slider->setSliderStyle(Slider::LinearBar);
			slider->setTextBoxStyle(Slider::TextBoxLeft, true, 80, 20);
			slider->setColour(Slider::backgroundColourId, Colour(0x38ffffff));
			slider->setColour(Slider::thumbColourId, Colour(SIGNAL_COLOUR));
			slider->setColour(Slider::rotarySliderOutlineColourId, Colours::black);
			slider->setColour(Slider::textBoxOutlineColourId, Colour(0x38ffffff));
			slider->setColour(Slider::textBoxTextColourId, Colours::black);
			slider->setTextBoxIsEditable(true);

			slider->addListener(this);
		}

		void resized()
		{
			slider->setBounds(getLocalBounds().reduced(1));
		}

		void setRowAndColumn(const int newRow, ColumnId column, double value, NormalisableRange<double> range)
		{
			row = newRow;
			columnId = column;

			slider->setRange(range.start, range.end, range.interval);
			slider->setSkewFactor(range.skew);

			slider->setValue(value, dontSendNotification);
		}

		ScopedPointer<Slider> slider;

	private:

		void sliderValueChanged(Slider *) override
		{
			auto newValue = slider->getValue();

			auto actualValue = owner.setRangeValue(row, columnId, newValue);

			if (newValue != actualValue)
				slider->setValue(actualValue, dontSendNotification);
		}

	private:
		MidiLearnPanel &owner;

		HiPropertyPanelLookAndFeel laf;

		int row;
		ColumnId columnId;
		
	};

	class InvertedButton : public Component,
		public ButtonListener
	{
	public:

		InvertedButton(MidiLearnPanel &owner_) :
			owner(owner_)
		{
			laf.setFontForAll(owner_.font);

			addAndMakeVisible(t = new TextButton("Inverted"));
			t->setButtonText("Inverted");
			t->setLookAndFeel(&laf);
			t->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
			t->addListener(this);
			t->setTooltip("Invert the range of the macro control for this parameter.");
			t->setColour(TextButton::buttonColourId, Colour(0x88000000));
			t->setColour(TextButton::buttonOnColourId, Colour(0x88FFFFFF));
			t->setColour(TextButton::textColourOnId, Colour(0xaa000000));
			t->setColour(TextButton::textColourOffId, Colour(0x99ffffff));

			t->setClickingTogglesState(true);
		};

		void resized()
		{
			t->setBounds(getLocalBounds().reduced(1));
		}

		void setRowAndColumn(const int newRow, bool value)
		{
			row = newRow;

			t->setToggleState(value, dontSendNotification);
			t->setButtonText(value ? "Inverted" : "Normal");
		}

		void buttonClicked(Button *b)
		{
			t->setButtonText(b->getToggleState() ? "Inverted" : "Normal");
			owner.setInverted(row, b->getToggleState());
		};

		ScopedPointer<TextButton> t;

	private:

		MidiLearnPanel &owner;

		int row;
		ColumnId columnId;
		

		HiPropertyPanelLookAndFeel laf;


	};

	TableListBox table;     // the table component itself
	Font font;

	ScopedPointer<TableHeaderLookAndFeel> laf;

	MidiControllerAutomationHandler& handler;

	int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnPanel)
};

} // namespace hise


#endif  // FRONTENDPANELTYPES_H_INCLUDED
