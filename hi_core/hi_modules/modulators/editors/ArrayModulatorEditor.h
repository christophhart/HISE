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

#ifndef __JUCE_HEADER_FE8142B7E6E70A5A__
#define __JUCE_HEADER_FE8142B7E6E70A5A__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ArrayModulatorEditor  : public ProcessorEditorBody
{
public:
    //==============================================================================
    ArrayModulatorEditor (ProcessorEditor *p);
    ~ArrayModulatorEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		sliderPackMix->update();
	}

	int getBodyHeight() const override { return h; }

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<SliderPack> sliderPackMix;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrayModulatorEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_FE8142B7E6E70A5A__
