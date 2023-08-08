/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_B9A18C827CE3BAF0__
#define __JUCE_HEADER_B9A18C827CE3BAF0__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;

class KeyGraph: public Component,
				public SafeChangeListener
{
public:

	KeyGraph(Table *t):
		table(t),
		currentKey(-1)
	{

	}

	void resized()
	{
		updatePath();
	}

	void changeListenerCallback(SafeChangeBroadcaster *)
	{
		updatePath();
		repaint();
	}

	void updatePath()
	{
		float x = 0.0f;
		float y = 0.0f;
		const float w = (float)getWidth();
		const float h = (float)getHeight();
		const float deltaX = w / 127.0f;

		path.clear();

		path.startNewSubPath(x, h);

		for(int i = 0; i < 128; i++)
		{

			y = (1.0f - table->getReadPointer()[i]) * h;


			path.lineTo(x, y);

			x += deltaX;
		}

		path.lineTo(w,h);
		path.closeSubPath();
	}

	void setCurrentKey(int newKey) noexcept
	{
		currentKey = newKey;
		repaint();
	};

	void paint(Graphics &g)
	{
		g.setColour(Colours::lightgrey.withAlpha(0.3f));
		g.strokePath (path, PathStrokeType(1.0f));

		g.setColour(Colours::lightgrey.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);

		g.setGradientFill (ColourGradient (Colour (0x77999999),
										   384.0f, 144.0f,
										   Colour (0x11777777),
										   408.0f, 520.0f,
										   false));

		g.setGradientFill (ColourGradient (Colour (0x95c6c6c6),
										0.0f, 0.0f,
										Colour (0x976c6c6c),
										0.0f, static_cast<float> (proportionOfHeight (1.0000f)),
										false));

		g.fillPath(path);

		if(currentKey != -1)
		{
			const float value = currentKey / 127.0f;

			g.setColour(Colours::lightgrey.withAlpha(0.05f));
			g.fillRect(jmax(0.0f, value * (float)getWidth()-5.0f), 0.0f, value == 0.0f ? 5.0f : 10.0f, (float)getHeight());
			g.setColour(Colours::white.withAlpha(0.6f));
			g.drawLine(Line<float>(value * (float)getWidth(), 0.0f, value * (float)getWidth(), (float)getHeight()), 0.5f);
		}

	}

private:

	int currentKey;

	Path path;

	WeakReference<Table> table;

};









//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class KeyEditor  : public ProcessorEditorBody,
                   public SliderListener,
                   public ButtonListener
{
public:
    //==============================================================================
    KeyEditor (ProcessorEditor *p);
    ~KeyEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.


	
	void updateGui() override
	{
	};

	int getBodyHeight() const override { return h; };

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TableEditor> midiTable;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_B9A18C827CE3BAF0__
