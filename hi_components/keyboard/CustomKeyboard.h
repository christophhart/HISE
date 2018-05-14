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

#ifndef __JUCE_HEADER_BD6D51421EADA068__
#define __JUCE_HEADER_BD6D51421EADA068__

namespace hise { using namespace juce;

class MidiKeyboardFocusTraverser : public KeyboardFocusTraverser
{
	Component *getDefaultComponent(Component *parentComponent) override;
};

class ComponentWithMidiKeyboardTraverser : public Component
{
public:

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };
};




class CustomKeyboardLookAndFeel: public LookAndFeel_V3
{
public:

	CustomKeyboardLookAndFeel();

	virtual void drawKeyboardBackground(Graphics &g, int width, int height);

	virtual void drawWhiteNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour);
	virtual void drawBlackNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour);

	// Binary resources:
	static const char* black_key_off_png;
	static const int black_key_off_pngSize;
	static const char* black_key_on_png;
	static const int black_key_on_pngSize;
	static const char* white_key_off_png;
	static const int white_key_off_pngSize;
	static const char* white_key_on_png;
	static const int white_key_on_pngSize;

private:

	//==============================================================================
	Image cachedImage_black_key_off_png;
	Image cachedImage_black_key_on_png;
	Image cachedImage_white_key_off_png;
	Image cachedImage_white_key_on_png;

};


class CustomKeyboard : public MidiKeyboardComponent,
					   public SafeChangeListener,
					   public ButtonListener
					   
{
public:
    //==============================================================================
    CustomKeyboard (MainController* mc);
    virtual ~CustomKeyboard();

	void buttonClicked(Button *b) override
	{
		if (b->getName() == "OctaveUp")
		{
            lowKey += 12;
        }
            
		else
		{
            lowKey -= 12;
        }
		
        setAvailableRange(lowKey, lowKey + 19);
	}

	void paint(Graphics &g) override
	{
		MidiKeyboardComponent::paint(g);
		
		if(!useCustomGraphics)
			dynamic_cast<CustomKeyboardLookAndFeel*>(&getLookAndFeel())->drawKeyboardBackground(g, getWidth(), getHeight());
	};


	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		
		repaint();
	}

	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void setUseCustomGraphics(bool shouldUseCustomGraphics);

	void setShowOctaveNumber(bool shouldDisplayOctaveNumber) { displayOctaveNumber = shouldDisplayOctaveNumber; }

	int getLowKey() const { return lowKey; }
	int getHiKey() const { return hiKey; }
	void setRange(int lowKey_, int hiKey_)
	{
		lowKey = jlimit<int>(0, 100, lowKey_);
		hiKey = jlimit<int>(10, 128, hiKey_);

		setAvailableRange(lowKey, hiKey);
	}


	bool isUsingCustomGraphics() const noexcept { return useCustomGraphics; };

	bool isShowingOctaveNumbers() const { return displayOctaveNumber; }

	void setEnableToggleMode(bool shouldBeEnabled)
	{
		toggleMode = shouldBeEnabled;
	}

	bool isToggleModeEnabled() const { return toggleMode; };

protected:

	void drawWhiteNote (int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour) override;

	void drawBlackNote (int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour) override;

private:

	ReferenceCountedArray<PoolEntry<Image>> upImages;
	ReferenceCountedArray<PoolEntry<Image>> downImages;

	MainController* mc;

	CustomKeyboardLookAndFeel laf;

	CustomKeyboardState *state;
 
	bool useCustomGraphics = false;

    bool narrowKeys;
    
    int lowKey = 12;
	int hiKey = 127;

	bool displayOctaveNumber = false;

	bool toggleMode = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomKeyboard)
};

class ComponentWithKeyboard
{
public:
    
    virtual ~ComponentWithKeyboard() {};
    
    virtual Component *getKeyboard() const = 0;
};


} // namespace hise

#endif   
