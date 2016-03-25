/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================

*/



#include "EqComponent.h"


//==============================================================================
EqComponent::EqComponent(ModulatorSynth *ownerSynth)
{
    setSize (500, 400);
	
	f = new FilterGraph(3);
	f->addFilter(FilterType::LowPass);
	

	addAndMakeVisible(f);
	f->setBounds(2,0, getWidth()-5, 150);

	addAndMakeVisible (gainSlider = new Slider ("Gain 1"));
    gainSlider->setRange (-18, 18, 0.1);
	gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gainSlider->addListener (this);
	gainSlider->setTextValueSuffix(" dB");
	ownerSynth->getMainController()->skin(*gainSlider);

    addAndMakeVisible (freqSlider = new Slider ("Freq 1"));
    freqSlider->setRange (20, 20000, 1);
    freqSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freqSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freqSlider->addListener (this);
	ownerSynth->getMainController()->skin(*freqSlider);
	freqSlider->setTextValueSuffix(" Hz");
	freqSlider->setSkewFactorFromMidPoint(1000.0);

    addAndMakeVisible (qSlider = new Slider ("Q 1"));
    qSlider->setRange (0.1, 5, 0.1);
    qSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    qSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    qSlider->addListener (this);
	ownerSynth->getMainController()->skin(*qSlider);
	qSlider->setSkewFactorFromMidPoint(1.0);

	gainSlider->setBounds (40, 168, 128, 48);
	freqSlider->setBounds (gainSlider->getRight(), 168, 128, 48);
    qSlider->setBounds (freqSlider->getRight(), 168, 128, 48);

}

void EqComponent::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
		//f->setEqBand(0, 44100.0, freqSlider->getValue(), qSlider->getValue(), (float)gainSlider->getValue(), BandType::HighShelf);
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == freqSlider)
    {
		//f->setEqBand(0, 44100.0, freqSlider->getValue(), qSlider->getValue(), (float)gainSlider->getValue(), BandType::HighShelf);
        //[UserSliderCode_freqSlider] -- add your slider handling code here..

		f->setFilter(0, 44100, freqSlider->getValue(), FilterType::LowPass);

        //[/UserSliderCode_freqSlider]
    }
    else if (sliderThatWasMoved == qSlider)
    {
        //[UserSliderCode_qSlider] -- add your slider handling code here..

		

		//f->setEqBand(0, 44100.0, freqSlider->getValue(), qSlider->getValue(), (float)gainSlider->getValue(), BandType::HighShelf);
        //[/UserSliderCode_qSlider]
    }

	

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

EqComponent::~EqComponent()
{
}

void EqComponent::paint (Graphics& )
{
  
}

void EqComponent::resized()
{
    // This is called when the EqComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

