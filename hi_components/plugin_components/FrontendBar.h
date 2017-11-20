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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef __FRONTENDBAR_H_INCLUDED
#define __FRONTENDBAR_H_INCLUDED

namespace hise { using namespace juce;

class BaseFrontendBar : public Component
{
public:

	virtual ~BaseFrontendBar() {};

	static BaseFrontendBar* createFrontendBar(MainController* mc);
};

#define CREATE_FRONTEND_BAR(ClassName) BaseFrontendBar* BaseFrontendBar::createFrontendBar(MainController* mc) { return new ClassName(mc); }



/** The bar that is displayed for every FrontendProcessorEditor */
class DefaultFrontendBar  : public BaseFrontendBar,
                     public Timer,
					 public SliderListener,
					 public SettableTooltipClient,
				     public ButtonListener
{
public:
    
	// ================================================================================================================

    DefaultFrontendBar (MainController *p);
    ~DefaultFrontendBar();

	void sliderValueChanged(Slider* slider) override;

	void setProperties(DynamicObject *p);
	static String createJSONString(DynamicObject *p=nullptr);
	static DynamicObject *createDefaultProperties();

	// ================================================================================================================

	void timerCallback();
    void paint (Graphics& g);
    void resized();
	void buttonClicked(Button* b) override;

	bool isOverlaying() const { return overlaying; }

	static BaseFrontendBar* createFrontendBar(MainController* mc)
	{
		return new DefaultFrontendBar(mc);
	}

private:

	const Image getFilmStripImageFromString(const String &fileReference) const;

	// ================================================================================================================

	
	MainController *mc;

	bool overlaying;

	int height;

	Colour bgColour;

	FrontendKnobLookAndFeel fklaf;

	ScopedPointer<Label> volumeSliderLabel;
	ScopedPointer<Label> pitchSliderLabel;
	ScopedPointer<Label> balanceSliderLabel;

	ScopedPointer<Slider> volumeSlider;
	ScopedPointer<Slider> balanceSlider;
	ScopedPointer<Slider> pitchSlider;

	ScopedPointer<VoiceCpuBpmComponent> voiceCpuComponent;

	UpdateMerger cpuUpdater;
	ScopedPointer<TooltipBar> tooltipBar;

	ScopedPointer<PresetBox> presetSelector;
	

	ScopedPointer<VuMeter> outMeter;

	ScopedPointer<ShapeButton> deviceSettingsButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DefaultFrontendBar)
};

class SampleDataImporter;


class DeactiveOverlay : public Component,
	public ButtonListener
{
public:

	DeactiveOverlay();;

	void buttonClicked(Button *b);

	enum State
	{
		AppDataDirectoryNotFound,
		LicenseNotFound,
		ProductNotMatching,
		UserNameNotMatching,
		EmailNotMatching,
		MachineNumbersNotMatching,
		LicenseExpired,
		LicenseInvalid,
		CopyProtectionError,
		CriticalCustomErrorMessage,
		SamplesNotInstalled,
		SamplesNotFound,
		CustomErrorMessage,
		CustomInformation,
		numReasons
	};

	void paint(Graphics &g)
	{
		g.setColour(Colours::black.withAlpha(0.8f));
		g.fillAll();
	}

	void setState(State s, bool value)
	{
		currentState.setBit(s, value);

		setVisible(currentState != 0);
		refreshLabel();
		resized();
	}

	void setCustomMessage(const String newCustomMessage)
	{
		customMessage = newCustomMessage;
	}

	bool check(State s, const String &value = String());

	State checkLicense(const String &keyContent = String());

	void refreshLabel()
	{
		for (int i = 0; i < numReasons; i++)
		{
			if (currentState[i])
			{
				descriptionLabel->setText(getTextForError((State)i), dontSendNotification);
				return;
			}
		}

		resized();
	}

	String getTextForError(State s) const;

	void resized();

private:

	AlertWindowLookAndFeel alaf;

	String customMessage;

	ScopedPointer<Label> descriptionLabel;

	ScopedPointer<TextButton> resolveLicenseButton;
	ScopedPointer<TextButton> installSampleButton;
	ScopedPointer<TextButton> resolveSamplesButton;
	ScopedPointer<TextButton> registerProductButton;
	ScopedPointer<TextButton> useActivationResponseButton;
	ScopedPointer<TextButton> ignoreButton;

	BigInteger currentState;

};

} // namespace hise

#endif  

