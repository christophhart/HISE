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

#ifndef FRONTENDPROCESSOREDITOR_H_INCLUDED
#define FRONTENDPROCESSOREDITOR_H_INCLUDED

#define INCLUDE_BAR 1


class ScriptContentContainer;


class DeactiveOverlay : public Component,
						public ButtonListener
{
public:

	DeactiveOverlay() :
		currentState(0)
	{
		addAndMakeVisible(descriptionLabel = new Label());

		descriptionLabel->setFont(GLOBAL_BOLD_FONT());
		descriptionLabel->setColour(Label::ColourIds::textColourId, Colours::white);
		descriptionLabel->setEditable(false, false, false);
		descriptionLabel->setJustificationType(Justification::centredTop);

		addAndMakeVisible(resolveLicenceButton = new TextButton("Find Licence File"));
		addAndMakeVisible(resolveSamplesButton = new TextButton("Choose Sample Folder"));
        addAndMakeVisible(createMachineIdButton = new TextButton("Show Computer ID"));
        

		resolveLicenceButton->setLookAndFeel(&alaf);
		resolveSamplesButton->setLookAndFeel(&alaf);
        createMachineIdButton->setLookAndFeel(&alaf);

		resolveLicenceButton->addListener(this);
		resolveSamplesButton->addListener(this);
        createMachineIdButton->addListener(this);
	};

	void buttonClicked(Button *b);

	enum State
	{
		AppDataDirectoryNotFound,
		LicenceNotFound,
		ProductNotMatching,
		UserNameNotMatching,
		EmailNotMatching,
		MachineNumbersNotMatching,
		LicenceInvalid,
		SamplesNotFound,
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

	bool check(State s, const String &value=String::empty);

	State checkLicence(const String &keyContent=String::empty);

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
	}

	String getTextForError(State s) const
	{
		switch (s)
		{
		case DeactiveOverlay::AppDataDirectoryNotFound:
			return "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)";
			break;
		case DeactiveOverlay::SamplesNotFound:
			return "The sample directory could not be located. \nClick below to choose the sample folder.";
			break;
		case DeactiveOverlay::LicenceNotFound:
			return "The licence key could not be found.\nClick below to locate the licence key.";
			break;
		case DeactiveOverlay::ProductNotMatching:
			return "The licence key is invalid (wrong plugin name / version).\nClick below to locate the correct licence key for this plugin / version";
			break;
		case DeactiveOverlay::MachineNumbersNotMatching:
			return "The machine ID is invalid / not matching.\nClick below to load the correct licence key for this computer (or request a new licence key for this machine through support.";
			break;
		case DeactiveOverlay::UserNameNotMatching:
			return "The user name is invalid.\nThis means usually a corrupt or rogued licence key file. Please contact support to get a new licence key.";
			break;
		case DeactiveOverlay::EmailNotMatching:
			return "The email name is invalid.\nThis means usually a corrupt or rogued licence key file. Please contact support to get a new licence key.";
			break;
		case DeactiveOverlay::LicenceInvalid:
			return "The licence key is malicious.\nPlease contact support.";
		case DeactiveOverlay::numReasons:
			break;
		default:
			break;
		}

		return String();
	}

	void resized()
	{
		if (currentState != 0)
		{
			descriptionLabel->centreWithSize(getWidth() - 20, 150);
		}

		if (currentState[LicenceNotFound] || 
			currentState[LicenceInvalid] || 
			currentState[MachineNumbersNotMatching] || 
			currentState[UserNameNotMatching] || 
			currentState[ProductNotMatching])
		{
			resolveLicenceButton->setVisible(true);
            createMachineIdButton->setVisible(true);
			resolveSamplesButton->setVisible(false);

			resolveLicenceButton->centreWithSize(200, 32);
            createMachineIdButton->centreWithSize(200, 32);
            
            createMachineIdButton->setTopLeftPosition(createMachineIdButton->getX(),
                                                      createMachineIdButton->getY() + 40);
		}
		else if (currentState[SamplesNotFound])
		{
			resolveLicenceButton->setVisible(false);
            createMachineIdButton->setVisible(false);
			resolveSamplesButton->setVisible(true);

			resolveSamplesButton->centreWithSize(200, 32);
		}
	}

private:

	AlertWindowLookAndFeel alaf;

	ScopedPointer<Label> descriptionLabel;

	ScopedPointer<TextButton> resolveLicenceButton;
	ScopedPointer<TextButton> resolveSamplesButton;
    ScopedPointer<TextButton> createMachineIdButton;

	BigInteger currentState;
	
};


class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer,
							   public ComponentWithKeyboard
{
public:

	FrontendProcessorEditor(FrontendProcessor *fp);;

	void timerCallback()
	{
#if USE_COPY_PROTECTION
		if (!dynamic_cast<FrontendProcessor*>(getAudioProcessor())->unlocker.isUnlocked())
		{
			getAudioProcessor()->suspendProcessing(true);
		}
#endif
	}

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };

	void paint(Graphics &g) override
	{
		g.fillAll(Colours::black);
	};

	void resized() override;

	void resetInterface()
	{
		//interfaceComponent->checkInterfaces();
	}

	Component *getKeyboard() const override { return keyboard; }

private:

	friend class FrontendBar;

	ScopedPointer<ScriptContentContainer> interfaceComponent;

	ScopedPointer<FrontendBar> mainBar;

	ScopedPointer<CustomKeyboard> keyboard;

	ScopedPointer<AboutPage> aboutPage;

	ScopedPointer<DeactiveOverlay> deactiveOverlay;

	ScopedPointer<ThreadWithQuasiModalProgressWindow::Overlay> loaderOverlay;
    
    bool overlayToolbar;
};



#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
