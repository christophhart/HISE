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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

FrontendProcessorEditor::FrontendProcessorEditor(FrontendProcessor *fp) :
AudioProcessorEditor(fp)
{
	LOG_START("Creating Interface")

    addAndMakeVisible(container = new FrontendEditorHolder());
    

	container->addAndMakeVisible(rootTile = new FloatingTile(fp, nullptr));

	LOG_START("Creating Root Panel");

	rootTile->setNewContent("InterfacePanel");
    
	fp->addOverlayListener(this);
	
	container->addAndMakeVisible(deactiveOverlay = new DeactiveOverlay());



#if !FRONTEND_IS_PLUGIN && !HISE_IOS


	deactiveOverlay->setState(DeactiveOverlay::SamplesNotInstalled, !ProjectHandler::Frontend::checkSamplesCorrectlyInstalled());

	deactiveOverlay->setState(DeactiveOverlay::SamplesNotFound, !fp->areSamplesLoadedCorrectly());
#else

	// make sure to call setState at least once or the overlay will be visible...
	deactiveOverlay->setState(DeactiveOverlay::SamplesNotFound, false);

#endif

#if USE_COPY_PROTECTION

	if (!fp->unlocker.isUnlocked())
	{
		deactiveOverlay->checkLicense();
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

    
#if HISE_IOS
    
    const float iosScaleFactor = (float)availableHeight / (float)originalSizeY;
    
    setGlobalScaleFactor(iosScaleFactor);
    
#else
    
	if (displayScaleFactor == 1.0f && availableHeight > 0 && (availableHeight - unscaledInterfaceHeight < 40))
	{
		setGlobalScaleFactor(0.85f);
	}
	else
	{
        setGlobalScaleFactor((float)fp->getGlobalScaleFactor());
	}
#endif
}

FrontendProcessorEditor::~FrontendProcessorEditor()
{
	dynamic_cast<OverlayMessageBroadcaster*>(getAudioProcessor())->removeOverlayListener(this);

	container->removeChildComponent(rootTile);

	rootTile = nullptr;
	container = nullptr;

	loaderOverlay = nullptr;
	debugLoggerComponent = nullptr;
}

void FrontendProcessorEditor::setGlobalScaleFactor(float newScaleFactor)
{
	LOG_START("Change scale factor");

    if (newScaleFactor > 0.2 && (scaleFactor != newScaleFactor))
    {
        scaleFactor = newScaleFactor;
        
        AffineTransform scaler = AffineTransform::scale(scaleFactor);
        
        container->setTransform(scaler);
        
        auto tl = findParentComponentOfClass<FrontendStandaloneApplication::AudioWrapper>();
        
        if (tl != nullptr)
        {
            tl->setSize((int)((float)originalSizeX * scaleFactor), (int)((float)originalSizeY * scaleFactor));
			setSize((int)((float)originalSizeX * scaleFactor), (int)((float)originalSizeY * scaleFactor));
        }
        else
        {
            setSize((int)((float)originalSizeX * scaleFactor), (int)((float)originalSizeY * scaleFactor));
        }
    }
}

void FrontendProcessorEditor::resized()
{
	LOG_START("Resizing interface");

#if HISE_IOS
	int width = originalSizeX != 0 ? originalSizeX : getWidth();
    int height = originalSizeY != 0 ? originalSizeY : getHeight();
#else
	int width = getWidth();
	int height = getHeight();
#endif


    container->setBounds(0, 0, width, height);
	rootTile->setBounds(0, 0, width, height);
    deactiveOverlay->setBounds(0, 0, width, height);
	loaderOverlay->setBounds(0, 0, width, height);
	debugLoggerComponent->setBounds(0, height-90, width, 90);

}

} // namespace hise
