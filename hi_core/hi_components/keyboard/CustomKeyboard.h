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

#ifndef __JUCE_HEADER_BD6D51421EADA068__
#define __JUCE_HEADER_BD6D51421EADA068__

class MidiKeyboardFocusTraverser : public KeyboardFocusTraverser
{
	Component *getDefaultComponent(Component *parentComponent) override;
};

class ComponentWithMidiKeyboardTraverser : public Component
{
public:

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };
};






class CustomKeyboard : public MidiKeyboardComponent,
					   public SafeChangeListener,
					   public ButtonListener
					   
{
public:
    //==============================================================================
    CustomKeyboard (CustomKeyboardState &state);
    virtual ~CustomKeyboard();

	void buttonClicked(Button *b) override
	{
		if (b->getName() == "OctaveUp")
		{
			setLowestVisibleKey(getLowestVisibleKey() + 12);
		}
		else
		{
			setLowestVisibleKey(getLowestVisibleKey() - 12);
		}
	}

	void paint(Graphics &g) override
	{
		MidiKeyboardComponent::paint(g);
		g.setColour(Colours::darkred);
		g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 3.0f);

		g.setGradientFill (ColourGradient (Colour (0x7d000000),
										   0.0f, 80.0f,
										   Colour (0x00008000),
										   5.0f, 80.0f,
										   false));
		g.fillRect (0, 0, 16, proportionOfHeight (1.0000f));

		g.setGradientFill (ColourGradient (Colour (0x7d000000),
										   (float)getWidth(), 80.0f,
										   Colour (0x00008000),
										   (float)getWidth()-5.0f, 80.0f,
										   false));
		g.fillRect (getWidth()-16, 0, 16, proportionOfHeight (1.0000f));

	};

    // Binary resources:
    static const char* black_key_off_png;
    static const int black_key_off_pngSize;
    static const char* black_key_on_png;
    static const int black_key_on_pngSize;
    static const char* white_key_off_png;
    static const int white_key_off_pngSize;
    static const char* white_key_on_png;
    static const int white_key_on_pngSize;

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		repaint();
	}

    void setNarrowKeys(bool shouldBeNarrowKeys);
	
protected:

	void drawWhiteNote (int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour) override;

	void drawBlackNote (int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour) override;

private:


	CustomKeyboardState *state;
 
    bool narrowKeys;
    
    //==============================================================================
    Image cachedImage_black_key_off_png;
    Image cachedImage_black_key_on_png;
    Image cachedImage_white_key_off_png;
    Image cachedImage_white_key_on_png;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomKeyboard)
};

class ComponentWithKeyboard
{
public:
    
    virtual ~ComponentWithKeyboard() {};
    
    virtual Component *getKeyboard() const = 0;
};


#endif   
