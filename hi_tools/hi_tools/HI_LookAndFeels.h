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

#pragma once

namespace hise { using namespace juce;

/** A class that provides the image data from the binary resources. It selects the correct image size based on the dpi scaling of the display to support retina displays. */
class ImageProvider
{
public:

	enum ImageType
	{
		KnobEmpty = 0,
		KnobUnmodulated,
		KnobModulated,
		MacroKnob,
		BalanceKnob,
		ToggleButton
	};

	enum DisplayScaleFactor
	{
		OneHundred = 0,
		OneHundredTwentyFive,
		OneHundredFifty,
		TwoHundred
	};

	static DisplayScaleFactor getScaleFactor();

	static Image getImage(ImageType type);
};




struct HiToolbarIcons
{
	static Path createSettingsPath();;
};


/** The default overlay that displays an assignment to a macro control. */
class NumberTag: public Component
{
public:

	struct LookAndFeelMethods
	{
        virtual ~LookAndFeelMethods();;
        
		virtual void drawNumberTag(Graphics& g, Component& comp, Colour& c, Rectangle<int> area, int offset, int size, int number);
	};

	struct DefaultLookAndFeel : public LookAndFeel_V3,
								public LookAndFeelMethods
	{};

	NumberTag(float offset_=0.f, float size_=13.0f, Colour c_=Colours::black);;

    ~NumberTag();

	void paint(Graphics &g) override;;
 	
	void setNumber(int newNumber);

private:

	DefaultLookAndFeel dlaf;

	int number;

	float offset;
	float size;
    Colour c;
};


class PopupLookAndFeel : public LookAndFeel_V3
{
public:
    
    static void drawFake3D(Graphics& g, Rectangle<int> area);

    PopupLookAndFeel();;

    static int showAtComponent(PopupMenu& m, Component* c, bool alignToBottom);

    static void drawHiBackground(Graphics &g, int x, int y, int width, int height, Component *c = nullptr, bool isMouseOverButton = false);

	void setComboBoxFont(Font f);

    PopupMenu::Options getOptionsForComboBoxPopupMenu (ComboBox& box, Label& label);
   
    
protected:

	void getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight) override;

    void drawMenuBarBackground(Graphics& g, int width, int height,
                               bool, MenuBarComponent& menuBar);

	void drawMenuBarItem(Graphics& g, int width, int height,
		int /*itemIndex*/, const String& itemText,
		bool isMouseOverItem, bool isMenuOpen,
		bool /*isMouseOverBar*/, MenuBarComponent& menuBar);

    virtual void 	drawTooltip (Graphics &g, const String &text, int width, int height);

    bool shouldPopupMenuScaleWithTargetComponent(const PopupMenu::Options& /*options*/) override;;

	void drawPopupMenuBackground(Graphics& g, int width, int height) override;

    void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area,
                            bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked,
                            bool hasSubMenu, const String& text,
                            const String& shortcutKeyText,
                            const Drawable* icon, const Colour* textColourToUse);


    void drawPopupMenuSectionHeader (Graphics& g, const Rectangle<int>& area, const String& sectionName);

    Rectangle<int> getPropertyComponentContentPosition(PropertyComponent& component);

    Component* getParentComponentForMenuOptions(const PopupMenu::Options& options);;


	Font getPopupMenuFont() override;;





	void drawComboBox(Graphics &g, int width, int height, bool isButtonDown, int, int, int, int, ComboBox &c);

	

	Font getComboBoxFont(ComboBox & ) override;

    void positionComboBoxText(ComboBox &c, Label &labelToPosition) override;

	void drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label);

	Font comboBoxFont;
};


struct ScriptnodeComboBoxLookAndFeel : public PopupLookAndFeel
{
	void drawComboBox(Graphics&, int width, int height, bool isButtonDown,
		int buttonX, int buttonY, int buttonW, int buttonH,
		ComboBox&) override;

	Font getComboBoxFont(ComboBox&) override { return GLOBAL_BOLD_FONT(); }

	Label* createComboBoxTextBox(ComboBox&) override;

	static void drawScriptnodeDarkBackground(Graphics& g, Rectangle<float> area, bool roundedCorners);
};


class TableHeaderLookAndFeel: public PopupLookAndFeel
{
public:

	TableHeaderLookAndFeel();;

	void drawTableHeaderBackground (Graphics &, TableHeaderComponent &) override;

	void drawTableHeaderColumn (Graphics &g, TableHeaderComponent&, const String &columnName, int /*columnId*/, int width, int height, bool /*isMouseOver*/, bool /*isMouseDown*/, int /*columnFlags*/) override;


	Font f;
	Colour bgColour;
	Colour textColour;
};


/* A collection of all LookAndFeels used by the HI engine. */

/* A VU style for ModulatorSynth headers. */
class VUSliderLookAndFeel: public LookAndFeel_V3
{
	void drawLinearSlider(Graphics &g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider &slider);
};

class FileBrowserLookAndFeel : public LookAndFeel_V3
{
	void drawFileBrowserRow(Graphics&, int width, int height, const File& file,
		const String& filename, Image* icon,
		const String& fileSizeDescription, const String& fileTimeDescription,
		bool isDirectory, bool isItemSelected, int itemIndex,
		DirectoryContentsDisplayComponent&) override;
};

class ConcertinaPanelHeaderLookAndFeel : public LookAndFeel_V3
{
public:

	void drawConcertinaPanelHeader(Graphics& g, const Rectangle<int>& area,
		bool isMouseOver, bool /*isMouseDown*/,
		ConcertinaPanel&, Component& panel) override;

};

class HiPropertyPanelLookAndFeel: public LookAndFeel_V3
{
public:


	void setFontForAll(const Font& newFont);

	HiPropertyPanelLookAndFeel();;

	void drawPropertyPanelSectionHeader (Graphics& g, const String& name, bool isOpen, int width, int height) override;

	void drawPropertyComponentBackground (Graphics& g, int /*width*/, int /*height*/, PropertyComponent& /*component*/) override;

	Rectangle<int> getPropertyComponentContentPosition(PropertyComponent& component);

	void setLabelWidth(int newLabelWidth);


	void drawPropertyComponentLabel (Graphics& g, int /*width*/, int /*height*/, PropertyComponent& component) override;
	;

	void drawLinearSlider (Graphics &g, int /*x*/, int /*y*/, int width, 
						   int height, float /*sliderPos*/, float /*minSliderPos*/, 
						   float /*maxSliderPos*/, const Slider::SliderStyle, Slider &s) override;

	Font getLabelFont(Label &) override;

	Font getComboBoxFont (ComboBox&) override;

	Font getPopupMenuFont () override;;

	

	void drawPopupMenuBackground (Graphics& g, int width, int height) override;

	Font getTextButtonFont(TextButton &, int) override;;

	void drawButtonBackground(Graphics& g, Button& b, const Colour& , bool over, bool down);

	void drawButtonText(Graphics& g, TextButton& b, bool , bool );

	int labelWidth = 110;

	Font comboBoxFont;
	Font textButtonFont;
	Font labelFont;
	Font popupMenuFont;

	Colour propertyBgColour = Colour(0xff3d3d3d);
};


class PresetBrowserLookAndFeelMethods
{
public:

	enum ColourIds
	{
		highlightColourId = 0xf312,
		backgroundColourId,
		tableBackgroundColourId
	};

	PresetBrowserLookAndFeelMethods() :
		highlightColour(Colour(0xffffa8a8)),
		textColour(Colours::white.withAlpha(0.9f)),
		modalBackgroundColour(Colours::black.withAlpha(0.7f))
	{};

	virtual ~PresetBrowserLookAndFeelMethods()
	{};

	virtual Path createPresetBrowserIcons(const String& id);

	virtual void drawPresetBrowserBackground(Graphics& g, Component* p);
	virtual void drawColumnBackground(Graphics& g, Component& column, int columnIndex, Rectangle<int> listArea, const String& emptyText);

	virtual Font getTagFont(Component& tagButton) { return font.withHeight(14.0f); }

	virtual void drawTag(Graphics& g, Component& tagButton, bool hover, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position);
	virtual void drawModalOverlay(Graphics& g, Component& modalWindow, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command);
	virtual void drawListItem(Graphics& g, Component& column, int columnIndex, int, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode, bool hover);
	virtual void drawSearchBar(Graphics& g, Component& labelComponent, Rectangle<int> area);

	Font getFont(bool fontForTitle);

	void drawPresetBrowserButtonBackground(Graphics& g, Button& button, const Colour&c, bool isOver, bool isDown);

	

	void drawPresetBrowserButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown);

	Colour backgroundColour;
	Colour highlightColour;
	Colour textColour;
	Colour modalBackgroundColour;
	Font font;
};

class DefaultPresetBrowserLookAndFeel : public LookAndFeel_V3,
	public PresetBrowserLookAndFeelMethods
{
	void drawButtonBackground(Graphics& g, Button& button, const Colour& c, bool isOver, bool isDown)
	{
		drawPresetBrowserButtonBackground(g, button, c, isOver, isDown);
	}

	void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
	{
		drawPresetBrowserButtonText(g, button, isMouseOverButton, isButtonDown);
	}
};



/* A Bipolar Slider. */
class BiPolarSliderLookAndFeel: public LookAndFeel_V3
{
	void drawLinearSlider (Graphics &g, int /*x*/, int /*y*/, int width, 
						   int height, float /*sliderPos*/, float /*minSliderPos*/, 
						   float /*maxSliderPos*/, const Slider::SliderStyle style, Slider &s) override;

	void drawLinearSliderBackground(Graphics&, int /*x*/, int /*y*/, int /*width*/, int /*height*/,
	                                float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/,
	                                const Slider::SliderStyle, Slider&) override;;

	Font getLabelFont(Label &) override;

	JUCE_LEAK_DETECTOR(BiPolarSliderLookAndFeel);
};



class AutocompleteLookAndFeel: public LookAndFeel_V3
{
public:
	
private:

	Font getPopupMenuFont () override
	{
		return GLOBAL_MONOSPACE_FONT();
	};

	void drawPopupMenuBackground (Graphics& g, int /*width*/, int /*height*/) override
	{
		g.setColour(Colours::grey);
        g.fillAll();
	}

};


class NoiseBackgroundDrawer
{
public:
    
    
private:
    
    static NoiseBackgroundDrawer instance;
    
};


class ProcessorEditorLookAndFeel
{
public:

    ProcessorEditorLookAndFeel();
    

	static void drawShadowBox(Graphics& g, Rectangle<int> area, Colour fillColour);

	static void setupEditorNameLabel(Label* label);

	static void fillEditorBackgroundRect(Graphics& g, Component* c, int offsetFromLeftRight = 84);

    static void fillEditorBackgroundRectFixed(Graphics& g, Component* c, int fixedWidth);
    
    
	static void drawBackground(Graphics &g, int width, int height, Colour bgColour, bool folded, int intendationLevel = 0);
    
    static void drawNoiseBackground(Graphics &g, Rectangle<int> area, Colour c=Colours::lightgrey);
    
private:
    
    Image img;
};


//	===================================================================================================
/* This class is used to style a ProcessorEditorHeader. */
class ProcessorEditorHeaderLookAndFeel
{
public:

	enum ColourIds
	{
		HeaderBackgroundColour = 0

	};

	ProcessorEditorHeaderLookAndFeel():
		isChain(false)
	{};
    
    virtual ~ProcessorEditorHeaderLookAndFeel() {};
    

	virtual Colour getColour(int ColourId) const = 0;

	void drawBackground(Graphics &g, float width, float height, bool /*isFolded*/);

	bool isChain;

};

/* A dark background with a normal slider. */
class ModulatorEditorHeaderLookAndFeel: public ProcessorEditorHeaderLookAndFeel
{
public:

	

    virtual ~ModulatorEditorHeaderLookAndFeel() {};
    
	Colour getColour(int /*ColourId*/) const override { return isChain ? c : Colour(0xFF222222); };

    Colour c;

};

/* A bright background and a VU meter slider. */
class ModulatorSynthEditorHeaderLookAndFeel: public ProcessorEditorHeaderLookAndFeel
{
public:
    
    virtual ~ModulatorSynthEditorHeaderLookAndFeel() {};

	Colour getColour(int /*ColourId*/) const override { return HiseColourScheme::getColour(HiseColourScheme::ColourIds::ModulatorSynthHeader);};
};



class AlertWindowLookAndFeel: public PopupLookAndFeel
{
public:

	AlertWindowLookAndFeel();

	Font getAlertWindowMessageFont () override;

	virtual Font 	getAlertWindowTitleFont() override;

	Font getTextButtonFont (TextButton &, int /*buttonHeight*/) override;

	Font getComboBoxFont(ComboBox&) override;

	Font getPopupMenuFont() override;;

	void drawPopupMenuBackground(Graphics& g, int width, int height) override;

	void drawButtonText (Graphics &g, TextButton &button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;
 	
	void drawButtonBackground (Graphics& g, Button& button, const Colour& /*backgroundColour*/,
                                           bool isMouseOverButton, bool isButtonDown) override;
 
	Font getAlertWindowFont () override;;

	void setColourIdsForAlertWindow(AlertWindow &window);

	void drawProgressBar (Graphics &g, ProgressBar &/*progressBar*/, int width, int height, double progress, const String &textToShow) override;

	void drawAlertBox (Graphics &g, AlertWindow &alert, const Rectangle< int > &textArea, juce::TextLayout &textLayout) override;;

	Colour dark, bright, special;
};



class FrontendKnobLookAndFeel : public LookAndFeel_V3
{
public:

	FrontendKnobLookAndFeel();

	void drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

	void setCustomFilmstripImage(const Image *customImage, int numFilmstrips);

private:

	int numStrips;

	bool useCustomStrip;

	Image volumeFilmStrip;

	Image balanceFilmStrip;

};

class GlobalHiseLookAndFeel: public AlertWindowLookAndFeel
{
public:

	GlobalHiseLookAndFeel();
	
	Slider::SliderLayout getSliderLayout(Slider&s) override;

	Label *createSliderTextBox (Slider &s) override;

	static void fillPathHiStyle(Graphics &g, const Path &p, int , int , bool drawBorders = true);;

    static void draw1PixelGrid(Graphics& g, Component* c, Rectangle<int> bounds, Colour lineColour=Colours::white);
    
    
	static Point<float> paintCable(Graphics& g, Rectangle<float> start, Rectangle<float> end, Colour c, float alpha = 1.0f, Colour holeColour = Colour(0xFFAAAAAA), bool returnMidPoint = false, bool useHangingCable=true, Point<float> velocity={});;

	static void setTextEditorColours(TextEditor& ed);

	int getSliderThumbRadius(Slider& ) override;

	void drawVectorRotaryKnob(Graphics& g, Rectangle<float> area, double value, bool bipolar, bool hover, bool down, bool enabled, float modValue);

	void drawLinearSlider (Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPos*/, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider &s) override;


	void drawRotarySlider (Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;
	

	void drawToggleButton (Graphics &g, ToggleButton &b, bool isMouseOverButton, bool /*isButtonDown*/) override;
    
	static void setDefaultColours(Component& c);

private:
    
	Path ring, ring2;

};

class FilmstripLookAndFeel : public GlobalHiseLookAndFeel
{
public:

	FilmstripLookAndFeel();;

	void setFilmstripImage(const Image &imageToUse_, int numStrips_, bool isVertical_=true);;

    void setScaleFactor(float newScaleFactor) noexcept;

	void drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool isButtonDown) override;

	void drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

private:

	int heightOfEachStrip;
	int widthOfEachStrip;

    float scaleFactor;
	
	bool isVertical;
	Image imageToUse;
    int numStrips;

};


class BalanceButtonLookAndFeel: public LookAndFeel_V3
{

public:

	BalanceButtonLookAndFeel();

	void drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           Slider& s) override;

};

class ChainBarButtonLookAndFeel: public LookAndFeel_V3
{
public:

	enum ColourIds
	{
		IconColour = Slider::ColourIds::thumbColourId,
		IconColourOff = Slider::ColourIds::trackColourId
	};

	Font getTextButtonFont (TextButton&, int /*buttonHeight*/) override
	{
		return GLOBAL_BOLD_FONT();
	}

	void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;

	void drawButtonBackground (Graphics &g, Button &b, const Colour &backgroundColour, bool isMouseOverButton, bool isButtonDown) override;;
};


class MacroKnobLookAndFeel: public LookAndFeel_V3
{

public:

	MacroKnobLookAndFeel();

	void drawRotarySlider (Graphics &g, int /*x*/, int /*y*/, int /*width*/, int /*height*/, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

	Label *createSliderTextBox (Slider &);

private:
	Image cachedImage_macroKnob_png;

	Image cachedImage_macroKnob_ring_png;

	static const char* macroKnob_png;
    static const int macroKnob_pngSize;
};


class BlackTextButtonLookAndFeel : public LookAndFeel_V3
{
public:

	BlackTextButtonLookAndFeel();

	void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/,
		bool isMouseOverButton, bool isButtonDown);

	void drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool);

	void drawButtonText(Graphics& g, TextButton& b, bool , bool ) override;

	Colour textColour;
	Font f;

private:


	Image ticked;
	Image unticked;
};



} // namespace hise
