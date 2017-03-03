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
    addAndMakeVisible(container = new FrontendEditorHolder());
    
    
	container->addAndMakeVisible(interfaceComponent = new ScriptContentContainer(fp->getMainSynthChain(), nullptr));
	interfaceComponent->checkInterfaces();

	interfaceComponent->setCurrentContent(0, dontSendNotification);
	interfaceComponent->refreshContentBounds();
	interfaceComponent->setIsFrontendContainer(true);

	fp->addOverlayListener(this);

#if INCLUDE_BAR

	container->addAndMakeVisible(mainBar = BaseFrontendBar::createFrontendBar(fp));

#endif

	container->addAndMakeVisible(keyboard = new CustomKeyboard(fp->getKeyboardState()));

    keyboard->setAvailableRange(fp->getKeyboardState().getLowestKeyToDisplay(), 127);
    
    bool showKeyboard = fp->getToolbarPropertiesObject()->getProperty("keyboard");

    keyboard->setVisible(showKeyboard);

	container->addAndMakeVisible(aboutPage = new AboutPage());
	aboutPage->setVisible(false);

	container->addAndMakeVisible(deactiveOverlay = new DeactiveOverlay());

#if !FRONTEND_IS_PLUGIN
	deactiveOverlay->setState(DeactiveOverlay::SamplesNotFound, !ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory());
#endif

#if USE_COPY_PROTECTION

	if (!fp->unlocker.isUnlocked())
	{
		deactiveOverlay->checkLicence();
	}

	deactiveOverlay->setState(DeactiveOverlay::LicenseInvalid, !fp->unlocker.isUnlocked());

#elif USE_TURBO_ACTIVATE

	deactiveOverlay->setState(DeactiveOverlay::State::LicenseNotFound, !fp->unlocker.licenceWasFound());
	deactiveOverlay->setState(DeactiveOverlay::State::LicenseExpired, fp->unlocker.licenceExpired());
	deactiveOverlay->setState(DeactiveOverlay::State::LicenseInvalid, !fp->unlocker.isUnlocked());

#endif
    
	container->addAndMakeVisible(loaderOverlay = new ThreadWithQuasiModalProgressWindow::Overlay());
    
	loaderOverlay->setDialog(nullptr);
	fp->setOverlay(loaderOverlay);

    overlayToolbar = dynamic_cast<DefaultFrontendBar*>(mainBar.get()) != nullptr && fp->getToolbarPropertiesObject()->getProperty("overlaying");
    
#if HISE_IOS
    const int keyboardHeight = 210;
#else
    const int keyboardHeight = 72;
#endif
    
#if HISE_IOS
    setSize(568, 320);
#else
    
    setSize(interfaceComponent->getContentWidth(), ((mainBar != nullptr && !overlayToolbar) ? mainBar->getHeight() : 0) + interfaceComponent->getContentHeight() + (showKeyboard ? keyboardHeight : 0));
#endif
    
	startTimer(4125);

	originalSizeX = getWidth();
	originalSizeY = getHeight();

	setGlobalScaleFactor(fp->scaleFactor);
}

FrontendProcessorEditor::~FrontendProcessorEditor()
{
	dynamic_cast<OverlayMessageBroadcaster*>(getAudioProcessor())->removeOverlayListener(this);

	mainBar = nullptr;
	interfaceComponent = nullptr;
	keyboard = nullptr;
	aboutPage = nullptr;
	loaderOverlay = nullptr;
}

void FrontendProcessorEditor::setGlobalScaleFactor(float newScaleFactor)
{
    if (newScaleFactor > 0.2 && (scaleFactor != newScaleFactor))
    {
        scaleFactor = newScaleFactor;
        
        AffineTransform scaler = AffineTransform::scale(scaleFactor);
        
        container->setTransform(scaler);
        
        auto tl = findParentComponentOfClass<TopLevelWindow>();
        
        if (tl != nullptr)
        {
            tl->setSize((int)((float)originalSizeX * scaleFactor), (int)((float)originalSizeY * scaleFactor));
        }
        else
        {
            setSize((int)((float)originalSizeX * scaleFactor), (int)((float)originalSizeY * scaleFactor));
        }
    }
}

void FrontendProcessorEditor::resized()
{
	int y = 0;
	
    int width = originalSizeX != 0 ? originalSizeX : getWidth();
    int height = originalSizeY != 0 ? originalSizeY : getHeight();
    
    container->setBounds(0, 0, width, height);
    
	if (mainBar != nullptr)
	{
		mainBar->setBounds(0, y, width, mainBar->getHeight());
		
        if(!overlayToolbar)
            y += mainBar->getHeight();
	}

	interfaceComponent->setBounds(0, y, width, interfaceComponent->getContentHeight());

	y = interfaceComponent->getBottom();

#if HISE_IOS
    const int keyboardHeight = 210;
#else
    const int keyboardHeight = 72;
#endif
    
    if(keyboard->isVisible())
    {
        keyboard->setBounds(0, y, width, keyboardHeight);
    }
	
	aboutPage->setBoundsInset(BorderSize<int>(80));
	
    deactiveOverlay->setBounds(0, 0, width, height);

	loaderOverlay->setBounds(0, 0, width, height);
}

#if USE_COPY_PROTECTION

class OnlineActivator : public ThreadWithAsyncProgressWindow
{
public:

	OnlineActivator(Unlocker* unlocker_, DeactiveOverlay* overlay_):
		ThreadWithAsyncProgressWindow("Online Authentication"),
		unlocker(unlocker_),
		overlay(overlay_)
	{
		addTextEditor("user", unlocker->getUserEmail(), "User Name");
		addTextEditor("serial", "XXXX-XXXX-XXXX-XXXX", "Serial number");


		addBasicComponents();

		showStatusMessage("Please enter your user name and the serial number and press OK");

	}

	void run() override
	{
		URL url = unlocker->getServerAuthenticationURL();

		StringPairArray data;

		StringArray ids = unlocker->getLocalMachineIDs();
		if (ids.size() > 0)
		{
			data.set("user", getTextEditorContents("user"));
			data.set("serial", getTextEditorContents("serial"));
			data.set("machine_id", ids[0]);

			url = url.withParameters(data);

			ScopedPointer<InputStream> uis = url.createInputStream(false, nullptr, nullptr);

			keyFile = uis->readEntireStreamAsString();

		}
	}

	void threadFinished() override
	{
		if (keyFile.startsWith("Keyfile for"))
		{
			ProjectHandler::Frontend::getLicenceKey().replaceWithText(keyFile);

			PresetHandler::showMessageWindow("Authentication successfull", "The authentication was successful and the licence key was stored.\nPlease reload this plugin to finish the authentication.", PresetHandler::IconType::Info);

			unlocker->applyKeyFile(keyFile);

			DeactiveOverlay::State returnValue = overlay->checkLicence(keyFile);

			if (returnValue != DeactiveOverlay::State::numReasons) return;

			

			overlay->setState(DeactiveOverlay::State::LicenseNotFound, false);
			overlay->setState(DeactiveOverlay::State::LicenseInvalid, !unlocker->isUnlocked());

		}
		else
		{
			PresetHandler::showMessageWindow("Authentication failed", keyFile, PresetHandler::IconType::Error);
		}
	}

private:

	String keyFile;

	Unlocker* unlocker;
	DeactiveOverlay* overlay;

};

#endif

void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenceButton)
	{
#if USE_COPY_PROTECTION
		FileChooser fc("Load Licence key file", File(), "*" + ProjectHandler::Frontend::getLicenceKeyExtension(), true);

		if (fc.browseForFileToOpen())
		{
			File f = fc.getResult();

			String keyContent = f.loadFileAsString();

			Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
			ul->applyKeyFile(keyContent);

			State returnValue = checkLicence(keyContent);

			if (returnValue == numReasons || returnValue == LicenseNotFound)
			{
				f.copyFileTo(ProjectHandler::Frontend::getLicenceKey());

				returnValue = checkLicence();
			}
			
            refreshLabel();
            
			if (returnValue != numReasons) return;

			setState(LicenseInvalid, !ul->isUnlocked());
            
            PresetHandler::showMessageWindow("Valid key file found.", "You found a valid key file. Please reload this instance to activate the plugin.");
		}
#elif USE_TURBO_ACTIVATE
        
		



        const String key = PresetHandler::getCustomName("Product Key", "Enter the product key that you've received along with the download link\nIt should have this format: XXXX-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX");
        
        TurboActivateUnlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
        
		setCustomMessage("Waiting for activation");
		setState(DeactiveOverlay::State::CustomErrorMessage, true);

		
        ul->activateWithKey(key.getCharPointer());
        
		setState(DeactiveOverlay::CustomErrorMessage, false);
		setCustomMessage(String());

        setState(DeactiveOverlay::State::LicenseNotFound, !ul->licenceWasFound());
		setState(DeactiveOverlay::State::LicenseExpired, ul->unlockState == TurboActivateUnlocker::State::Deactivated);
        setState(DeactiveOverlay::State::LicenseInvalid, !ul->isUnlocked());
        
        if(ul->isUnlocked())
        {
			PresetHandler::showMessageWindow("Registration successful", "The software is now unlocked and ready to use.");
            
            FrontendProcessor* fp =  dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
            
            fp->loadSamplesAfterRegistration();
        }
#endif
	}
	else if (b == resolveSamplesButton)
	{
		FileChooser fc("Select Sample Location", ProjectHandler::Frontend::getSampleLocationForCompiledPlugin(), "*.*", true);

		if (fc.browseForDirectory())
		{
			ProjectHandler::Frontend::setSampleLocation(fc.getResult());

			const bool directorySelected = ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory();

			if (directorySelected)
			{
				FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());

				fp->checkAllSampleReferences();

				if (fp->areSampleReferencesCorrect())
				{
					PresetHandler::showMessageWindow("Sample Folder changed", "The sample folder was relocated, but you'll need to open a new instance of this plugin before it can be used.");
				}
				
				setState(SamplesNotFound, !fp->areSampleReferencesCorrect());
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
        Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		OnlineActivator* activator = new OnlineActivator(ul, this);
		activator->setModalBaseWindowComponent(this);
#endif
    }
	else if (b == ignoreButton)
	{
		if (currentState[CustomErrorMessage])
		{
			setState(CustomErrorMessage, false);
		}
		else if (currentState[SamplesNotFound])
		{
			setState(SamplesNotFound, false);
		}
	}
}

bool DeactiveOverlay::check(State s, const String &value/*=String()*/)
{
#if USE_COPY_PROTECTION
	Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

	String compareValue;

	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return ProjectHandler::Frontend::getAppDataDirectory().isDirectory();
		break;
	case DeactiveOverlay::LicenseNotFound:
		return ProjectHandler::Frontend::getLicenceKey().existsAsFile();
		break;
	case DeactiveOverlay::ProductNotMatching:
		compareValue = ProjectHandler::Frontend::getProjectName() + String(" ") + ProjectHandler::Frontend::getVersionString();
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
	case DeactiveOverlay::LicenseInvalid:
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
#if USE_COPY_PROTECTION
	String key = keyContent;

	if (keyContent.isEmpty())
	{
		key = ProjectHandler::Frontend::getLicenceKey().loadFileAsString();
	}

	StringArray lines = StringArray::fromLines(key);

	if (!check(AppDataDirectoryNotFound) || !check(LicenseNotFound) || lines.size() < 4)
	{
		setState(LicenseNotFound, true);
		return LicenseNotFound;
	}

	currentState.setBit(LicenseNotFound, false);

	const String productName = lines[0].fromFirstOccurrenceOf("Keyfile for ", false, false);
	CHECK_LICENCE_PARAMETER(ProductNotMatching, productName);

	const String keyFileMachineId = lines[3].fromFirstOccurrenceOf("Machine numbers: ", false, false);
	CHECK_LICENCE_PARAMETER(MachineNumbersNotMatching, keyFileMachineId);

    return numReasons;
#else
	ignoreUnused(keyContent);
	return numReasons;
#endif
}

String DeactiveOverlay::getTextForError(State s) const
{
	if (customMessage.isNotEmpty())
	{
		if (s == DeactiveOverlay::numReasons)
			return String();
		else
		{
			return customMessage;
		}
	}

	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)";
		break;
	case DeactiveOverlay::SamplesNotFound:
		return "The sample directory could not be located. \nClick below to choose the sample folder.";
		break;
	case DeactiveOverlay::LicenseNotFound:
	{
#if USE_COPY_PROTECTION
		return "This computer is not registered.\nClick below to authenticate this machine using either online authorization or by loading a license key.";
		break;
#elif USE_TURBO_ACTIVATE
		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		return ul->getErrorMessage();
#else
		return "";
#endif
	}
		
	case DeactiveOverlay::ProductNotMatching:
		return "The license key is invalid (wrong plugin name / version).\nClick below to locate the correct license key for this plugin / version";
		break;
	case DeactiveOverlay::MachineNumbersNotMatching:
		return "The machine ID is invalid / not matching.\nClick below to load the correct license key for this computer (or request a new license key for this machine through support.";
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
		return "The license key is malicious.\nPlease contact support.";
#elif USE_TURBO_ACTIVATE
		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		return ul->getErrorMessage();
#else
		return "";
#endif
	}
	case DeactiveOverlay::LicenseExpired:
	{
#if USE_COPY_PROTECTION
		return "The license key is expired. Press OK to reauthenticate (you'll need to be online for this)";
#elif USE_TURBO_ACTIVATE
		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		return ul->getErrorMessage();
#else
		return "";
#endif
	}
	case DeactiveOverlay::numReasons:
		break;
	default:
		break;
	}

	return String();
}

#undef CHECK_LICENCE_PARAMETER