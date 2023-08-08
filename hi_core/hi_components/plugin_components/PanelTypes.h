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


#ifndef PANELTYPES_H_INCLUDED
#define PANELTYPES_H_INCLUDED

namespace hise { using namespace juce;

/** Type-ID: `CustomSettings`

A settings dialogue with a customable fields.

![CustomSettings Screenshot](http://hise.audio/images/floating_tile_gifs/CustomSettings.gif)

### Used base properties

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour of the description labels
`Font`					  | the font
`FontSize`				  | the font size

### Example JSON

```
const var data = {
"Type": "CustomSettings",
"StyleData": {
},
"Font": "Tahoma",
"FontSize": 19,
"ColourData": {
"textColour": "0xFFFF8888"
},
"Driver": true,
"Device": true,
"Output": true,
"BufferSize": true,
"SampleRate": true,
"GlobalBPM": true,
"StreamingMode": false,
"GraphicRendering": false,
"ScaleFactor": true,
"SustainCC": false,
"ClearMidiCC": true,
"SampleLocation": true,
"DebugMode": true,
"ScaleFactorList": [0.5, 1, 2]
};
```

*/
class CustomSettingsWindowPanel : public FloatingTileContent,
	public Component
{
public:

	enum class SpecialPanelIds
	{
		Driver = 7, ///< The audio driver type (ASIO, WDM, CoreAudio)
		Device, ///< the audio device type (which sound card / driver)
		Output, ///< the audio output channel for multichannel audio devices
		BufferSize, ///< the buffer size (ideally only power of twos)
		SampleRate, ///< the supported sample rates
		GlobalBPM, ///< the global BPM in standalone apps (replaces the sync to host tempo)
		ScaleFactor, ///< a global UI scale factor that is applied to the interface
		GraphicRendering, ///< switch between software / Open GL rendering
		StreamingMode, ///< allows to double the preload size for old hard disks
		SustainCC, ///< the CC number for the sustain pedal (useful if you want to remap this function)
		VoiceAmount, ///< the voice limit per sound generator.
		ClearMidiCC, ///< a button that clears all MIDI CC mappings
		SampleLocation, ///< shows the location of the sample folder and a button to relocate
		DebugMode, ///< enables Debug mode which creates a useful log file for bug chasing
		ScaleFactorList, ///< an array containing all available zoom factors (eg. `[0.5, 1.5, 1.25]` for 50%, 100%, 125%)
		numProperties
	};

	CustomSettingsWindowPanel(FloatingTile* parent);

	SET_PANEL_NAME("CustomSettings");

	bool showTitleInPresentationMode() const override { return false; }

	void resized();;

	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

private:

	ScopedPointer<LookAndFeel> slaf;
	ScopedPointer<Viewport> viewport;
	ScopedPointer<CustomSettingsWindow> window;
};


/** Type-ID: `MidiChannelList`

A list with all MIDI channels that can be enabled or disabled.

![MidiChannelList Screenshot](http://hise.audio/images/floating_tile_gifs/MidiChannelList.gif)

### Used base properties

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour

### Example JSON

```
const var data = {
"Type": "MidiChannelList",

"ColourData": {
"textColour": "0xFFaa4444"
}
};
```

*/
class MidiChannelPanel : public FloatingTileContent,
	public Component,
	public ToggleButtonList::Listener
{
public:

	MidiChannelPanel(FloatingTile* parent);

	SET_PANEL_NAME("MidiChannelList");

	void resized() override;

	void periodicCheckCallback(ToggleButtonList* /*list*/) override;
	void toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value) override;
	
	ToggleButtonList* getList() { return channelList; };

private:

	ScopedPointer<Viewport> viewport;
	ScopedPointer<ToggleButtonList> channelList;
	ScopedPointer<LookAndFeel> slaf;
};

/** Type-ID: `MidiSources`

A list with all available MIDI devices that can be enabled / disabled (similar to the MidiChannelPanel).

### Used base properties

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour

### Example JSON

```
const var data = {
"Type": "MidiSources",

"ColourData": {
"textColour": "0xFFaa4444"
}
};
```

*/
class MidiSourcePanel : public FloatingTileContent,
	public Component,
	public ToggleButtonList::Listener
{
public:

	MidiSourcePanel(FloatingTile* parent);

	SET_PANEL_NAME("MidiSources");

	bool showTitleInPresentationMode() const override;

	void resized() override;

	void periodicCheckCallback(ToggleButtonList* list) override;
	void toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value) override;

	ToggleButtonList* getList() { return midiInputList; };

private:

    ScopedPointer<LookAndFeel> slaf;
	ScopedPointer<Viewport> viewport;
	ScopedPointer<ToggleButtonList> midiInputList;

	int numMidiDevices = 0;
};

} // namespace hise

#endif  // PANELTYPES_H_INCLUDED
