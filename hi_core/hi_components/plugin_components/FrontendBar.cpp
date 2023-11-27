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

namespace hise { using namespace juce;

DeactiveOverlay::DeactiveOverlay(MainController* mc) :
	ControlledObject(mc),
	currentState(0)
{
	alaf = PresetHandler::createAlertWindowLookAndFeel();

	addAndMakeVisible(descriptionLabel = new Label());

	descriptionLabel->setFont(alaf->getAlertWindowMessageFont());
	descriptionLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	descriptionLabel->setEditable(false, false, false);
	descriptionLabel->setJustificationType(Justification::centredTop);


	addAndMakeVisible(resolveLicenseButton = new TextButton("Use License File"));
	addAndMakeVisible(registerProductButton = new TextButton("Activate this computer"));
	addAndMakeVisible(resolveSamplesButton = new TextButton("Choose Sample Folder"));

	addAndMakeVisible(installSampleButton = new TextButton("Install Samples"));

	addAndMakeVisible(ignoreButton = new TextButton("Ignore"));

	resolveLicenseButton->setLookAndFeel(alaf);
	resolveSamplesButton->setLookAndFeel(alaf);
	registerProductButton->setLookAndFeel(alaf);
	ignoreButton->setLookAndFeel(alaf);
	installSampleButton->setLookAndFeel(alaf);

	resolveLicenseButton->addListener(this);
	resolveSamplesButton->addListener(this);
	registerProductButton->addListener(this);
	ignoreButton->addListener(this);
	installSampleButton->addListener(this);

	getMainController()->addOverlayListener(this);
}

DeactiveOverlay::~DeactiveOverlay()
{
	getMainController()->removeOverlayListener(this);
}

void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenseButton)
	{
#if USE_COPY_PROTECTION && !USE_SCRIPT_COPY_PROTECTION
		Unlocker::resolveLicenseFile(this);
#endif
	}
	else if (b == installSampleButton)
	{
#if USE_FRONTEND
		auto fpe = findParentComponentOfClass<FrontendProcessorEditor>();

		auto l = new SampleDataImporter(fpe);

		l->setModalBaseWindowComponent(fpe);
#endif
	}
	else if (b == resolveSamplesButton)
	{
		if (currentState[State::SamplesNotInstalled])
		{
#if HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON
			bool ask = !PresetHandler::showYesNoWindow("Have you installed the samples yet", "Use this only if you have previously installed and extracted all samples from the .hr1 file.\nIf you don't have installed them yet, press cancel to open the sample install dialogue instead");
#else
			bool ask = false;
#endif

			if (ask)
			{
#if USE_FRONTEND
				auto fpe = findParentComponentOfClass<FrontendProcessorEditor>();

				auto l = new SampleDataImporter(fpe);

				l->setModalBaseWindowComponent(fpe);
				return;
#endif
			}
		}
		
		FileChooser fc("Select Sample Location", FrontendHandler::getSampleLocationForCompiledPlugin(), "*.*", true);

		if (fc.browseForDirectory())
		{
			FrontendHandler::setSampleLocation(fc.getResult());

			const bool directorySelected = FrontendHandler::getSampleLocationForCompiledPlugin().isDirectory();

			if (directorySelected)
			{
#if USE_FRONTEND
				auto& handler = dynamic_cast<MainController*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor())->getSampleManager().getProjectHandler();

				handler.checkAllSampleReferences();

				if (handler.areSampleReferencesCorrect())
				{
					PresetHandler::showMessageWindow("Sample Folder changed", "The sample folder was relocated, but you might need to open a new instance of this plugin before it can be used.");
				}

				setStateInternal(State::SamplesNotFound, !handler.areSampleReferencesCorrect());
				setStateInternal(State::SamplesNotInstalled, !handler.areSampleReferencesCorrect());
#endif
			}
			else
			{
				setStateInternal(State::SamplesNotFound, true);
			}
		}
	}
	else if (b == registerProductButton)
	{
#if USE_COPY_PROTECTION  && !USE_SCRIPT_COPY_PROTECTION
		Unlocker::showActivationWindow(this);
#endif
	}
	else if (b == ignoreButton)
	{
		if (currentState[State::CustomErrorMessage])
		{
			setStateInternal(State::CustomErrorMessage, false);
		}
		if (currentState[State::CustomInformation])
		{
			setStateInternal(State::CustomInformation, false);
		}
		else if (currentState[State::SamplesNotFound])
		{
			auto& handler = dynamic_cast<MainController*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor())->getSampleManager().getProjectHandler();

			ignoreUnused(handler);

			// Allows partial sample loading the next time
			FRONTEND_ONLY(handler.setAllSampleReferencesCorrect());

			setStateInternal(State::SamplesNotFound, false);
		}
	}
}

void DeactiveOverlay::paint(Graphics& g)
{
	g.fillAll();
	g.drawImageAt(img, 0, 0);
	g.setColour(Colours::black.withAlpha(0.66f * (float)(numFramesLeft) / 10.0f));
	g.fillAll();
}

void DeactiveOverlay::timerCallback()
{
	numFramesLeft -= 1;

	if (numFramesLeft <= 0)
	{
		stopTimer();
		setVisible(false);
		originalImage = Image();
		return;
	}

	triggerAsyncUpdate();
}

void DeactiveOverlay::fadeout()
{
	numFramesLeft = 1;
	startTimer(30);
}

void DeactiveOverlay::handleAsyncUpdate()
{
	rebuildImage();
	repaint();
}

void DeactiveOverlay::rebuildImage()
{
	auto rebuildFromParent = !originalImage.isValid();

	if (rebuildFromParent)
	{
		if (auto pc = getParentComponent())
		{
			setVisible(false);
			originalImage = pc->createComponentSnapshot(pc->getLocalBounds());
			setVisible(true);
		}
	}

	img = originalImage.createCopy();

	if (img.isValid())
	{
		PostGraphicsRenderer r(stack, img);

		if (rebuildFromParent)
		{
			r.addNoise(0.1f * (float)numFramesLeft / 10.0f);
			r.gaussianBlur(6);
		}

		r.boxBlur(numFramesLeft);

		if(rebuildFromParent)
			r.addNoise(0.03f);
	}
}

void DeactiveOverlay::overlayMessageSent(int state, const String& message)
{
	if (state == OverlayMessageBroadcaster::CustomErrorMessage ||
		state == OverlayMessageBroadcaster::CustomInformation ||
		state == OverlayMessageBroadcaster::CriticalCustomErrorMessage)
	{
		customMessage = message;
	}

	setStateInternal((State)state, true);
}

#if !USE_COPY_PROTECTION
bool DeactiveOverlay::check(State s, const String& value/*=String()*/)
{
	ignoreUnused(s, value);
	return true;
}


DeactiveOverlay::State DeactiveOverlay::checkLicense(const String &keyContent)
{
	ignoreUnused(keyContent);
	return State::numReasons;
}
#endif

void DeactiveOverlay::refreshLabel()
{
	if (currentState == 0)
	{
		descriptionLabel->setText("", dontSendNotification);
	}

	for (int i = 0; i < State::numReasons; i++)
	{
		if (currentState[i])
		{
			descriptionLabel->setText(getTextForError((State)i), dontSendNotification);
			return;
		}
	}

	resized();
}


String DeactiveOverlay::getTextForError(State s) const
{
	if (s == State::CustomInformation ||
		s == State::CustomErrorMessage ||
		s == State::CriticalCustomErrorMessage)
	{
		return customMessage;
	}

	return getMainController()->getOverlayTextMessage(s);
}

void DeactiveOverlay::resized()
{
	installSampleButton->setVisible(false);
	
	if (auto pc = getParentComponent())
	{
		if (isVisible())
		{
			triggerAsyncUpdate();
		}
	}

	if (currentState != 0)
	{
		descriptionLabel->centreWithSize(getWidth() - 20, 150);
	}

	if (currentState[State::CustomInformation])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
		ignoreButton->setButtonText("OK");
	}

	if (currentState[State::CustomErrorMessage])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
		ignoreButton->setButtonText("Ignore");
	}
	

	if (currentState[State::SamplesNotFound])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);

		

		resolveSamplesButton->setVisible(true);
		ignoreButton->setVisible(true);

		resolveSamplesButton->centreWithSize(200, 32);
		ignoreButton->centreWithSize(200, 32);

		ignoreButton->setTopLeftPosition(ignoreButton->getX(),
			resolveSamplesButton->getY() + 40);

		ignoreButton->setButtonText("Ignore");
	}

	if (currentState[State::SamplesNotInstalled])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);

		auto b = getLocalBounds().withSizeKeepingCentre(200, 50);
        ignoreUnused(b);

#if HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON
		installSampleButton->setVisible(true);
		installSampleButton->setBounds(b.removeFromTop(32));
#else
		installSampleButton->setVisible(false);
#endif

#if HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		resolveSamplesButton->setVisible(true);
		resolveSamplesButton->setBounds(b.removeFromBottom(32));
#else
		resolveSamplesButton->setVisible(false);
#endif
		
		ignoreButton->setVisible(false);
	}

	if (currentState[State::LicenseNotFound] ||
		currentState[State::LicenseInvalid] ||
		currentState[State::MachineNumbersNotMatching] ||
		currentState[State::UserNameNotMatching] ||
		currentState[State::ProductNotMatching])
	{
#if HISE_ALLOW_OFFLINE_ACTIVATION
		resolveLicenseButton->setVisible(true);
#else
		resolveLicenseButton->setVisible(false);
#endif

		registerProductButton->setVisible(true);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(false);
        installSampleButton->setVisible(false);
        
		resolveLicenseButton->centreWithSize(200, 32);
		registerProductButton->centreWithSize(200, 32);

		resolveLicenseButton->setTopLeftPosition(registerProductButton->getX(),
			registerProductButton->getY() + 40);
	}
	
	if (currentState[State::CriticalCustomErrorMessage])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
        installSampleButton->setVisible(false);
		ignoreButton->setVisible(false);
	}
}


void DeactiveOverlay::clearState(State s)
{
	setStateInternal(s, false);
}

void DeactiveOverlay::setStateInternal(State s, bool value)
{
	bool wasVisible = currentState != 0;

#if !HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON && !HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
	if (s == OverlayMessageBroadcaster::SamplesNotInstalled && value)
		return;
#endif

	currentState.setBit(s, value);

	if (!wasVisible && currentState != 0)
	{
		numFramesLeft = 10;
		setVisible(true);
		refreshLabel();
		resized();
	}

	if (wasVisible && currentState == 0)
	{
		refreshLabel();
		fadeout();
		resized();
	}

	if (wasVisible && currentState != 0)
	{
		refreshLabel();
		resized();
	}

	if (!wasVisible && currentState == 0)
	{
		setVisible(false);
		refreshLabel();
		resized();
	}
}

} // namespace hise
