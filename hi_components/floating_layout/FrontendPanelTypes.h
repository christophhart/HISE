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


class MatrixPeakMeter: public PanelWithProcessorConnection
{
public:
    
    enum ColourIds
    {
        bgColour,
        trackColour,
        peakColour,
        maxPeakColour
    };
    
    enum class SpecialProperties
    {
        SegmentLedSize = (int)SpecialPanelIds::numSpecialPanelIds,
        UseSourceChannels,
        ChannelIndexes,
        UpDecayTime,
        DownDecayTime,
        SkewFactor,
        PaddingSize,
        ShowMaxPeak,
        numSpecialProperties
    };
    
    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() {}
        
        virtual void drawMatrixPeakMeter(Graphics& g, float* peakValues, float* maxPeaks, int numChannels, bool isVertical, float segmentSize, float paddingSize, Component* c)
        {
            auto b = c->getLocalBounds().toFloat().reduced(paddingSize);
            
            g.fillAll(c->findColour(bgColour));
            
            auto w = isVertical ? b.getWidth() : b.getHeight();
            
            w -= (float)(numChannels - 1) * paddingSize;
            
            auto wPerPeak = w / numChannels;
            
            auto tc = c->findColour(trackColour);
            auto pc = c->findColour(peakColour);
            auto mc = c->findColour(maxPeakColour);
            
            RectangleList<float> onSegments, offSegments, maxSegments;
            
            for(int i = 0; i < numChannels; i++)
            {
                const float fullSize = (isVertical ? b.getHeight() : b.getWidth());
                
                auto thisTrack = isVertical ? b.removeFromLeft(wPerPeak) : b.removeFromTop(wPerPeak);
                
                if(segmentSize < 1.0f)
                {
                    auto maxCopy = thisTrack;
                    
                    g.setColour(tc);
                    g.fillRect(thisTrack);
                    g.setColour(pc);
                    
                    auto gainSize = peakValues[i] * fullSize;
                    auto thisPeak = isVertical ? thisTrack.removeFromBottom(gainSize) : thisTrack.removeFromLeft(gainSize);
                    g.fillRect(thisPeak);
                    
                    if(maxPeaks != nullptr && maxPeaks[i] > 0.0f)
                    {
                        auto maxPos = fullSize * maxPeaks[i];
                        
                        g.setColour(mc);
                        
                        auto c = isVertical ? maxCopy.removeFromBottom(maxPos).withHeight(2.0f) :
                                              maxCopy.removeFromLeft(maxPos).removeFromRight(2.0f);
                        
                        g.fillRect(c);
                    }
                }
                else
                {
                    float sw = segmentSize + paddingSize;
                    
                    int numSegments = roundToInt(fullSize / sw);
                    int peakIndex = roundToInt((float)numSegments * peakValues[i]);
                    
                    int maxPeakIndex = maxPeaks != nullptr ? roundToInt((float)numSegments * maxPeaks[i]) : -1;
                    
                    for(int j = 0; j < numSegments; j++)
                    {
                        auto segment = isVertical ? thisTrack.removeFromBottom(sw) :
                                                    thisTrack.removeFromLeft(sw);
                        
                        isVertical ? segment.removeFromTop(paddingSize) : segment.removeFromRight(paddingSize);
                        
                        if(j < peakIndex)
                            onSegments.addWithoutMerging(segment);
                        else
                            offSegments.addWithoutMerging(segment);
                        
                        if(j == maxPeakIndex)
                            maxSegments.addWithoutMerging(segment);
                    }
                }
                
                if(isVertical)
                    b.removeFromLeft(paddingSize);
                else
                    b.removeFromTop(paddingSize);
            }
            
            if(!onSegments.isEmpty() || !offSegments.isEmpty())
            {
                g.setColour(pc);
                g.fillRectList(onSegments);
                g.setColour(tc);
                g.fillRectList(offSegments);
                g.setColour(mc);
                g.fillRectList(maxSegments);
            }
        }
    };
    
    struct InternalComp: public Component,
                         public ControlledObject,
                         public PooledUIUpdater::SimpleTimer
    {
        InternalComp(MainController* mc, RoutableProcessor::MatrixData* d):
          ControlledObject(mc),
          SimpleTimer(mc->getGlobalUIUpdater()),
          data(d)
        {
            memset(currentPeaks, 0, sizeof(float) * NUM_MAX_CHANNELS);
            memset(maxPeaks, 0, sizeof(float) * NUM_MAX_CHANNELS);
            memset(maxPeakCounter, 0, sizeof(int) * NUM_MAX_CHANNELS);
            data->setEditorShown(true);
        }
        
        ~InternalComp()
        {
            if(data != nullptr)
                data->setEditorShown(false);
        }
        
        void timerCallback() override
        {
            if(!isShowing())
                return;
            
            if(data == nullptr)
                return;
            
            bool change = false;
            
            const auto numTotalChannels = getSource ? data->getNumSourceChannels() :
                                                      data->getNumDestinationChannels();
            
            auto numChannels = numTotalChannels;
            
            if(!channelIndexes.isEmpty())
                numChannels = jmin(numChannels, channelIndexes.size());
            
            change |= numChannels != lastNumChannels;
            lastNumChannels = numChannels;
            
            for(int i = 0; i < numChannels; i++)
            {
                int channelIndex = i;
                
                if(!channelIndexes.isEmpty() && isPositiveAndBelow(i, channelIndexes.size()))
                    channelIndex = jlimit(0, numTotalChannels-1, channelIndexes[i]);
                
                auto thisValue = data->getGainValue(channelIndex, getSource);
                
                thisValue = std::pow(thisValue, skewFactor);
                
                if(showMaxPeaks)
                {
                    if(thisValue > maxPeaks[i])
                    {
                        maxPeaks[i] = thisValue;
                        maxPeakCounter[i] = MaxCounter;
                        change = true;
                    }
                    else if(--maxPeakCounter[i] == 0)
                    {
                        maxPeaks[i] = 0.0f;
                        change = true;
                    }
                }
                
                auto oldValue = currentPeaks[i];
                
                change |= hmath::abs(thisValue - oldValue) > 0.001;
                currentPeaks[i] = thisValue;
            }
            
            if(change)
                repaint();
        }
        
        void paint(Graphics& g) override
        {
            if(data == nullptr || lastNumChannels == 0)
            {
                return;
            }
                
            
            auto lafToUse = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
            
            if(lafToUse == nullptr)
                lafToUse = &fallback;
            
            lafToUse->drawMatrixPeakMeter(g,
                               currentPeaks,
                               showMaxPeaks ? maxPeaks : nullptr,
                               lastNumChannels,
                               getWidth() < getHeight(),
                               segmentSize,
                               paddingSize,
                               this);
        }
        
        LookAndFeelMethods fallback;
        bool getSource = true;
        bool isVertical = false;
        
        float currentPeaks[NUM_MAX_CHANNELS];
        float maxPeaks[NUM_MAX_CHANNELS];
        int maxPeakCounter[NUM_MAX_CHANNELS];
        
        WeakReference<RoutableProcessor::MatrixData> data;
        int lastNumChannels = 0;
        
        float skewFactor = 1.0f;
        float segmentSize = 0.0f;
        float paddingSize = 1.0f;
        
        Array<int> channelIndexes;
        
        bool showMaxPeaks = false;
        
        static constexpr int MaxCounter = 40;
    };
    
    MatrixPeakMeter(FloatingTile* parent):
      PanelWithProcessorConnection(parent)
    {
        setDefaultPanelColour(PanelColourId::bgColour, Colours::black);
        setDefaultPanelColour(PanelColourId::itemColour1, Colour(0xFF555555));
        setDefaultPanelColour(PanelColourId::itemColour2, Colour(0xFF222222));
        setDefaultPanelColour(PanelColourId::textColour, Colours::white);
    }
    
    SET_PANEL_NAME("MatrixPeakMeter");
    
    void fillModuleList(StringArray& moduleList) override;
    
    var toDynamicObject() const override
    {
        auto obj = PanelWithProcessorConnection::toDynamicObject();
        
        storePropertyInObject(obj, (int)SpecialProperties::SegmentLedSize, segmentSize);
        storePropertyInObject(obj, (int)SpecialProperties::UpDecayTime, upDecayFactor);
        storePropertyInObject(obj, (int)SpecialProperties::DownDecayTime, downDecayFactor);
        storePropertyInObject(obj, (int)SpecialProperties::UseSourceChannels, useSourceChannel);
        storePropertyInObject(obj, (int)SpecialProperties::SkewFactor, skewFactor);
        storePropertyInObject(obj, (int)SpecialProperties::PaddingSize, paddingSize);
        storePropertyInObject(obj, (int)SpecialProperties::ShowMaxPeak, showMaxPeak);
        
        var ad;
        
        if(!channelIndexes.isEmpty())
        {
            Array<var> d;
            
            for(auto c: channelIndexes)
                d.add(var(c));
            
            ad = var(d);
        }
        
        storePropertyInObject(obj, (int)SpecialProperties::ChannelIndexes, ad);
        
        return obj;
    }

    void fromDynamicObject(const var& obj) override
    {
        segmentSize =  getPropertyWithDefault(obj, (int)SpecialProperties::SegmentLedSize);
        upDecayFactor = getPropertyWithDefault(obj, (int)SpecialProperties::UpDecayTime);
        downDecayFactor = getPropertyWithDefault(obj, (int)SpecialProperties::DownDecayTime);
        useSourceChannel = getPropertyWithDefault(obj, (int)SpecialProperties::UseSourceChannels);
        skewFactor = getPropertyWithDefault(obj, (int)SpecialProperties::SkewFactor);
        paddingSize = getPropertyWithDefault(obj, (int)SpecialProperties::PaddingSize);
        showMaxPeak = getPropertyWithDefault(obj, (int)SpecialProperties::ShowMaxPeak);
        
        auto cArray = getPropertyWithDefault(obj, (int)SpecialProperties::ChannelIndexes);
        
        if(cArray.isArray())
        {
            channelIndexes.clear();
            for(auto& v: *cArray.getArray())
                channelIndexes.add((int)v);
        }
        
        PanelWithProcessorConnection::fromDynamicObject(obj);
    }


    int getNumDefaultableProperties() const override
    {
        return (int)SpecialProperties::numSpecialProperties;
    }

    Identifier getDefaultablePropertyId(int index) const override
    {
        if(isPositiveAndBelow(index, (int)SpecialPanelIds::numSpecialPanelIds))
            return PanelWithProcessorConnection::getDefaultablePropertyId(index);
        
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::SegmentLedSize, "SegmentLedSize");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::UpDecayTime, "UpDecayTime");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::DownDecayTime, "DownDecayTime");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::UseSourceChannels, "UseSourceChannels");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::SkewFactor, "SkewFactor");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::PaddingSize, "PaddingSize");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::ShowMaxPeak, "ShowMaxPeak");
        RETURN_DEFAULT_PROPERTY_ID(index, (int)SpecialProperties::ChannelIndexes, "ChannelIndexes");
        
        jassertfalse;
        return {};
    }

    var getDefaultProperty(int index) const override
    {
        if(isPositiveAndBelow(index, (int)SpecialPanelIds::numSpecialPanelIds))
            return PanelWithProcessorConnection::getDefaultProperty(index);
        
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::SegmentLedSize, var(0.0f));
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::UpDecayTime, 0.0f);
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::DownDecayTime, 0.0f);
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::UseSourceChannels, false);
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::SkewFactor, 1.0f);
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::PaddingSize, 1.0f);
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::ShowMaxPeak, true);
        RETURN_DEFAULT_PROPERTY(index, (int)SpecialProperties::ChannelIndexes, var(Array<var>()));
        
        return {};
    }
    
    static float getCoefficient(double sr, float timeMs)
    {
        if(sr <= 0.0)
            return 1.0f;
        
        if(timeMs == 0.0f)
            return 1.0f;
        
        const float freq = 1000.0f / timeMs;
        auto x = expf(-2.0f * float_Pi * freq / sr);
        FloatSanitizers::sanitizeFloatNumber(x);
        
        return x;
    }
    
    Component* createContentComponent(int index) override
    {
        if(auto rp = dynamic_cast<RoutableProcessor*>(getConnectedProcessor()))
        {
            auto ni = new InternalComp(getMainController(), &rp->getMatrix());
            ni->getSource = useSourceChannel;
            
            auto sr = getConnectedProcessor()->getSampleRate();
            sr /= (double)getConnectedProcessor()->getLargestBlockSize();
            
            auto upCoeff = getCoefficient(sr, upDecayFactor);
            auto downCoeff = getCoefficient(sr, downDecayFactor);
            ni->data->setDecayCoefficients(upCoeff, downCoeff);
            
            ni->setColour(bgColour, findPanelColour(PanelColourId::bgColour));
            ni->setColour(peakColour, findPanelColour(PanelColourId::itemColour1));
            ni->setColour(trackColour, findPanelColour(PanelColourId::itemColour2));
            ni->setColour(maxPeakColour, findPanelColour(PanelColourId::textColour));
            
            if(ni->findColour(bgColour).isOpaque())
                ni->setOpaque(true);
            
            ni->skewFactor = skewFactor;
            ni->segmentSize = segmentSize;
            ni->paddingSize = paddingSize;
            ni->showMaxPeaks = showMaxPeak;
            
            ni->channelIndexes.addArray(channelIndexes);
            
            return ni;
        }
        
        return nullptr;
    }
    
    float segmentSize = 0.0f;
    float upDecayFactor = 1.0f;
    float downDecayFactor = 1.0f;
    bool useSourceChannel = false;
    float skewFactor = 1.0f;
    float paddingSize = 1.0f;
    float showMaxPeak = true;
    Array<int> channelIndexes;
};


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
		ShowMidiLabel = (int)FloatingTileContent::PanelPropertyId::numPropertyIds, ///< Specifies whether the **MIDI** label should be shown.
        MidiLabel,
        UseMidiPath,
        Base64MidiPath,
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
    void resized() override;
	void setOn(bool shouldBeOn);

private:

    bool showMidiLabel;
    bool useMidiPath;
    String midiLabel;
    String base64MidiPath;
    Path path;

	bool isOn = false;
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
		ShowExpansionsAsColumn,
		NumColumns,
		ColumnWidthRatio,
		EditButtonOffset,
		ListAreaOffset,
		ShowAddButton,
		ShowRenameButton,
		ShowDeleteButton,
		ButtonsInsideBorder,
		ColumnRowPadding,
		SearchBarBounds,
		SaveButtonBounds,
		MoreButtonBounds,
		FavoriteButtonBounds,
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


class TableFloatingTileBase : public FloatingTileContent,
							  public Component,
							  public TableListBoxModel
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

	TableFloatingTileBase(FloatingTile* parent);
	void initTable();

	virtual ~TableFloatingTileBase() {};

	void updateContent();
	void fromDynamicObject(const var& object);
	void paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected) override;

	//==============================================================================
	void resized() override;

	void selectedRowsChanged(int /*lastRowSelected*/) {};

	/** Tries to set the range and returns the actual value. Compare this against newRangeValue and update your UI accordingly. */
	double setRangeValue(int row, ColumnId column, double newRangeValue);;

	void deleteKeyPressed(int lastRowSelected) override;

	Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
		Component* existingComponentToUpdate) override;

	virtual void removeEntry(int rowIndex) = 0;
	virtual bool setRange(int rowIndex, NormalisableRange<double> newRange) = 0;
	virtual NormalisableRange<double> getRange(int rowIndex) const = 0;
	virtual bool isUsed(int rowIndex) const = 0;
	virtual bool isInverted(int rowIndex) const = 0;
	virtual NormalisableRange<double> getFullRange(int rowIndex) const = 0;
	virtual void setInverted(int rowIndex, bool value) = 0;

	virtual String getIndexName() const = 0;
	virtual String getCellText(int rowNumber, int columnId) const = 0;
	
	void paintCell(Graphics& g, int rowNumber, int columnId,
		int width, int height, bool /*rowIsSelected*/) override;

protected:

	Colour textColour;
	Colour itemColour1;
	Colour itemColour2;

	class ValueSliderColumn : public Component,
		public SliderListener
	{
	public:

		ValueSliderColumn(TableFloatingTileBase &table);
		void resized();
		void setRowAndColumn(const int newRow, int column, double value, NormalisableRange<double> range);

		ScopedPointer<Slider> slider;

	private:

		void sliderValueChanged(Slider *) override;

	private:
		TableFloatingTileBase &owner;

		HiPropertyPanelLookAndFeel laf;

		int row;
		int columnId;

	};

	class InvertedButton : public Component,
		public ButtonListener
	{
	public:

		InvertedButton(TableFloatingTileBase &owner_);;

		void resized();
		void setRowAndColumn(const int newRow, bool value);
		void buttonClicked(Button *b);;

		ScopedPointer<TextButton> t;

	private:

		TableFloatingTileBase &owner;

		int row;
		int columnId;
		HiPropertyPanelLookAndFeel laf;
	};

	TableListBox table;     // the table component itself
	Font font;
	int numRows;            // The number of rows of data we've got
	ScopedPointer<TableHeaderLookAndFeel> laf;
};


class MidiLearnPanel : public TableFloatingTileBase,
					   public SafeChangeListener
{
public:

	MidiLearnPanel(FloatingTile* parent);
	~MidiLearnPanel();

	void changeListenerCallback(SafeChangeBroadcaster*) override;

	SET_PANEL_NAME("MidiLearnPanel");

	String getIndexName() const override { return "CC #"; };

	int getNumRows() override;;
	void removeEntry(int rowIndex) override;
	bool setRange(int rowIndex, NormalisableRange<double> newRange);
	bool isInverted(int rowIndex) const override;

	NormalisableRange<double> getFullRange(int rowIndex) const override;
	NormalisableRange<double> getRange(int rowIndex) const override;

	bool isUsed(int rowIndex) const;
	void setInverted(int row, bool value);
	String getCellText(int rowNumber, int columnId) const override;

	MidiControllerAutomationHandler& handler;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnPanel)
};

struct FrontendMacroPanel : public TableFloatingTileBase,
						   public MacroControlBroadcaster::MacroConnectionListener
{
	SET_PANEL_NAME("FrontendMacroPanel");

	FrontendMacroPanel(FloatingTile* parent);;
	~FrontendMacroPanel();

	String getIndexName() const override { return "Source"; };

	void macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded);

	MacroControlBroadcaster* macroChain;
	MainController::MacroManager& macroManager;

	MacroControlBroadcaster::MacroControlData* getData(MacroControlBroadcaster::MacroControlledParameterData* pd);
	const MacroControlBroadcaster::MacroControlData* getData(MacroControlBroadcaster::MacroControlledParameterData* pd) const;

	int getNumRows() override;;
	void removeEntry(int rowIndex) override;
	bool setRange(int rowIndex, NormalisableRange<double> newRange);
	void paint(Graphics& g) override;
	bool isInverted(int rowIndex) const override;
	NormalisableRange<double> getFullRange(int rowIndex) const override;
	NormalisableRange<double> getRange(int rowIndex) const override;
	bool isUsed(int rowIndex) const;
	void setInverted(int row, bool value);
	String getCellText(int rowNumber, int columnId) const override;

	Array<WeakReference<MacroControlBroadcaster::MacroControlledParameterData>> connectionList;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrontendMacroPanel)
};

} // namespace hise


#endif  // FRONTENDPANELTYPES_H_INCLUDED
