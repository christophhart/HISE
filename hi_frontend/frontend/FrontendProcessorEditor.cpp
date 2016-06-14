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


FrontendProcessorEditor::FrontendProcessorEditor(FrontendProcessor *fp) :
AudioProcessorEditor(fp)
{

	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(fp->getMainSynthChain(), nullptr));
	interfaceComponent->checkInterfaces();

	interfaceComponent->setCurrentContent(0, dontSendNotification);
	interfaceComponent->refreshContentBounds();
	interfaceComponent->setIsFrontendContainer(true);

#if INCLUDE_BAR

	addAndMakeVisible(mainBar = new FrontendBar(fp));

#endif

	addAndMakeVisible(keyboard = new CustomKeyboard(fp->getKeyboardState()));

    keyboard->setAvailableRange(fp->getKeyboardState().getLowestKeyToDisplay(), 127);
    
	

	

	addAndMakeVisible(aboutPage = new AboutPage());
	aboutPage->setVisible(false);

	addAndMakeVisible(deactiveOverlay = new DeactiveOverlay());

	deactiveOverlay->setState(DeactiveOverlay::SamplesNotFound, !ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory());

#if USE_COPY_PROTECTION

	if (!fp->unlocker.isUnlocked())
	{
		deactiveOverlay->checkLicence();
	}

	deactiveOverlay->setState(DeactiveOverlay::LicenceInvalid, !fp->unlocker.isUnlocked());
#endif

	addAndMakeVisible(loaderOverlay = new ThreadWithQuasiModalProgressWindow::Overlay());
	loaderOverlay->setDialog(nullptr);
	fp->setOverlay(loaderOverlay);

	setSize(interfaceComponent->getContentWidth(), (mainBar != nullptr ? mainBar->getHeight() : 0) + interfaceComponent->getContentHeight() + 72);

	startTimer(4125);

}

void FrontendProcessorEditor::resized()
{
	int y = 0;
	
	if (mainBar != nullptr)
	{
		mainBar->setBounds(0, y, getWidth(), mainBar->getHeight());
		y += mainBar->getHeight();
	}

	interfaceComponent->setBounds(0, y, getWidth(), interfaceComponent->getContentHeight());

	y = interfaceComponent->getBottom();

	keyboard->setBounds(0, y, getWidth(), 72);

	aboutPage->setBoundsInset(BorderSize<int>(80));
	deactiveOverlay->setBounds(getLocalBounds());
	loaderOverlay->setBounds(getLocalBounds());
}


void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenceButton)
	{
#if USE_COPY_PROTECTION
		FileChooser fc("Load Licence key file", File::nonexistent, "*" + ProjectHandler::Frontend::getLicenceKeyExtension(), true);

		if (fc.browseForFileToOpen())
		{
			File f = fc.getResult();

			String keyContent = f.loadFileAsString();

			Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
			ul->applyKeyFile(keyContent);

			State returnValue = checkLicence(keyContent);

			if (returnValue == numReasons || returnValue == LicenceNotFound)
			{
				f.copyFileTo(ProjectHandler::Frontend::getLicenceKey());

				returnValue = checkLicence();
			}
			
            refreshLabel();
            
			if (returnValue != numReasons) return;

			setState(LicenceInvalid, !ul->isUnlocked());
            
            PresetHandler::showMessageWindow("Valid key file found.", "You found a valid key file. Please reload this instance to activate the plugin.");
		}
#endif
	}
	else if (b == resolveSamplesButton)
	{
		FileChooser fc("Select Sample Location", File::nonexistent, "*.*", true);

		if (fc.browseForDirectory())
		{
			ProjectHandler::Frontend::setSampleLocation(fc.getResult());

			const bool directorySelected = ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory();

			if (directorySelected)
			{
				PresetHandler::showMessageWindow("Sample Folder changed", "The sample folder was relocated, but you'll need to open a new instance of this plugin before it can be used.");
			}

			setState(SamplesNotFound, !directorySelected);
		}
	}
    else if (b == createMachineIdButton)
    {
#if USE_COPY_PROTECTION
        Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
        
        StringArray machineIds = ul->getLocalMachineIDs();
        
        if(machineIds.size() > 0)
        {
            SystemClipboard::copyTextToClipboard(machineIds[0]);
            
            PresetHandler::showMessageWindow("Computer ID", "Use this ID to obtain a licence key:\n\n" +machineIds[0] + "\n\nThis ID is copied to the clipboard. If this computer is not connected to the internet, write it down somewhere and use it with another computer that has internet access.");
        }
#endif
    }
}

bool DeactiveOverlay::check(State s, const String &value/*=String::empty*/)
{
#if USE_COPY_PROTECTION
	Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

	String compareValue;

	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return ProjectHandler::Frontend::getAppDataDirectory().isDirectory();
		break;
	case DeactiveOverlay::LicenceNotFound:
		return ProjectHandler::Frontend::getLicenceKey().existsAsFile();
		break;
	case DeactiveOverlay::ProductNotMatching:
		compareValue = String(JucePlugin_Name) + String(" ") + String(JucePlugin_VersionString);
		return value == compareValue;
		break;
	case DeactiveOverlay::UserNameNotMatching:
		break;
	case DeactiveOverlay::EmailNotMatching:
		break;
	case DeactiveOverlay::MachineNumbersNotMatching:
	{
		StringArray ids = ul->getLocalMachineIDs();
		return ids.contains(value);
		break;
	}
	case DeactiveOverlay::LicenceInvalid:
		return ul->isUnlocked();
		break;
	case DeactiveOverlay::SamplesNotFound:
		break;
	case DeactiveOverlay::numReasons:
		break;
	default:
		break;
	}

#else

	ignoreUnused(s, value);

#endif
    
	return true;
}


#define CHECK_LICENCE_PARAMETER(state, text){\
if (!check(state, text))\
{ setState(state, true); return state; }\
currentState.setBit(state, false);}


DeactiveOverlay::State DeactiveOverlay::checkLicence(const String &keyContent)
{
	String key = keyContent;

	if (keyContent.isEmpty())
	{
		key = ProjectHandler::Frontend::getLicenceKey().loadFileAsString();
	}

	StringArray lines = StringArray::fromLines(key);

	if (!check(AppDataDirectoryNotFound) || !check(LicenceNotFound) || lines.size() < 4)
	{
		setState(LicenceNotFound, true);
		return LicenceNotFound;
	}

	currentState.setBit(LicenceNotFound, false);

	const String productName = lines[0].fromFirstOccurrenceOf("Keyfile for ", false, false);
	CHECK_LICENCE_PARAMETER(ProductNotMatching, productName);

	const String keyFileMachineId = lines[3].fromFirstOccurrenceOf("Machine numbers: ", false, false);
	CHECK_LICENCE_PARAMETER(MachineNumbersNotMatching, keyFileMachineId);

    return numReasons;
}

#undef CHECK_LICENCE_PARAMETER