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
                       public Timer,
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

	void timerCallback() override
	{
		// Returns a rectangle because of lazyness
		auto values = dynamic_cast<ShapeFX*>(getProcessor())->getPeakValues();

		inMeter->setPeak(values.getX(), values.getY());
		outMeter->setPeak(values.getWidth(), values.getHeight());
	}

	void updateGui() override
	{
		biasLeft->updateValue();
		biasRight->updateValue();
		modeSelector->updateValue();
		highPass->updateValue();
		gainSlider->updateValue();
		reduceSlider->updateValue();
		lowPass->updateValue();
		mixSlider->updateValue();
		oversampling->updateValue();
		autoGain->updateValue();
		limitButton->updateValue();

		auto m = (ShapeFX::ShapeMode)(int)getProcessor()->getAttribute(ShapeFX::SpecialParameters::Mode);

		table->setVisible(m == ShapeFX::Curve);

#if HI_USE_SHAPE_FX_SCRIPTING
		editor->setVisible(m == ShapeFX::Script || m == ShapeFX::CachedScript);
#endif

		refreshBodySize();
	}

	int getBodyHeight() const override 
	{ 
#if HI_USE_SHAPE_FX_SCRIPTING
		return (table->isVisible() || editor->isVisible()) ? 600 : 330; 
#else
		return table->isVisible() ? 600 : 330;
#endif
	}

#if HI_USE_SHAPE_FX_SCRIPTING
	bool keyPressed(const KeyPress& key) override
	{
		if (key.isKeyCode(KeyPress::F5Key))
		{
			dynamic_cast<ShapeFX*>(getProcessor())->compileScript();
			return true;
		}

		return false;
	}
#endif

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;

#if HI_USE_SHAPE_FX_SCRIPTING
	ScopedPointer<JavascriptTokeniser> tokeniser;
#endif
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
    ScopedPointer<HiSlider> mixSlider;
    ScopedPointer<HiComboBox> oversampling;
    ScopedPointer<HiToggleButton> autoGain;
    ScopedPointer<HiSlider> lowPass;
    ScopedPointer<TableEditor> table;

#if HI_USE_SHAPE_FX_SCRIPTING
    ScopedPointer<JavascriptCodeEditor> editor;
#endif

    ScopedPointer<HiToggleButton> limitButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeFXEditor)
};

//[EndFile] You can add extra defines here...
}
//[/EndFile]
