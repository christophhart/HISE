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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


FrontendProcessorEditor::FrontendProcessorEditor(FrontendProcessor *fp) :
AudioProcessorEditor(fp)
{
    addAndMakeVisible(container = new FrontendEditorHolder());
    

	container->addAndMakeVisible(rootTile = new FloatingTile(fp, nullptr));

	rootTile->setNewContent("InterfacePanel");
    
	fp->addOverlayListener(this);
	
	container->addAndMakeVisible(deactiveOverlay = new DeactiveOverlay());

#if !FRONTEND_IS_PLUGIN
	deactiveOverlay->setState(DeactiveOverlay::SamplesNotFound, !fp->areSamplesLoadedCorrectly());
#endif

#if USE_COPY_PROTECTION

	if (!fp->unlocker.isUnlocked())
	{
		deactiveOverlay->checkLicence();
	}

	deactiveOverlay->setState(DeactiveOverlay::LicenseInvalid, !fp->unlocker.isUnlocked());

#elif USE_TURBO_ACTIVATE

	deactiveOverlay->setState(DeactiveOverlay::State::CopyProtectionError, !fp->unlocker.isUnlocked());

#endif
    
	container->addAndMakeVisible(loaderOverlay = new ThreadWithQuasiModalProgressWindow::Overlay());
    
	loaderOverlay->setDialog(nullptr);
	fp->setOverlay(loaderOverlay);

    addChildComponent(debugLoggerComponent = new DebugLoggerComponent(&fp->getDebugLogger()));

	debugLoggerComponent->setVisible(fp->getDebugLogger().isLogging());

	auto jsp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(fp);
    
    if(jsp != nullptr)
    {
        setSize(jsp->getScriptingContent()->getContentWidth(), jsp->getScriptingContent()->getContentHeight());

    }
    
	
	startTimer(4125);

	originalSizeX = getWidth();
	originalSizeY = getHeight();

	const int availableHeight = Desktop::getInstance().getDisplays().getMainDisplay().userArea.getHeight();
	const float displayScaleFactor = (float)Desktop::getInstance().getDisplays().getMainDisplay().scale;
	const int unscaledInterfaceHeight = getHeight();

	if (displayScaleFactor == 1.0f && availableHeight > 0 && (availableHeight - unscaledInterfaceHeight < 40))
	{
		setGlobalScaleFactor(0.85f);
	}
	else
	{
		setGlobalScaleFactor((float)fp->getGlobalScaleFactor());
	}
}

FrontendProcessorEditor::~FrontendProcessorEditor()
{
	dynamic_cast<OverlayMessageBroadcaster*>(getAudioProcessor())->removeOverlayListener(this);

	loaderOverlay = nullptr;
	debugLoggerComponent = nullptr;
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
	int width = originalSizeX != 0 ? originalSizeX : getWidth();
    int height = originalSizeY != 0 ? originalSizeY : getHeight();
    
    container->setBounds(0, 0, width, height);
	rootTile->setBounds(0, 0, width, height);
    deactiveOverlay->setBounds(0, 0, width, height);
	loaderOverlay->setBounds(0, 0, width, height);
	debugLoggerComponent->setBounds(0, height-90, width, 90);
}

#if USE_COPY_PROTECTION

OnlineActivator::OnlineActivator(Unlocker* unlocker_, DeactiveOverlay* overlay_) :
	ThreadWithAsyncProgressWindow("Online Authentication"),
	unlocker(unlocker_),
	overlay(overlay_)
{
	addTextEditor("user", unlocker->getUserEmail(), "Email Adress");
	addTextEditor("serial", "XXXX-XXXX-XXXX-XXXX", "Serial number");


	addBasicComponents();

	showStatusMessage("Please enter your user name and the serial number and press OK");
}


void OnlineActivator::run()
{
	URL url = unlocker->getServerAuthenticationURL();

	StringPairArray data;

	StringArray ids = unlocker->getLocalMachineIDs();

	if (ids.size() > 0)
	{
		auto email = getTextEditorContents("user");

		//URL::isProbablyAnEmailAddress(email)

		data.set("user", email);
		data.set("serial", getTextEditorContents("serial"));
		data.set("machine_id", ids[0]);

		url = url.withParameters(data);

		ScopedPointer<InputStream> uis = url.createInputStream(false, nullptr, nullptr);

		if (uis != nullptr)
		{
			keyFile = uis->readEntireStreamAsString();

			if (keyFile.startsWith("Keyfile for"))
			{
				error = OK;
				
			}
			else
			{
				error = Fail;
				errorMessage = keyFile;

			}
		}
		else
		{
			error = noConnection;
			errorMessage = "No connection to server";
		}
	}
}

void OnlineActivator::threadFinished()
{
	if (error == OK)
	{
		ProjectHandler::Frontend::getLicenceKey().replaceWithText(keyFile);

		PresetHandler::showMessageWindow("Authentication successfull", "The authentication was successful and the licence key was stored.\nPlease reload this plugin to finish the authentication.", PresetHandler::IconType::Info);

		unlocker->applyKeyFile(keyFile);

		DeactiveOverlay::State returnValue = overlay->checkLicence(keyFile);

		if (returnValue != DeactiveOverlay::State::numReasons) return;

		overlay->setState(DeactiveOverlay::State::LicenseNotFound, false);
		overlay->setState(DeactiveOverlay::State::LicenseInvalid, !unlocker->isUnlocked());

	}
	else if (error == noConnection)
	{
		auto ids = unlocker->getLocalMachineIDs();

		auto id = ids[0];

		if (id.isNotEmpty())
		{
			PresetHandler::showMessageWindow("No connection", "Authentification failed because the server could not be reached.\nIf you want to authorize this machine from another computer, use this ID: " + id, PresetHandler::IconType::Info);
		}
	}
	else
	{
		PresetHandler::showMessageWindow("Authentication failed", errorMessage, PresetHandler::IconType::Error);
	}
}

#endif