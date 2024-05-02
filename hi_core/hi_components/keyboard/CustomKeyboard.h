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
public:

	struct ParentWithKeyboardFocus
	{
		virtual ~ParentWithKeyboardFocus() {};
	};

	Component *getDefaultComponent(Component *parentComponent) override;
};

class ComponentWithMidiKeyboardTraverser : public Component
{
public:

	std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override { return std::make_unique<MidiKeyboardFocusTraverser>(); };
};


class CustomKeyboardLookAndFeelBase
{
public:

	CustomKeyboardLookAndFeelBase();

	virtual ~CustomKeyboardLookAndFeelBase() {}
	
	virtual void drawKeyboardBackground(Graphics &g, Component* c, int width, int height);

	virtual void drawWhiteNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour);
	virtual void drawBlackNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour);

	bool useFlatStyle = false;

	Colour bgColour;
	Colour topLineColour;
	Colour overlayColour;
	Colour activityColour;
};

class CustomKeyboardLookAndFeel: public CustomKeyboardLookAndFeelBase,
							     public LookAndFeel_V3
{};

class KeyboardBase
{
public:

	virtual bool isMPEKeyboard() const = 0;

	virtual void setLowestKeyBase(int lowKey) = 0;

	Component* asComponent() { return dynamic_cast<Component*>(this); }
	const Component* asComponent() const { return dynamic_cast<const Component*>(this); }

	virtual float getKeyWidthBase() const = 0;
	virtual bool isShowingOctaveNumbers() const = 0;
	virtual int getRangeStartBase() const = 0;
	virtual int getRangeEndBase() const = 0;
	virtual bool isUsingCustomGraphics() const { return false; }
	virtual double getBlackNoteLengthProportionBase() const = 0;
	virtual bool isToggleModeEnabled() const { return false; }
	virtual int getMidiChannelBase() const = 0;

	
	virtual void setUseCustomGraphics(bool) = 0;
	virtual void setRangeBase(int min, int max) = 0;
	virtual void setKeyWidthBase(float w) = 0;

	virtual void setShowOctaveNumber(bool /*shouldShow*/) {};
	virtual void setBlackNoteLengthProportionBase(float /*ratio*/) {};
	virtual void setEnableToggleMode(bool /*isOn*/) {};
	virtual void setMidiChannelBase(int /*midiChannel*/) = 0;

	virtual void setUseVectorGraphics(bool shouldUseVectorGraphics, bool useFlatStyle) { ignoreUnused(shouldUseVectorGraphics, useFlatStyle); }
	virtual bool isUsingVectorGraphics() const { return true; };
	virtual bool isUsingFlatStyle() const { return false; };

	virtual ~KeyboardBase() {};
};

class CustomKeyboard : public MidiKeyboardComponent,
					   public SafeChangeListener,
					   public ButtonListener,
					   public KeyboardBase
					   
{
public:
    //==============================================================================
    CustomKeyboard (MainController* mc);
    virtual ~CustomKeyboard();

	using CustomClickCallback = std::function<bool(const MouseEvent&, bool)>;

	void buttonClicked(Button *b) override;
    void paint(Graphics &g) override;;
	void changeListenerCallback(SafeChangeBroadcaster *) override;

    void mouseDown(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	bool isMPEKeyboard() const override { return false; }
	bool isUsingCustomGraphics() const noexcept override { return useCustomGraphics; };
	void setUseCustomGraphics(bool shouldUseCustomGraphics) override;

	void setShowOctaveNumber(bool shouldDisplayOctaveNumber) override { displayOctaveNumber = shouldDisplayOctaveNumber; }
	bool isShowingOctaveNumbers() const override { return displayOctaveNumber; }

	void setLowestKeyBase(int lowKey_) override { setLowestVisibleKey(lowKey_); }

	float getKeyWidthBase() const override { return getKeyWidth(); };
	void setKeyWidthBase(float w) override { setKeyWidth(w); }

	int getRangeStartBase() const override { return lowKey; };
	int getRangeEndBase() const override { return hiKey; };

	int getMidiChannelBase() const override { return getMidiChannel(); }
	void setMidiChannelBase(int newChannel) override;

    int getLowKey() const { return lowKey; }
	int getHiKey() const { return hiKey; }
	
	void setRangeBase(int min, int max) override { setRange(min, max); }

	void setBlackNoteLengthProportionBase(float ratio) override { setBlackNoteLengthProportion(ratio); }
	double getBlackNoteLengthProportionBase() const override { return getBlackNoteLengthProportion(); }

	bool isToggleModeEnabled() const override { return toggleMode; };
	void setEnableToggleMode(bool shouldBeEnabled) override { toggleMode = shouldBeEnabled; }

	void setUseVectorGraphics(bool shouldUseVectorGraphics, bool useFlatStyle=false) override;

	bool isUsingVectorGraphics() const override { return true; }

	bool isUsingFlatStyle() const override;

	void setRange(int lowKey_, int hiKey_);

    Rectangle<float> getRectangleForKeyPublic(int midiNoteNumber)
	{
		return getRectangleForKey(midiNoteNumber);
	}
	
	/** Set a custom click callback. Pass in a lambda with the signature
		
		bool f(const MouseEvent& e, bool isMouseDown)

		that returns true if the event was consumed or if it should perform
		the default functionality.
	*/
	void setCustomClickCallback(const CustomClickCallback& f);

protected:

	void drawWhiteNote (int midiNoteNumber, Graphics &g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour) override;
	void drawBlackNote (int midiNoteNumber, Graphics &g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour) override;

private:

	CustomClickCallback ccc;

	Array<PooledImage> upImages;
	Array<PooledImage> downImages;

	MainController* mc;

	ScopedPointer<LookAndFeel> ownedLaf;

	CustomKeyboardState *state;
 
	bool useCustomGraphics = false;
	bool useVectorGraphics = false;

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
