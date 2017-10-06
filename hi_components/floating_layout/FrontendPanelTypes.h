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

#ifndef FRONTENDPANELTYPES_H_INCLUDED
#define FRONTENDPANELTYPES_H_INCLUDED

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

	Image on;
	Image off;
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
"BlackKeyRatio": 0.7
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
	public ComponentWithKeyboard
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
		numProperyIds
	};

	SET_PANEL_NAME("Keyboard");

	MidiKeyboardPanel(FloatingTile* parent);
	~MidiKeyboardPanel();

	bool showTitleInPresentationMode() const override;
	CustomKeyboard* getKeyboard() const override;

	int getNumDefaultableProperties() const override;
	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void paint(Graphics& g) override;
	void resized() override;

	int getFixedHeight() const override;

private:

	bool defaultAppearance = true;
	ScopedPointer<CustomKeyboard> keyboard;
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

	SET_PANEL_NAME("PresetBrowser");

	PresetBrowserPanel(FloatingTile* parent);

	~PresetBrowserPanel();

	void fromDynamicObject(const var& object) override;
	bool showTitleInPresentationMode() const override;
	void resized() override;

private:

	ScopedPointer<MultiColumnPresetBrowser> presetBrowser;
};

/** Type-ID: `TooltipPanel`
*
*	Shows a descriptive text whenever the mouse hovers over a widget with a tooltip.
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

#endif  // FRONTENDPANELTYPES_H_INCLUDED
