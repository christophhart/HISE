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

class FrontendEditorHolder: public Component
{
    
};


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

#if USE_TURBO_ACTIVATE
		addAndMakeVisible(resolveLicenceButton = new TextButton("Register Product Key"));
#else
        addAndMakeVisible(resolveLicenceButton = new TextButton("Use Licence File"));
#endif
		addAndMakeVisible(resolveSamplesButton = new TextButton("Choose Sample Folder"));
        addAndMakeVisible(registerProductButton = new TextButton("Online authentication"));
		addAndMakeVisible(ignoreButton = new TextButton("Ignore"));

		resolveLicenceButton->setLookAndFeel(&alaf);
		resolveSamplesButton->setLookAndFeel(&alaf);
        registerProductButton->setLookAndFeel(&alaf);
		ignoreButton->setLookAndFeel(&alaf);

		resolveLicenceButton->addListener(this);
		resolveSamplesButton->addListener(this);
        registerProductButton->addListener(this);
		ignoreButton->addListener(this);
	};

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
		CriticalCustomErrorMessage,
		SamplesNotFound,
		CustomErrorMessage,
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

	bool check(State s, const String &value=String());

	State checkLicence(const String &keyContent=String());

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

	String getTextForError(State s) const;

	void resized()
	{
		if (currentState != 0)
		{
			descriptionLabel->centreWithSize(getWidth() - 20, 150);
		}

		if (currentState[LicenseNotFound] || 
			currentState[LicenseInvalid] || 
			currentState[MachineNumbersNotMatching] || 
			currentState[UserNameNotMatching] || 
			currentState[ProductNotMatching])
		{
			resolveLicenceButton->setVisible(true);
            registerProductButton->setVisible(true);
			resolveSamplesButton->setVisible(false);
			ignoreButton->setVisible(false);

			resolveLicenceButton->centreWithSize(200, 32);
            registerProductButton->centreWithSize(200, 32);
            
            resolveLicenceButton->setTopLeftPosition(registerProductButton->getX(),
                                                      registerProductButton->getY() + 40);
		}
		else if (currentState[SamplesNotFound])
		{
			resolveLicenceButton->setVisible(false);
            registerProductButton->setVisible(false);
			resolveSamplesButton->setVisible(true);
			ignoreButton->setVisible(true);

			resolveSamplesButton->centreWithSize(200, 32);
			ignoreButton->centreWithSize(200, 32);

			ignoreButton->setTopLeftPosition(ignoreButton->getX(),
				resolveSamplesButton->getY() + 40);
		}
		else if (currentState[CustomErrorMessage])
		{
			resolveLicenceButton->setVisible(false);
			registerProductButton->setVisible(false);
			resolveSamplesButton->setVisible(false);
			ignoreButton->setVisible(true);

			ignoreButton->centreWithSize(200, 32);
		}
        
#if USE_TURBO_ACTIVATE
        registerProductButton->setVisible(false);
#endif
	}

private:

	AlertWindowLookAndFeel alaf;

	String customMessage;

	ScopedPointer<Label> descriptionLabel;

	ScopedPointer<TextButton> resolveLicenceButton;
	ScopedPointer<TextButton> resolveSamplesButton;
    ScopedPointer<TextButton> registerProductButton;
	ScopedPointer<TextButton> ignoreButton;

	BigInteger currentState;
	
};


class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer,
							   public ComponentWithKeyboard,
							   public ModalBaseWindow,
							   public OverlayMessageBroadcaster::Listener


{
public:

	FrontendProcessorEditor(FrontendProcessor *fp);;

	~FrontendProcessorEditor();

	void overlayMessageSent(int state, const String& message) override
	{
		if (deactiveOverlay != nullptr)
		{
			deactiveOverlay->setCustomMessage(message);
			deactiveOverlay->setState((DeactiveOverlay::State)state, true);
		}
	}

	void timerCallback()
	{
#if USE_COPY_PROTECTION
		if (!dynamic_cast<FrontendProcessor*>(getAudioProcessor())->unlocker.isUnlocked())
		{
			getAudioProcessor()->suspendProcessing(true);
		}
#endif
	}

	KeyboardFocusTraverser *createFocusTraverser() override { return (keyboard != nullptr && keyboard->isVisible()) ? new MidiKeyboardFocusTraverser() : new KeyboardFocusTraverser(); };

	void paint(Graphics &g) override
	{
		g.fillAll(Colours::black);
	};

    void setGlobalScaleFactor(float newScaleFactor);

	void resized() override;

	void resetInterface()
	{
		//interfaceComponent->checkInterfaces();
	}

	Component *getKeyboard() const override { return keyboard; }

private:

    ScopedPointer<FrontendEditorHolder> container;
    
	friend class BaseFrontendBar;

	ScopedPointer<ScriptContentContainer> interfaceComponent;
	ScopedPointer<BaseFrontendBar> mainBar;
	ScopedPointer<CustomKeyboard> keyboard;
	ScopedPointer<AboutPage> aboutPage;
	ScopedPointer<DeactiveOverlay> deactiveOverlay;
	ScopedPointer<ThreadWithQuasiModalProgressWindow::Overlay> loaderOverlay;
    
	float scaleFactor = 1.0f;

    int originalSizeX = 0;
    int originalSizeY = 0;

    bool overlayToolbar;
};



#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
