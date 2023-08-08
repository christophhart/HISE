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

#ifndef __JUCE_HEADER_9A1036EE3AE7F8AC__
#define __JUCE_HEADER_9A1036EE3AE7F8AC__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;

#define TableEnvelopeEditor(x) ModulatorEditor(x, new TableEnvelopeEditorBody(x))


class HiButtonLookAndFeel: public LookAndFeel_V3
{
	Font getTextButtonFont(TextButton &, int ) override
	{
		return GLOBAL_FONT().withHeight(11.0f);
	};

	void drawButtonBackground (Graphics &g, Button &, const Colour &/*backgroundColour*/, bool /*isMouseOverButton*/, bool isButtonDown) override
	{
		g.setColour(isButtonDown ? Colours::darkred : Colours::grey);

		g.fillRoundedRectangle((float)(g.getClipBounds().getTopLeft().getX() + 1),
							   (float)(g.getClipBounds().getTopLeft().getX() + 1),
							   (float)(g.getClipBounds().getWidth() - 2),
							   (float)(g.getClipBounds().getHeight() - 2),
							   1.0f);

		g.setColour(Colours::black);

		g.drawRoundedRectangle((float)(g.getClipBounds().getTopLeft().getX() + 1),
							   (float)(g.getClipBounds().getTopLeft().getX() + 1),
							   (float)(g.getClipBounds().getWidth() - 2),
							   (float)(g.getClipBounds().getHeight() - 2),
							   1.0f,
							   0.5f);



		//GlowEffect *c = new GlowEffect();
		//c->setGlowProperties(4.0f, Colours::white.withAlpha(0.1f));

		//b.setComponentEffect(c);


	};

	int 	getTextButtonWidthToFitText (TextButton &, int buttonHeight) {return buttonHeight;};

	void 	drawButtonText (Graphics &g, TextButton &b, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
	{
		g.setColour(Colours::black);
		g.setFont(getTextButtonFont(b, 10));
		g.drawText(b.getButtonText(), g.getClipBounds(), Justification::centred, false);

	};

	/*void 	drawToggleButton (Graphics &, ToggleButton &, bool isMouseOverButton, bool isButtonDown) {};*/

	/*void 	changeToggleButtonWidthToFitText (ToggleButton &) {};*/

	/*void 	drawTickBox (Graphics &, Component &, float x, float y, float w, float h, bool ticked, bool isEnabled, bool isMouseOverButton, bool isButtonDown) {};*/

	/*void 	drawDrawableButton (Graphics &, DrawableButton &, bool isMouseOverButton, bool isButtonDown) {};*/
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
class TableEnvelopeEditorBody  : public ProcessorEditorBody,
                                 public Timer,
                                 public SliderListener
{
public:
    //==============================================================================
    TableEnvelopeEditorBody (ProcessorEditor *p);
    ~TableEnvelopeEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui()
	{

		attackSlider->updateValue();
		releaseSlider->updateValue();

		/*
		const float value = getProcessor()->getDisplayValues().inL;

		const bool release = getProcessor()->getDisplayValues().inR < 0.0f;

		if(value == 0.0f)
		{
			attackTable->setDisplayedIndex(0.0f);
			releaseTable->setDisplayedIndex(0.0f);
			return;
		}

		TableEditor *editorToUse = release ? releaseTable : attackTable;

		editorToUse->setDisplayedIndex(abs(value));
		*/

	}

	void timerCallback() override
	{
		attackSlider->setDisplayValue(getProcessor()->getChildProcessor(TableEnvelope::AttackChain)->getOutputValue());
		releaseSlider->setDisplayValue(getProcessor()->getChildProcessor(TableEnvelope::ReleaseChain)->getOutputValue());
	}

	int getBodyHeight() const { return h; };

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	HiButtonLookAndFeel buttonLookAndFeel;

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> attackSlider;
    ScopedPointer<HiSlider> releaseSlider;
    ScopedPointer<TableEditor> attackTable;
    ScopedPointer<TableEditor> releaseTable;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableEnvelopeEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_9A1036EE3AE7F8AC__
