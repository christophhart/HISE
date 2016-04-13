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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef __JUCE_HEADER_59561CDF26DED86E__
#define __JUCE_HEADER_59561CDF26DED86E__

class VuMeter;


class VoiceCounterCpuUsageComponent : public Component,
									  public Timer,
									  public ButtonListener
{
public:

	VoiceCounterCpuUsageComponent(MainController *mc_);

	void buttonClicked(Button *b) override;

	void timerCallback() override;

	void resized() override;

	void paint(Graphics& g) override;

	void paintOverChildren(Graphics& g) override;

private:

	ScopedPointer<ShapeButton> panicButton;

	ScopedPointer<Label> voiceLabel;

	ScopedPointer<VuMeter> cpuSlider;

	MainController *mc;
};



/** The bar that is displayed for every FrontendProcessorEditor */
class FrontendBar  : public Component,
                     public Timer,
                     public ButtonListener,
					 public SliderListener,
					 public SettableTooltipClient
{
public:
    
    FrontendBar (MainController *p);
    ~FrontendBar();

	void buttonClicked(Button *b) override;

	void refreshPresetFileList();

	void sliderValueChanged(Slider* slider) override;

	void timerCallback();

    void paint (Graphics& g);
    void resized();

private:

	MainController *mc;

	FrontendKnobLookAndFeel fklaf;

	ScopedPointer<Label> volumeSliderLabel;
	ScopedPointer<Label> pitchSliderLabel;
	ScopedPointer<Label> balanceSliderLabel;

	ScopedPointer<Slider> volumeSlider;
	ScopedPointer<Slider> balanceSlider;
	ScopedPointer<Slider> pitchSlider;

	ScopedPointer<VoiceCounterCpuUsageComponent> voiceCpuComponent;

	UpdateMerger cpuUpdater;
	ScopedPointer<TooltipBar> tooltipBar;

	ScopedPointer<ComboBox> presetSelector;
	ScopedPointer<ShapeButton> presetSaveButton;

    //==============================================================================
    ScopedPointer<VuMeter> outMeter;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrontendBar)
};


#endif  
