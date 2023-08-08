/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_30EE194281C5F822__
#define __JUCE_HEADER_30EE194281C5F822__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;

class WaveformComponent;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SineSynthBody  : public ProcessorEditorBody,
                       public SliderListener,
                       public LabelListener,
                       public ButtonListener
{
public:
    //==============================================================================
    SineSynthBody (ProcessorEditor *p);
    ~SineSynthBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override { return h; };

	void updateGui() override;;

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void labelTextChanged (Label* labelThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> octaveSlider;
    ScopedPointer<Label> fadeTimeLabel;
    ScopedPointer<Label> voiceAmountLabel;
    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> fadeTimeEditor;
    ScopedPointer<HiSlider> semiToneSlider;
    ScopedPointer<HiToggleButton> musicalRatio;
    ScopedPointer<HiSlider> saturationSlider;
    ScopedPointer<WaveformComponent> waveDisplay;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SineSynthBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_30EE194281C5F822__
