/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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
