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

#ifndef __JUCE_HEADER_DB2227C1418B1EA__
#define __JUCE_HEADER_DB2227C1418B1EA__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ReverbEditor  : public ProcessorEditorBody,
                      public SliderListener
{
public:
    //==============================================================================
    ReverbEditor (ProcessorEditor *p);
    ~ReverbEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui()
	{
		wetSlider->updateValue();
		roomSlider->updateValue();
		dampingSlider->updateValue();
		widthSlider->updateValue();
	};

	int getBodyHeight() const override
	{
		return h;
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> wetSlider;
    ScopedPointer<HiSlider> roomSlider;
    ScopedPointer<HiSlider> dampingSlider;
    ScopedPointer<HiSlider> widthSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_DB2227C1418B1EA__
