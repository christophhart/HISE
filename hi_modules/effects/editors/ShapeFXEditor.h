/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
namespace hise {
using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
	An auto-generated component, created by the Projucer.

	Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ShapeFXEditor  : public ProcessorEditorBody,
                       public Slider::Listener,
                       public ComboBox::Listener,
                       public Button::Listener
{
public:
    //==============================================================================
    ShapeFXEditor (ProcessorEditor* p);
    ~ShapeFXEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		biasLeft->updateValue();
		biasRight->updateValue();
		modeSelector->updateValue();
		highPass->updateValue();
		gainSlider->updateValue();
		reduceSlider->updateValue();
		driveSlider->updateValue();
		mixSlider->updateValue();
		oversampling->updateValue();
		autoGain->updateValue();
	}

	int getBodyHeight() const override { return h; }

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<WaveformComponent> shapeDisplay;
    ScopedPointer<HiSlider> biasLeft;
    ScopedPointer<VuMeter> outMeter;
    ScopedPointer<VuMeter> inMeter;
    ScopedPointer<HiComboBox> modeSelector;
    ScopedPointer<HiSlider> biasRight;
    ScopedPointer<HiSlider> highPass;
    ScopedPointer<HiSlider> gainSlider;
    ScopedPointer<HiSlider> reduceSlider;
    ScopedPointer<HiSlider> driveSlider;
    ScopedPointer<HiSlider> mixSlider;
    ScopedPointer<HiComboBox> oversampling;
    ScopedPointer<HiToggleButton> autoGain;
    ScopedPointer<Label> function;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeFXEditor)
};

//[EndFile] You can add extra defines here...
}
//[/EndFile]
