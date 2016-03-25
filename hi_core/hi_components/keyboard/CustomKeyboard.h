
#ifndef __JUCE_HEADER_BD6D51421EADA068__
#define __JUCE_HEADER_BD6D51421EADA068__



class CustomKeyboard : public MidiKeyboardComponent,
					   public SafeChangeListener
					   
{
public:
    //==============================================================================
    CustomKeyboard (CustomKeyboardState &state);
    virtual ~CustomKeyboard();

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



#endif   
