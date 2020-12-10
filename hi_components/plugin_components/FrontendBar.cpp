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
}

void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenseButton)
	{
#if USE_COPY_PROTECTION
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
		if (currentState[SamplesNotInstalled])
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

				setState(SamplesNotFound, !handler.areSampleReferencesCorrect());
				setState(SamplesNotInstalled, !handler.areSampleReferencesCorrect());
#endif
			}
			else
			{
				setState(SamplesNotFound, true);
			}
		}
	}
	else if (b == registerProductButton)
	{
#if USE_COPY_PROTECTION
		Unlocker::showActivationWindow(this);
#endif
	}
	else if (b == ignoreButton)
	{
		if (currentState[CustomErrorMessage])
		{
			setState(CustomErrorMessage, false);
		}
		if (currentState[CustomInformation])
		{
			setState(CustomInformation, false);
		}
		else if (currentState[SamplesNotFound])
		{
			auto& handler = dynamic_cast<MainController*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor())->getSampleManager().getProjectHandler();

			ignoreUnused(handler);

			// Allows partial sample loading the next time
			FRONTEND_ONLY(handler.setAllSampleReferencesCorrect());

			setState(SamplesNotFound, false);
		}
	}
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



#if !USE_COPY_PROTECTION

bool DeactiveOverlay::check(State s, const String &value/*=String()*/)
{
	ignoreUnused(s, value);
	return true;
}

DeactiveOverlay::State DeactiveOverlay::checkLicense(const String &keyContent)
{
	ignoreUnused(keyContent);
	return numReasons;
}

#endif



String DeactiveOverlay::getTextForError(State s) const
{
	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)";
		break;
	case DeactiveOverlay::SamplesNotFound:
		return "The sample directory could not be located. \nClick below to choose the sample folder.";
		break;
	case DeactiveOverlay::SamplesNotInstalled:
#if HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON && HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		return "Please click below to install the samples from the downloaded archive or point to the location where you've already installed the samples.";
#elif HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON
        return "Please click below to install the samples from the downloaded archive.";
#elif HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		return "Please click below to point to the location where you've already installed the samples.";
#else
		return "This should never show :)";
		jassertfalse;
#endif

		break;
	case DeactiveOverlay::LicenseNotFound:
	{
#if USE_COPY_PROTECTION
#if HISE_ALLOW_OFFLINE_ACTIVATION
		return "This computer is not registered.\nClick below to authenticate this machine using either online authorization or by loading a license key.";
#else

		registerProductButton->triggerClick();

		return "This computer is not registered.";
#endif
#else
		return "";
#endif
	}
	case DeactiveOverlay::ProductNotMatching:
		return "The license key is invalid (wrong plugin name / version).\nClick below to locate the correct license key for this plugin / version";
		break;
	case DeactiveOverlay::MachineNumbersNotMatching:
		return "The machine ID is invalid / not matching.\nThis might be caused by a major OS / or system hardware update which change the identification of this computer.\nIn order to solve the issue, just repeat the activation process again to register this system with the new specifications.";
		break;
	case DeactiveOverlay::UserNameNotMatching:
		return "The user name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case DeactiveOverlay::EmailNotMatching:
		return "The email name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case DeactiveOverlay::LicenseInvalid:
	{
#if USE_COPY_PROTECTION

		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

		return ul->getProductErrorMessage();
#else
		return "";
#endif
	}
	case DeactiveOverlay::LicenseExpired:
	{
#if USE_COPY_PROTECTION
		return "The license key is expired. Press OK to reauthenticate (you'll need to be online for this)";
#else
		return "";
#endif
	}
	case DeactiveOverlay::CustomErrorMessage:
		return customMessage;
	case DeactiveOverlay::CriticalCustomErrorMessage:
		return customMessage;
	case DeactiveOverlay::CustomInformation:
		return customMessage;
	case DeactiveOverlay::numReasons:
		break;
	default:
		break;
	}

	return String();
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

	if (currentState[CustomInformation])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
		ignoreButton->setButtonText("OK");
	}

	if (currentState[CustomErrorMessage])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
		ignoreButton->setButtonText("Ignore");
	}
	

	if (currentState[SamplesNotFound])
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

	if (currentState[SamplesNotInstalled])
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

	if (currentState[LicenseNotFound] ||
		currentState[LicenseInvalid] ||
		currentState[MachineNumbersNotMatching] ||
		currentState[UserNameNotMatching] ||
		currentState[ProductNotMatching])
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
	
	if (currentState[CriticalCustomErrorMessage])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
        installSampleButton->setVisible(false);
		ignoreButton->setVisible(false);
	}
}


} // namespace hise
