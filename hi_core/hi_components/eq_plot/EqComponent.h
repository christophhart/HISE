/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef __MAINCOMPONENT_H_5C0D5756__
#define __MAINCOMPONENT_H_5C0D5756__

class ModulatorSynth;

//==============================================================================
/** @class EqComponent
    @ingroup components

    @brief A component that displays a filter graph of an EQ.

	@todo: add more filters (with a combobox)
*/
class EqComponent   : public Component,
							   public SliderListener
{
public:
    //==============================================================================
    EqComponent(ModulatorSynth *ownerSynth);
    ~EqComponent();

	void sliderValueChanged (Slider* sliderThatWasMoved);
    void paint (Graphics&);
    void resized();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqComponent)

	ScopedPointer<FilterGraph> f;
	ScopedPointer<Slider> gainSlider;
    ScopedPointer<Slider> freqSlider;
    ScopedPointer<Slider> qSlider;

	WeakReference<ModulatorSynth> owner;

};


#endif  // __MAINCOMPONENT_H_5C0D5756__
