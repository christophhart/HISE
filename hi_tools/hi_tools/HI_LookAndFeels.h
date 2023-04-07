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
	static Path createSettingsPath()
	{
		static const unsigned char pathData[] = { 110,109,35,219,53,65,18,131,180,65,108,19,131,74,65,86,14,166,65,108,84,227,51,65,119,190,154,65,108,219,249,22,65,111,18,165,65,98,178,157,17,65,117,147,163,65,59,223,11,65,66,96,162,65,10,215,5,65,31,133,161,65,108,0,0,0,65,0,0,144,65,108,0,0,192,64,
		0,0,144,65,108,236,81,180,64,31,133,161,65,98,138,65,168,64,66,96,162,65,156,196,156,64,105,145,163,65,74,12,146,64,111,18,165,65,108,176,114,48,64,119,190,154,65,108,108,231,171,63,86,14,166,65,108,116,147,40,64,18,131,180,65,98,165,155,28,64,38,49,
		183,65,12,2,19,64,98,16,186,65,87,57,12,64,122,20,189,65,108,0,8,131,58,255,255,191,65,108,0,8,131,58,255,255,207,65,108,87,57,12,64,132,235,210,65,98,110,18,19,64,157,239,213,65,165,155,28,64,216,206,216,65,214,163,40,64,236,124,219,65,108,47,8,172,
		63,168,241,233,65,108,18,131,48,64,135,65,245,65,108,123,20,146,64,143,237,234,65,98,205,204,156,64,137,108,236,65,186,73,168,64,188,159,237,65,29,90,180,64,223,122,238,65,108,50,8,192,64,254,255,255,65,108,25,4,0,65,254,255,255,65,108,35,219,5,65,223,
		122,238,65,98,84,227,11,65,188,159,237,65,203,161,17,65,149,110,236,65,244,253,22,65,143,237,234,65,108,109,231,51,65,135,65,245,65,108,44,135,74,65,168,241,233,65,108,60,223,53,65,236,124,219,65,98,48,221,56,65,216,206,216,65,150,67,59,65,156,239,213,
		65,220,249,60,65,132,235,210,65,108,26,4,96,65,255,255,207,65,108,26,4,96,65,255,255,191,65,108,220,249,60,65,122,20,189,65,98,150,67,59,65,97,16,186,65,73,225,56,65,38,49,183,65,60,223,53,65,18,131,180,65,99,109,0,0,224,64,0,0,216,65,98,215,163,188,
		64,0,0,216,65,0,0,160,64,10,215,208,65,0,0,160,64,0,0,200,65,98,0,0,160,64,246,40,191,65,215,163,188,64,0,0,184,65,0,0,224,64,0,0,184,65,98,20,174,1,65,0,0,184,65,0,0,16,65,246,40,191,65,0,0,16,65,0,0,200,65,98,0,0,16,65,10,215,208,65,20,174,1,65,0,0,
		216,65,0,0,224,64,0,0,216,65,99,109,0,0,0,66,0,0,64,65,108,0,0,0,66,0,0,32,65,108,234,38,239,65,59,223,25,65,98,11,215,238,65,34,219,21,65,177,114,238,65,84,227,17,65,207,247,237,65,231,251,13,65,108,41,92,252,65,71,225,246,64,108,113,61,246,65,118,190,
		187,64,108,6,129,229,65,63,53,202,64,98,176,114,228,65,198,75,195,64,235,81,227,65,116,147,188,64,196,32,226,65,122,20,182,64,108,71,225,235,65,186,116,123,64,108,104,145,224,65,192,245,32,64,108,225,122,210,65,216,249,110,64,98,35,219,208,65,161,112,
		101,64,14,45,207,65,124,106,92,64,176,114,205,65,204,247,83,64,108,98,16,209,65,240,40,156,63,108,174,71,194,65,172,245,232,62,108,12,2,185,65,134,65,16,64,98,86,14,183,65,222,122,12,64,110,18,181,65,171,71,9,64,98,16,179,65,177,200,6,64,108,0,0,176,
		65,0,0,64,181,108,0,0,160,65,0,0,64,181,108,158,239,156,65,177,200,6,64,98,146,237,154,65,171,71,9,64,170,241,152,65,124,106,12,64,244,253,150,65,134,65,16,64,108,82,184,141,65,168,245,232,62,108,60,223,125,65,239,40,156,63,108,80,141,130,65,204,247,
		83,64,98,242,210,128,65,124,106,92,64,187,73,126,65,161,112,101,64,62,10,123,65,216,249,110,64,108,48,221,94,65,192,245,32,64,108,114,61,72,65,186,116,123,64,108,120,190,91,65,122,20,182,64,98,42,92,89,65,116,147,188,64,161,26,87,65,198,75,195,64,245,
		253,84,65,63,53,202,64,108,32,133,51,65,118,190,187,64,108,175,71,39,65,71,225,246,64,108,99,16,68,65,231,251,13,65,98,185,30,67,65,84,227,17,65,236,81,66,65,34,219,21,65,46,178,65,65,59,223,25,65,108,1,0,32,65,0,0,32,65,108,1,0,32,65,0,0,64,65,108,46,
		178,65,65,197,32,70,65,98,236,81,66,65,222,36,74,65,161,26,67,65,172,28,78,65,99,16,68,65,25,4,82,65,108,175,71,39,65,93,143,100,65,108,32,133,51,65,99,16,129,65,108,245,253,84,65,98,229,122,65,98,161,26,87,65,30,90,126,65,42,92,89,65,36,219,128,65,120,
		190,91,65,226,122,130,65,108,114,61,72,65,105,145,144,65,108,48,221,94,65,72,225,155,65,108,62,10,123,65,197,32,146,65,98,187,73,126,65,236,81,147,65,242,210,128,65,177,114,148,65,80,141,130,65,7,129,149,65,108,60,223,125,65,114,61,166,65,108,82,184,
		141,65,42,92,172,65,108,244,253,150,65,208,247,157,65,98,170,241,152,65,165,112,158,65,146,237,154,65,11,215,158,65,158,239,156,65,235,38,159,65,108,0,0,160,65,2,0,176,65,108,0,0,176,65,2,0,176,65,108,98,16,179,65,236,38,159,65,98,110,18,181,65,13,215,
		158,65,86,14,183,65,179,114,158,65,12,2,185,65,209,247,157,65,108,174,71,194,65,43,92,172,65,108,98,16,209,65,115,61,166,65,108,176,114,205,65,8,129,149,65,98,14,45,207,65,178,114,148,65,35,219,208,65,237,81,147,65,225,122,210,65,198,32,146,65,108,104,
		145,224,65,73,225,155,65,108,71,225,235,65,106,145,144,65,108,196,32,226,65,227,122,130,65,98,235,81,227,65,37,219,128,65,176,114,228,65,32,90,126,65,6,129,229,65,100,229,122,65,108,112,61,246,65,100,16,129,65,108,40,92,252,65,96,143,100,65,108,206,247,
		237,65,28,4,82,65,98,163,112,238,65,175,28,78,65,9,215,238,65,225,36,74,65,233,38,239,65,200,32,70,65,108,0,0,0,66,3,0,64,65,99,109,0,0,168,65,154,153,117,65,98,180,200,148,65,154,153,117,65,51,51,133,65,152,110,86,65,51,51,133,65,0,0,48,65,98,51,51,
		133,65,104,145,9,65,180,200,148,65,205,204,212,64,0,0,168,65,205,204,212,64,98,76,55,187,65,205,204,212,64,205,204,202,65,104,145,9,65,205,204,202,65,0,0,48,65,98,205,204,202,65,152,110,86,65,76,55,187,65,154,153,117,65,0,0,168,65,154,153,117,65,99,101,
		0,0 };

		Path path;
		path.loadPathFromData (pathData, sizeof (pathData));
		
		return path;
		
	};
};


/** The default overlay that displays an assignment to a macro control. */
class NumberTag: public Component
{
public:

	struct LookAndFeelMethods
	{
        virtual ~LookAndFeelMethods() {};
        
		virtual void drawNumberTag(Graphics& g, Colour& c, Rectangle<int> area, int offset, int size, int number)
		{
			if (number > 0)
			{
				Rectangle<float> rect = area.reduced(offset).removeFromTop(size).removeFromRight(size).toFloat();
				DropShadow d;

				d.colour = c.withAlpha(0.3f);
				d.radius = (int)offset * 2;
				d.offset = Point<int>();
				d.drawForRectangle(g, Rectangle<int>((int)rect.getX(), (int)rect.getY(), (int)rect.getWidth(), (int)rect.getHeight()));

				g.setColour(Colours::black.withAlpha(0.3f));
				g.setColour(Colours::white.withAlpha(0.7f));
				g.drawRoundedRectangle(rect.reduced(1.0f), 4.0f, 1.0f);
				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText(String(number), rect, Justification::centred, false);
			}
		}
	};

	struct DefaultLookAndFeel : public LookAndFeel_V3,
								public LookAndFeelMethods
	{};

	NumberTag(float offset_=0.f, float size_=13.0f, Colour c_=Colours::black):
		number(0),
		offset(offset_),
		size(size_),
		c(c_)
	{
		setInterceptsMouseClicks(false, false);
		setLookAndFeel(&dlaf);
	};

    ~NumberTag()
    {
        setLookAndFeel(nullptr);
    }
    
	void paint(Graphics &g) override
	{
		if(number == 0) 
			return;

		if (auto l = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
			l->drawNumberTag(g, c, getLocalBounds(), roundToInt(offset), roundToInt(size), number);
	};
 	
	void setNumber(int newNumber)
	{
		number = newNumber;
	}

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
    
    static void drawFake3D(Graphics& g, Rectangle<int> area)
    {
        Colour upperLight = JUCE_LIVE_CONSTANT_OFF(Colour(0x10ffffff));
        Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0x06ffffff));
        Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0x10000000));
        Colour shadowLight = JUCE_LIVE_CONSTANT_OFF(Colour(0x58000000));
        
        
        g.setGradientFill(ColourGradient(c1, 0.0f, (float)area.getY(), c2, 0.0f, (float)area.getBottom(), false));
        g.fillRect(area);
        
        g.setColour(upperLight);
        
        g.drawHorizontalLine(area.getY(), (float)area.getX(), (float)area.getRight());
        
        g.setColour(shadowLight);
        g.drawHorizontalLine(area.getBottom()-1, (float)area.getX(), (float)area.getRight());
    }
    
	PopupLookAndFeel()
	{
		comboBoxFont = GLOBAL_BOLD_FONT();

		setColour(PopupMenu::highlightedBackgroundColourId, Colour(SIGNAL_COLOUR));

		Colour dark = JUCE_LIVE_CONSTANT_OFF(Colour(0xff333333));

		Colour bright(0xFFAAAAAA);

		setColour(PopupMenu::ColourIds::backgroundColourId, dark);
		setColour(PopupMenu::ColourIds::textColourId, bright);
		setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
		setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
		setColour(PopupMenu::ColourIds::headerTextColourId, bright);
	};

    static int showAtComponent(PopupMenu& m, Component* c, bool alignToBottom)
    {
        
        PopupMenu::Options options;
        options = options.withTargetComponent (c);
        
        if(!alignToBottom)
        {
            Rectangle<int> r;
            
            r.setPosition(Desktop::getMousePosition());
            options = options.withTargetScreenArea(r);
        }
        
        return m.showMenu(options);
    }
    
	static void drawHiBackground(Graphics &g, int x, int y, int width, int height, Component *c = nullptr, bool isMouseOverButton = false);

	void setComboBoxFont(Font f)
	{
		comboBoxFont = f;
	}

    PopupMenu::Options getOptionsForComboBoxPopupMenu (ComboBox& box, Label& label);
   
    
protected:

	void getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight) override
	{
		if (HiseDeviceSimulator::isMobileDevice())
		{
			idealHeight = 28;

			idealWidth = getPopupMenuFont().getStringWidth(text) + 50;
		}
		else
		{
			if (isSeparator)
			{
				idealWidth = 50;
				idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : 10;
			}
			else
			{
				Font font(getPopupMenuFont());

				if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / 1.3f)
					font.setHeight(standardMenuItemHeight / 1.3f);

				idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : roundToInt(font.getHeight() * 1.3f);

				idealHeight = jmax<int>(idealHeight, 18);

				idealWidth = font.getStringWidth(text) + idealHeight * 2;
			}
		}
	}

	void drawMenuBarBackground(Graphics& g, int width, int height,
		bool, MenuBarComponent& menuBar);

	void drawMenuBarItem(Graphics& g, int width, int height,
		int /*itemIndex*/, const String& itemText,
		bool isMouseOverItem, bool isMenuOpen,
		bool /*isMouseOverBar*/, MenuBarComponent& menuBar)
	{
		if (!menuBar.isEnabled())
		{
			g.setColour(menuBar.findColour(PopupMenu::textColourId));
		}
		else if (isMenuOpen || isMouseOverItem)
		{
			Colour c1 = findColour(PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.4f));
			Colour c2 = findColour(PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(1.1f);

			g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)height, false));

			g.fillRect(0, 0, width, height);

			Colour dark(0xFF444444);

			

			g.setColour(dark);
		}
		else
		{
			g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffbdbdbd)));
		}

		g.setFont(getPopupMenuFont());
		
		g.drawFittedText(itemText, 0, 0, width, height, Justification::centred, 1);

		
	}
    
    virtual void 	drawTooltip (Graphics &g, const String &text, int width, int height)
    {
        g.fillAll(Colour(0xFF444444));
        g.setColour(Colours::white.withAlpha(0.8f));
        g.drawRect(0, 0, width, height, 1);
        g.setFont(GLOBAL_BOLD_FONT());
        g.drawText(text, 0, 0, width, height, Justification::centred);
    }

	bool shouldPopupMenuScaleWithTargetComponent(const PopupMenu::Options& /*options*/) override
	{
		return true;
	};

	void drawPopupMenuBackground(Graphics& g, int width, int height) override
	{
		Colour c1 = findColour(PopupMenu::backgroundColourId).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.1f));
		Colour c2 = findColour(PopupMenu::backgroundColourId).withMultipliedBrightness(0.9f);

		g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)height, false));

		g.fillRect(0.0f, 0.0f, (float)width, (float)height);
		(void)width; (void)height;

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff555555)));

		g.drawRect(0.0f, 0.0f, (float)width, (float)height, 1.0f);

#if ! JUCE_MAC
		g.setColour(findColour(PopupMenu::textColourId));
		//g.drawRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f, 0.5f);
#endif
	}
    
    void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area,
                                            bool isSeparator, bool isActive,
                                            bool isHighlighted, bool isTicked,
                                            bool hasSubMenu, const String& text,
                                            const String& shortcutKeyText,
                                            const Drawable* icon, const Colour* textColourToUse)
    {
        if (isSeparator)
        {
            Rectangle<int> r (area.reduced (0, 0));
            r.removeFromTop (r.getHeight() / 2 - 1);
            
            g.setColour (Colour(0x55999999));
            g.fillRect (r.removeFromTop (1));
        }
        else
        {
            Colour textColour (findColour (PopupMenu::textColourId));
            
            if (textColourToUse != nullptr)
                textColour = *textColourToUse;
            
            Rectangle<int> r (area.reduced (1));
            
            if (isHighlighted)
            {
				Colour c1 = findColour (PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.4f));
				Colour c2 = findColour(PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(1.1f);
                
				g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)r.getHeight(), false));

				g.fillRect (r);
                
                g.setColour (findColour (PopupMenu::highlightedTextColourId));
            }
            else
            {
                g.setColour (textColour);
            }
            
            if (! isActive)
                g.setOpacity (0.3f);
            
            Font font (getPopupMenuFont());
            
            const float maxFontHeight = area.getHeight() / 1.3f;
            
            if (font.getHeight() > maxFontHeight)
                font.setHeight (maxFontHeight);
            
            g.setFont (font);
            
            Rectangle<float> iconArea (r.removeFromLeft ((r.getHeight() * 5) / 4).reduced (3).toFloat());
            
            if (icon != nullptr)
            {
				if (auto dp = dynamic_cast<const DrawablePath*>(icon))
				{
					auto p = dp->getPath();

					p.scaleToFit(iconArea.getX(), iconArea.getY(), iconArea.getWidth(), iconArea.getHeight(), true);

					g.fillPath(p);
				}
				else
				{
					icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
				}

				
            }
            else if (isTicked)
            {
                const Path tick (getTickShape (1.0f));
                g.fillPath (tick, tick.getTransformToScaleToFit (iconArea, true));
            }
            
            if (hasSubMenu)
            {
                const float arrowH = 0.6f * getPopupMenuFont().getAscent();
                
                const float x = (float) r.removeFromRight ((int) arrowH).getX();
                const float halfH = (float) r.getCentreY();
                
                Path p;
                p.addTriangle (x, halfH - arrowH * 0.5f,
                               x, halfH + arrowH * 0.5f,
                               x + arrowH * 0.6f, halfH);
                
                g.fillPath (p);
            }
            
            r.removeFromRight (3);
            g.drawFittedText (text, r, Justification::centredLeft, 1);
            
            if (shortcutKeyText.isNotEmpty())
            {
                Font f2 (font);
                f2.setHeight (f2.getHeight() * 0.75f);
                f2.setHorizontalScale (0.95f);
                g.setFont (f2);
                
                g.drawText (shortcutKeyText, r, Justification::centredRight, true);
            }
        }
    }
    


    void drawPopupMenuSectionHeader (Graphics& g, const Rectangle<int>& area, const String& sectionName)
    {
        g.setFont (getPopupMenuFont());
        g.setColour (Colours::white);
        
        g.drawFittedText (sectionName,
                          area.getX() + 12, area.getY(), area.getWidth() - 16, (int) (area.getHeight() * 0.8f),
                          Justification::bottomLeft, 1);
    }

	Rectangle<int> getPropertyComponentContentPosition(PropertyComponent& component)
	{
		const int textW = jmin(200, component.getWidth() / 3);
		return Rectangle<int>(textW, 1, component.getWidth() - textW - 1 - component.getHeight()-1, component.getHeight() - 3);
	}

	Component* getParentComponentForMenuOptions(const PopupMenu::Options& options)
	{
		if (HiseDeviceSimulator::isAUv3())
		{
			if (options.getParentComponent() == nullptr && options.getTargetComponent() != nullptr)
				return options.getTargetComponent()->getTopLevelComponent();
		}
		
		return LookAndFeel_V3::getParentComponentForMenuOptions(options);

	};


	Font getPopupMenuFont() override;;





	void drawComboBox(Graphics &g, int width, int height, bool isButtonDown, int, int, int, int, ComboBox &c);

	

	Font getComboBoxFont(ComboBox & ) override
	{
		return comboBoxFont;
	}

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

	TableHeaderLookAndFeel()
	{
		f = GLOBAL_BOLD_FONT();

		bgColour = Colour(0xff474747);
		textColour = Colour(0xa2ffffff);
	};

	void drawTableHeaderBackground (Graphics &, TableHeaderComponent &) override
	{
		
		
	}

	void drawTableHeaderColumn (Graphics &g, TableHeaderComponent&, const String &columnName, int /*columnId*/, int width, int height, bool /*isMouseOver*/, bool /*isMouseDown*/, int /*columnFlags*/) override
	{
		if (width > 0)
		{
			g.setColour(bgColour);

			g.fillRect(0.0f, 0.0f, (float)width - 1.0f, (float)height);

			g.setFont(f);
			g.setColour(textColour);

			g.drawText(columnName, 3, 0, width - 3, height, Justification::centredLeft, true);
		}
	}

	
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


	void setFontForAll(const Font& newFont)
	{
		comboBoxFont = newFont;
		textButtonFont = newFont;
		labelFont = newFont;
		popupMenuFont = newFont;
	}

	HiPropertyPanelLookAndFeel()
	{
		comboBoxFont = GLOBAL_FONT();
		textButtonFont = GLOBAL_FONT();
		labelFont = GLOBAL_FONT();
		popupMenuFont = GLOBAL_FONT();

		setColour(PopupMenu::highlightedBackgroundColourId, Colour(SIGNAL_COLOUR));

		Colour dark(0xFF333333);

		Colour bright(0xFF999999);

		setColour(PopupMenu::ColourIds::backgroundColourId, dark);
		setColour(PopupMenu::ColourIds::textColourId, bright);
		setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
		setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
		setColour(PopupMenu::ColourIds::headerTextColourId, bright);
        setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
        setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

	};

	void drawPropertyPanelSectionHeader (Graphics& g, const String& name, bool isOpen, int width, int height) override
	{
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff1b1b1b)));

        auto r = Rectangle<int>(0, 0, width, height);
        
		g.fillRect(r);

        PopupLookAndFeel::drawFake3D(g, r);
        
        
        
		const float buttonSize = height * 0.75f;
		const float buttonIndent = (height - buttonSize) * 0.5f;

		drawTreeviewPlusMinusBox (g, Rectangle<float> (buttonIndent, buttonIndent, buttonSize, buttonSize), Colours::black, isOpen, false);

		const int textX = (int) (buttonIndent * 2.0f + buttonSize + 2.0f);

		g.setColour (JUCE_LIVE_CONSTANT_OFF(Colour(0xffa2a2a2)));
		g.setFont (GLOBAL_BOLD_FONT());
		g.drawText (name, textX, 0, width - textX - 4, height, Justification::centredLeft, true);
	}

    void drawPropertyComponentBackground (Graphics& g, int /*width*/, int /*height*/, PropertyComponent& /*component*/) override;

	Rectangle<int> getPropertyComponentContentPosition(PropertyComponent& component)
	{
		const int textW = jmin(labelWidth, component.getWidth() / 3);
		return Rectangle<int>(textW, 1, component.getWidth() - textW - 1, component.getHeight() - 3);
	}

	void setLabelWidth(int newLabelWidth)
	{
		labelWidth = newLabelWidth;
	}


    void drawPropertyComponentLabel (Graphics& g, int /*width*/, int /*height*/, PropertyComponent& component) override
	{
		Colour textColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xffdddddd));

		 g.setColour (textColour.withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.6f));

		 g.setFont (labelFont);

		const Rectangle<int> r (getPropertyComponentContentPosition (component));

		g.drawFittedText (component.getName(),
						  3, r.getY(), r.getX() - 8, r.getHeight(),
						  Justification::centredRight, 2);
	};

	void drawLinearSlider (Graphics &g, int /*x*/, int /*y*/, int width, 
						   int height, float /*sliderPos*/, float /*minSliderPos*/, 
						   float /*maxSliderPos*/, const Slider::SliderStyle, Slider &s) override
	{
		const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

		float leftX;
		float actualWidth;

		const float max = (float)s.getMaximum();
		const float min = (float)s.getMinimum();

		Colour c = s.findColour(Slider::ColourIds::backgroundColourId);

		g.fillAll(c);

		if(isBiPolar)
		{
			const float value = ((float)s.getValue() - min) / (max - min);

			leftX = 2.0f + (value < 0.5f ? value * (float)(width-2) : 0.5f * (float)(width-2));
			actualWidth = fabs(0.5f - value) * (float)(width-2);
		}
		else
		{
			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			//const float value = ((float)s.getValue() - min) / (max - min);

			leftX = 2;
			actualWidth = (float)proportion * (float)(width-2);
		}

		c = s.findColour(Slider::ColourIds::thumbColourId);

		g.setGradientFill (ColourGradient (c.withMultipliedAlpha(s.isEnabled() ? 0.8f : 0.4f),
                                       0.0f, 0.0f,
                                       c.withMultipliedAlpha(s.isEnabled() ? 0.8f : 0.4f),
                                       0.0f, (float)height,
                                       false));
		g.fillRect(leftX, 2.0f, actualWidth , (float)(height-2));

	}

	Font getLabelFont(Label &) override
	{
		return labelFont;
	}

	Font getComboBoxFont (ComboBox&) override
	{
		return comboBoxFont;
	}

	Font getPopupMenuFont () override
	{
		return popupMenuFont;
	};

	

	void drawPopupMenuBackground (Graphics& g, int width, int height) override
	{
		g.setColour( (findColour (PopupMenu::backgroundColourId)));
		g.fillRect(0.0f, 0.0f, (float)width, (float)height);
	}

	Font getTextButtonFont(TextButton &, int) override
	{
		return textButtonFont;
	};

	void drawButtonBackground(Graphics& g, Button& b, const Colour& , bool over, bool down)
	{
		auto ar = b.getLocalBounds().toFloat();

		if (b.getToggleState())
		{
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.4f));
			g.fillRoundedRectangle(ar, 2.0f);
		}

		g.setColour(Colours::white.withAlpha(0.1f));

		if (over)
			g.fillRoundedRectangle(ar, 2.0f);

		if (down)
			g.fillRoundedRectangle(ar, 2.0f);
	}

	void drawButtonText(Graphics& g, TextButton& b, bool , bool )
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(b.getToggleState() ? Colours::black : Colours::white);
		g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
	}

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
	virtual void drawColumnBackground(Graphics& g, int columnIndex, Rectangle<int> listArea, const String& emptyText);
	virtual void drawTag(Graphics& g, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position);
	virtual void drawModalOverlay(Graphics& g, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command);
	virtual void drawListItem(Graphics& g, int columnIndex, int, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode, bool hover);
	virtual void drawSearchBar(Graphics& g, Rectangle<int> area);

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
						   float /*maxSliderPos*/, const Slider::SliderStyle style, Slider &s) override
	{

		if (style == Slider::SliderStyle::LinearBarVertical)
		{
			const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

			float leftY;
			float actualHeight;

			const float max = (float)s.getMaximum();
			const float min = (float)s.getMinimum();

			g.fillAll(s.findColour(Slider::ColourIds::backgroundColourId));

			if (isBiPolar)
			{
				const float value = (-1.0f * (float)s.getValue() - min) / (max - min);

				leftY = (value < 0.5f ? value * (float)(height) : 0.5f * (float)(height));
				actualHeight = fabs(0.5f - value) * (float)(height);
			}
			else
			{
				const double value = s.getValue();
				const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
				const double proportion = pow(normalizedValue, s.getSkewFactor());

				//const float value = ((float)s.getValue() - min) / (max - min);

				actualHeight = (float)proportion * (float)(height);
				leftY = (float)height - actualHeight ;
			}

			Colour c = s.findColour(Slider::ColourIds::thumbColourId);

			g.setGradientFill(ColourGradient(c.withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.4f),
				0.0f, 0.0f,
				c.withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.3f).withMultipliedBrightness(0.9f),
				0.0f, (float)height,
				false));

			g.fillRect(0.0f, leftY, (float)(width+1), actualHeight+1.0f);

			if (width > 4)
			{
				//g.setColour(Colours::white.withAlpha(0.3f));
				g.setColour(s.findColour(Slider::trackColourId));
				g.drawRect(0.0f, leftY, (float)(width + 1), actualHeight + 1.0f, 1.0f);
			}

			
		}
		else
		{
			const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

			float leftX;
			float actualWidth;

			const float max = (float)s.getMaximum();
			const float min = (float)s.getMinimum();

			g.fillAll(Colour(0xfb333333));

			if (isBiPolar)
			{
				const float value = ((float)s.getValue() - min) / (max - min);

				leftX = 2.0f + (value < 0.5f ? value * (float)(width - 2) : 0.5f * (float)(width - 2));
				actualWidth = fabs(0.5f - value) * (float)(width - 2);
			}
			else
			{
				const double value = s.getValue();
				const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
				const double proportion = pow(normalizedValue, s.getSkewFactor());

				//const float value = ((float)s.getValue() - min) / (max - min);

				leftX = 2;
				actualWidth = (float)proportion * (float)(width - 2);
			}

			g.setGradientFill(ColourGradient(Colour(0xff888888).withAlpha(s.isEnabled() ? 0.8f : 0.4f),
				0.0f, 0.0f,
				Colour(0xff666666).withAlpha(s.isEnabled() ? 0.8f : 0.4f),
				0.0f, (float)height,
				false));
			g.fillRect(leftX, 2.0f, actualWidth, (float)(height - 2));
		}

		

	}

	void drawLinearSliderBackground(Graphics&, int /*x*/, int /*y*/, int /*width*/, int /*height*/,
		float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/,
		const Slider::SliderStyle, Slider&) override {};

	Font getLabelFont(Label &) override
	{
		return GLOBAL_FONT();
	}

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

	AlertWindowLookAndFeel()
	{
		dark = Colour(0xFF252525);
		bright = Colour(0xFFAAAAAA);

		setColour(PopupMenu::ColourIds::backgroundColourId, dark);
		setColour(PopupMenu::ColourIds::textColourId, bright);
		setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
		setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
		setColour(PopupMenu::ColourIds::headerTextColourId, dark);
        
        
        setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
        setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour (SIGNAL_COLOUR));
                                  
	}

	Font getAlertWindowMessageFont () override
	{
		return GLOBAL_BOLD_FONT();
	}
    
	virtual Font 	getAlertWindowTitleFont() override
	{
		if (HiseDeviceSimulator::isMobileDevice())
		{
			return GLOBAL_BOLD_FONT().withHeight(24.0f);
		}
		else
		{
			return GLOBAL_BOLD_FONT().withHeight(17.0f);
		}
    }

	Font getTextButtonFont (TextButton &, int /*buttonHeight*/) override
	{
		return getAlertWindowFont();
	}

	Font getComboBoxFont(ComboBox&) override
	{
		return GLOBAL_FONT();
	}

	Font getPopupMenuFont() override
	{
		return GLOBAL_BOLD_FONT();
	};

	void drawPopupMenuBackground(Graphics& g, int width, int height) override
	{
		g.setColour((findColour(PopupMenu::backgroundColourId)));
		g.fillRect(0.0f, 0.0f, (float)width, (float)height);
	}
 
	void drawButtonText (Graphics &g, TextButton &button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;
 	
	void drawButtonBackground (Graphics& g, Button& button, const Colour& /*backgroundColour*/,
                                           bool isMouseOverButton, bool isButtonDown) override;
 
	Font getAlertWindowFont () override
	{
		return GLOBAL_BOLD_FONT();
	};

	void setColourIdsForAlertWindow(AlertWindow &window)
	{
		window.setColour(AlertWindow::ColourIds::backgroundColourId, dark);
		window.setColour(AlertWindow::ColourIds::textColourId, bright);
		window.setColour(AlertWindow::ColourIds::outlineColourId, bright);
	}

	void drawProgressBar (Graphics &g, ProgressBar &/*progressBar*/, int width, int height, double progress, const String &textToShow) override
	{
		const Colour background = bright;
		const Colour foreground = dark;

		g.fillAll (dark);

		if (progress >= 0.0f && progress < 1.0f)
		{
			ColourGradient grad(bright, 0.0f, 0.0f, bright.withAlpha(0.6f), 0.0f, (float)height, false);

			g.setColour(bright);

			g.drawRect(0, 0, width, height);

			int x = 2;
			int y = 2;
			int w = (int)((width - 4) * progress);
			int h = height - 4;

			g.setGradientFill(grad);

			g.fillRect(x,y,w,h);
		}
		else
		{
			
			
		}

		if (textToShow.isNotEmpty())
		{
			g.setColour (Colour::contrasting (background, foreground));
			g.setFont (GLOBAL_FONT());

			g.drawText (textToShow, 0, 0, width, height, Justification::centred, false);
		}

	}

	void drawAlertBox (Graphics &g, AlertWindow &alert, const Rectangle< int > &textArea, juce::TextLayout &textLayout) override;;

	Colour dark, bright, special;
};



class FrontendKnobLookAndFeel : public LookAndFeel_V3
{
public:

	FrontendKnobLookAndFeel();

	void drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

	void setCustomFilmstripImage(const Image *customImage, int numFilmstrips)
	{
		if (numFilmstrips != 0 && customImage->isValid())
		{
			volumeFilmStrip = customImage->createCopy();
			balanceFilmStrip = volumeFilmStrip;

			numStrips = numFilmstrips;
			useCustomStrip = true;
		}
	}

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
	
	Slider::SliderLayout getSliderLayout(Slider&s) override
	{
		auto layout = LookAndFeel_V3::getSliderLayout(s);

		if (s.getSliderStyle() == Slider::RotaryHorizontalVerticalDrag)
		{
			layout.textBoxBounds = layout.textBoxBounds.withBottomY(s.getHeight()-3);
		}

		return layout;
	}

	Label *createSliderTextBox (Slider &s) override
	{
		Label *label = new Label("Textbox");
		label->setFont (GLOBAL_FONT());
		label->setEditable (false, false, false);
		
		Colour textColour;
		Colour contrastColour;

		if(s.getSliderStyle() == Slider::RotaryHorizontalVerticalDrag)
		{
			
			label->setJustificationType (Justification::centred);
			label->setEditable (false, false, false);
			
			textColour = Colour(0x66ffffff);
			contrastColour = Colours::black;
		}
		else
		{
			
			label->setJustificationType (Justification::centred);
			
			textColour = s.findColour(Slider::textBoxTextColourId);
			contrastColour = textColour.contrasting();
			
		}

		label->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
		label->setColour(Label::textColourId, textColour);
		label->setColour(Label::ColourIds::textWhenEditingColourId, textColour);
		label->setColour(TextEditor::ColourIds::highlightColourId, textColour);
		label->setColour(TextEditor::ColourIds::highlightedTextColourId, contrastColour);
		label->setColour(TextEditor::ColourIds::focusedOutlineColourId, textColour);

		return label;
	}

	static void fillPathHiStyle(Graphics &g, const Path &p, int , int , bool drawBorders = true);;

    static void draw1PixelGrid(Graphics& g, Component* c, Rectangle<int> bounds, Colour lineColour=Colours::white);
    
    
	static Point<float> paintCable(Graphics& g, Rectangle<float> start, Rectangle<float> end, Colour c, float alpha = 1.0f, Colour holeColour = Colour(0xFFAAAAAA), bool returnMidPoint = false, bool useHangingCable=true);;

	static void setTextEditorColours(TextEditor& ed);

	int getSliderThumbRadius(Slider& ) override { return 0; }

	void drawVectorRotaryKnob(Graphics& g, Rectangle<float> area, double value, bool bipolar, bool hover, bool down, bool enabled, float modValue);

	void drawLinearSlider (Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPos*/, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider &s) override
	{
		if (style == Slider::TwoValueHorizontal)
		{
			g.fillAll(s.findColour(Slider::backgroundColourId));

			//width += 18;
			
			const float leftBoxPos = minSliderPos;// *(float)(width - 6.0f) + 3.0f;
			const float rightBoxPos = maxSliderPos;// *(float)(width - 6.0f) + 3.0f;

			Rectangle<float> area(leftBoxPos, 0.0f, rightBoxPos - leftBoxPos, (float)height);

			area.reduce(-1.0f, -1.0f);
			
			g.setColour(s.findColour(Slider::ColourIds::thumbColourId));
			g.fillRect(area);

			g.setColour(s.findColour(Slider::ColourIds::trackColourId));
			g.drawRect(0, 0, width, height, 1);

			g.drawLine(leftBoxPos, 0.0f, leftBoxPos, (float)height, 1.0f);
			g.drawLine(rightBoxPos, 0.0f, rightBoxPos, (float)height, 1.0f);

			g.setColour(s.findColour(Slider::ColourIds::textBoxTextColourId));

			const int decimal = jmax<int>(0, (int)-log10(s.getInterval()));

			const String text = String(s.getMinValue(), decimal) + " - " + String(s.getMaxValue(), decimal);
			g.setFont(GLOBAL_FONT());
			g.drawText(text, 0, 0, width, height, Justification::centred, false);
		}
		else
		{
			height = s.getHeight();
			width = s.getWidth();

			if (!s.isEnabled()) g.setOpacity(0.4f);

			//drawHiBackground(g, 12, 6, width-18, 32);

			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			g.fillAll(s.findColour(Slider::backgroundColourId));

			g.setColour(s.findColour(Slider::ColourIds::thumbColourId));

			if (style == Slider::SliderStyle::LinearBar)
			{
				g.fillRect(0.0f, 0.0f, (float)proportion * (float)width, (float)height);
			}
			else if (style == Slider::SliderStyle::LinearBarVertical)
			{
				g.fillRect(0.0f, (float)(1.0 - proportion)*height, (float)width, (float)(proportion * height));
			}

			g.setColour(s.findColour(Slider::ColourIds::trackColourId));
			g.drawRect(0, 0, width, height, 1);
		}

		

	}


	void drawRotarySlider (Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;
	

	void drawToggleButton (Graphics &g, ToggleButton &b, bool isMouseOverButton, bool /*isButtonDown*/) override;
	
	static const char* smalliKnob_png;
    static const int smalliKnob_pngSize;

	static const char* knobRing_png;
	static const int knobRing_size;

	static const char* toggle_png;
    static const int toggle_pngSize;
	static const char* slider_strip2_png;
    static const int slider_strip2_pngSize;
	static const char* slider2_bipolar_png;
    static const int slider2_bipolar_pngSize;
    
	static void setDefaultColours(Component& c)
	{
		c.setColour(HiseColourScheme::ComponentBackgroundColour, Colours::transparentBlack);
		c.setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
		c.setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
		c.setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
		c.setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
	}

private:
    

    Image cachedImage_smalliKnob_png;
	Image cachedImage_knobRing_png;
	Image cachedImage_toggle_png;
	Image cachedImage_slider_strip2_png;
	Image cachedImage_slider2_bipolar_png;
	
	Image ring_red;
	Image ring_green;
	Image ring_yellow;
	Image ring_blue;
	Image ring_modulated;

	Path ring, ring2;

};

class FilmstripLookAndFeel : public GlobalHiseLookAndFeel
{
public:

	FilmstripLookAndFeel() :
		imageToUse(Image()),
		isVertical(true),
		numStrips(0),
        scaleFactor(1.0f)
	{};

	void setFilmstripImage(const Image &imageToUse_, int numStrips_, bool isVertical_=true)
	{
		imageToUse = imageToUse_;
		numStrips = numStrips_;
		isVertical = isVertical_;

		if (numStrips == 0) return;

		if (isVertical)
		{
			heightOfEachStrip = imageToUse.getHeight() / numStrips;
			widthOfEachStrip = imageToUse.getWidth();
		}
		else
		{
			heightOfEachStrip = imageToUse.getHeight();
			widthOfEachStrip = imageToUse.getWidth() / numStrips;
		}

	};

    void setScaleFactor(float newScaleFactor) noexcept
    {
#ifdef HI_FIXED_SCALEFACTOR_FOR_FILMSTRIPS
		ignoreUnused(newScaleFactor);

		scaleFactor = HI_FIXED_SCALEFACTOR_FOR_FILMSTRIPS;
#else
        scaleFactor = newScaleFactor;
#endif
    }
    
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

	BalanceButtonLookAndFeel()
	{
		cachedImage_balanceKnob_png = ImageCache::getFromMemory (balanceKnob_png, balanceKnob_pngSize);
	}

	void drawRotarySlider (Graphics &g, int /*x*/, int /*y*/, int /*width*/, int /*height*/, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override
	{
		float alphaValue = 1.0f;

		if(!s.isEnabled()) alphaValue *= 0.4f;

		
		

		const double value = s.getValue();

		if(value == 0.0) alphaValue *= 0.66f;

        const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());
		const int stripIndex = (int) (proportion * 63);

        const int offset = stripIndex * 28;
		Image clip = cachedImage_balanceKnob_png.getClippedImage(Rectangle<int>(0, offset, 28, 28));

        g.setColour (Colours::black.withAlpha(alphaValue));
        g.drawImage (clip, 0, 0, 24, 24, 0, 0, 28, 28); 

		

	}

private:
	Image cachedImage_balanceKnob_png;


	static const char* balanceKnob_png;
    static const int balanceKnob_pngSize;
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

	MacroKnobLookAndFeel()
	{
		cachedImage_macroKnob_png = ImageProvider::getImage(ImageProvider::KnobEmpty);// ImageCache::getFromMemory(macroKnob_png, macroKnob_pngSize);
		cachedImage_macroKnob_ring_png = ImageProvider::getImage(ImageProvider::KnobUnmodulated);
	}

	void drawRotarySlider (Graphics &g, int /*x*/, int /*y*/, int /*width*/, int /*height*/, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

	Label *createSliderTextBox (Slider &)
	{


		Label *label = new Label("Textbox");
		label->setFont (GLOBAL_FONT());
		label->setEditable (false, false, false);
		label->setColour (Label::textColourId, Colours::black);
		
		label->setJustificationType (Justification::centred);

		return label;
	}



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
		bool isMouseOverButton, bool isButtonDown)
	{
		

		auto area = button.getLocalBounds().toFloat();

		if (button.getToggleState())
		{
			g.setColour(textColour);
			g.drawRoundedRectangle(area.reduced(1.0f), 4.0f, 1.0f);
		}

		float alpha = 0.2f;

		if (isButtonDown)
			alpha += 0.1f;

		if (isMouseOverButton)
			alpha += 0.1f;

		if (!button.isEnabled())
			alpha = 0.1f;
		

		g.setGradientFill(ColourGradient(Colours::white.withAlpha(alpha + 0.1f), 0.0f, 0.0f,
			Colours::white.withAlpha(alpha), 0.0f, (float)button.getHeight(), false));

		g.fillRoundedRectangle(area, 4.0f);
	}

	void drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool);

	void drawButtonText(Graphics& g, TextButton& b, bool , bool ) override
	{
		float alpha = 1.0f;

		if (!b.isEnabled())
			alpha = 0.5f;

		g.setColour(textColour.withMultipliedAlpha(alpha));
		g.setFont(f);
		g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
	}

	Colour textColour;
	Font f;

private:


	Image ticked;
	Image unticked;
};

struct PeriodicScreenshotter : public Thread
{
    struct Holder
    {
        virtual ~Holder() {};
        virtual PeriodicScreenshotter* getScreenshotter() = 0;
        JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
    };

    PeriodicScreenshotter(Component* c) :
        Thread("screenshotter"),
        comp(c)
    {
        startThread(4);
    };
    
    static void disableForScreenshot(Component* c)
    {
        c->getProperties().set("SkipScreenshot", true);
    }
    
    struct PopupGlassLookAndFeel: public PopupLookAndFeel
    {
        PopupGlassLookAndFeel(Component& c):
            holder(c.findParentComponentOfClass<Holder>())
        {
            jassert(holder != nullptr);
            setColour(PopupMenu::backgroundColourId, Colours::transparentBlack);
        }

        virtual void drawPopupMenuBackgroundWithOptions (Graphics& g,
                                                         int width,
                                                         int height,
                                                         const PopupMenu::Options& o);
        
        
        WeakReference<Holder> holder;
    };

    ~PeriodicScreenshotter()
    {
        stopThread(500);
    }

    struct ScopedPopupDisabler
    {
        ScopedPopupDisabler(Component* r)
        {
            recursive(r);
        }

        ~ScopedPopupDisabler()
        {
            for (const auto& c : skippers)
                c->setSkipPainting(false);
        }

        void recursive(Component* c)
        {
            if (!c->isVisible())
                return;

            if (auto t = c->getProperties()["SkipScreenshot"])
            {
                skippers.add(c);
                c->setSkipPainting(true);
                return;
            }

            for (int i = 0; i < c->getNumChildComponents(); i++)
                recursive(c->getChildComponent(i));
        }

        Array<Component*> skippers;
    };

    void drawGlassSection(Graphics& g, Component* c, Rectangle<int> b, Point<int> offset={});

    void run() override;

    Image img;
    Component* comp;
};

} // namespace hise
