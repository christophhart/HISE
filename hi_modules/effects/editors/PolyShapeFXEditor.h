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
class PolyShapeFXEditor  : public ProcessorEditorBody,
                           public Timer,
                           public ComboBox::Listener,
                           public Slider::Listener,
                           public Button::Listener
{
public:
    //==============================================================================
    PolyShapeFXEditor (ProcessorEditor* p);
    ~PolyShapeFXEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void updateGui() override
	{
		driveSlider->updateValue();
		overSampling->updateValue();
		modeSelector->updateValue();
		bias->updateValue();

		auto mode = getProcessor()->getAttribute(PolyshapeFX::SpecialParameters::Mode);

		table->setVisible(mode == ShapeFX::ShapeMode::Curve);
		table2->setVisible(mode == ShapeFX::ShapeMode::AsymetricalCurve);
	}

	int getBodyHeight() const override { return h; }

	void timerCallback() override
	{
		const float displayValue = (float)getProcessor()->getChildProcessor(PolyshapeFX::DriveModulation)->getOutputValue();

		driveSlider->setDisplayValue(displayValue);
	}
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<WaveformComponent> shapeDisplay;
    ScopedPointer<HiComboBox> modeSelector;
    ScopedPointer<HiSlider> driveSlider;
    ScopedPointer<HiToggleButton> overSampling;
    ScopedPointer<TableEditor> table;
    ScopedPointer<HiSlider> bias;
    ScopedPointer<TableEditor> table2;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyShapeFXEditor)
};

//[EndFile] You can add extra defines here...
}
//[/EndFile]
