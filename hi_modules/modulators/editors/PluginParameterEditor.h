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

#ifndef __JUCE_HEADER_29E3F062AED5C8A8__
#define __JUCE_HEADER_29E3F062AED5C8A8__

//[Headers]     -- You can add your own extra header files here --
#define LfoEditor(x) ProcessorEditor(x, new LfoEditorBody(x))
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PluginParameterEditorBody  : public ProcessorEditorBody,
                                   public SliderListener
{
public:
    //==============================================================================
    PluginParameterEditorBody (ProcessorEditor *p);
    ~PluginParameterEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		PluginParameterModulator *pm = static_cast<PluginParameterModulator*>(getProcessor());

		valueSlider->setValue((double)pm->getAttribute(PluginParameterModulator::Value) * 100.0, dontSendNotification);
	};

	void editorInitialized()
	{
		valueSlider->setScrollWheelEnabled(false);

		PluginParameterModulator *pm = static_cast<PluginParameterModulator*>(getProcessor());

		valueSlider->setTextValueSuffix(pm->getSuffix());
	}

	int getBodyHeight() const override
	{
		return 50;
	};

	void sliderDragStarted(Slider *)
	{
		PluginParameterModulator *pm = static_cast<PluginParameterModulator*>(getProcessor());

		int slotIndex = pm->getSlotIndex();

		if(slotIndex != -1) getProcessor()->getMainController()->beginParameterChangeGesture(pm->getSlotIndex());
	};

	void sliderDragEnded(Slider *)
	{
		PluginParameterModulator *pm = static_cast<PluginParameterModulator*>(getProcessor());

		int slotIndex = pm->getSlotIndex();

		if(slotIndex != -1) getProcessor()->getMainController()->endParameterChangeGesture(pm->getSlotIndex());
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> valueSlider;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginParameterEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */

//[/EndFile]

#endif   // __JUCE_HEADER_29E3F062AED5C8A8__
