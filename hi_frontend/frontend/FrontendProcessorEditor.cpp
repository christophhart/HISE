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
	usesOpenGl = dynamic_cast<GlobalSettingManager*>(fp)->useOpenGL;
	
	if(usesOpenGl)
		setEnableOpenGL(this);

	fp->addScaleFactorListener(this);
	fp->incActiveEditors();

    Desktop::getInstance().setDefaultLookAndFeel(&globalLookAndFeel);
    
	LOG_START("Creating Interface")

    addAndMakeVisible(container = new FrontendEditorHolder());
    
#if USE_RAW_FRONTEND
	container->addAndMakeVisible(rawEditor = fp->rawDataHolder->createEditor());
#else
	container->addAndMakeVisible(rootTile = new FloatingTile(fp, nullptr));

	LOG_START("Creating Root Panel");

#if HI_ENABLE_EXPANSION_EDITING
	ExpansionHandler::Helpers::createFrontendLayoutWithExpansionEditing(rootTile);
#else
	rootTile->setNewContent("InterfacePanel");
#endif
#endif

	if(fp->isUsingDefaultOverlay())
		container->addAndMakeVisible(deactiveOverlay = new DeactiveOverlay(fp));

#if FRONTEND_IS_PLUGIN || HISE_IOS
    const bool searchSamples = false;
#else
    const bool searchSamples = ProcessorHelpers::getFirstProcessorWithType<ModulatorSampler>(fp->getMainSynthChain()) != nullptr || FullInstrumentExpansion::isEnabled(fp);
    
#endif

    if(searchSamples && !fp->deactivatedBecauseOfMemoryLimitation)
    {
		auto samplesInstalled = FrontendHandler::checkSamplesCorrectlyInstalled();
		auto samplesLoaded = GET_PROJECT_HANDLER(fp->getMainSynthChain()).areSamplesLoadedCorrectly();

		if (!samplesInstalled)
			fp->sendOverlayMessage(OverlayMessageBroadcaster::SamplesNotInstalled);

		if (!samplesLoaded)
			fp->sendOverlayMessage(OverlayMessageBroadcaster::SamplesNotFound);
	}



#if USE_COPY_PROTECTION && !USE_SCRIPT_COPY_PROTECTION
	if (!fp->unlocker.isUnlocked())
	{
		if (deactiveOverlay != nullptr)
		{
			auto s = deactiveOverlay->checkLicense();

			fp->sendOverlayMessage(s);
		}
	}
#endif

	if (deactiveOverlay != nullptr)
	{
		deactiveOverlay->checkVisibility();
	}

	container->addAndMakeVisible(loaderOverlay = new ThreadWithQuasiModalProgressWindow::Overlay());
    
	loaderOverlay->setDialog(nullptr);
	fp->setOverlay(loaderOverlay);

    container->addChildComponent(debugLoggerComponent = new DebugLoggerComponent(&fp->getDebugLogger()));

	debugLoggerComponent->setVisible(fp->getDebugLogger().isLogging());

    if(fp->deactivatedBecauseOfMemoryLimitation)
    {
        auto b = HiseDeviceSimulator::getDisplayResolution();
        
        getContentComponent()->setVisible(false);
        
		if (deactiveOverlay != nullptr)
		{
			deactiveOverlay->clearAllFlags();
			deactiveOverlay->setVisible(false);
		}

        container->setVisible(false);
        
        setSize(b.getWidth(), b.getHeight());
        return;
    }
    
#if USE_RAW_FRONTEND
	setSize(getContentComponent()->getWidth(), getContentComponent()->getHeight());
#else
	auto jsp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(fp);
    

    

    if(jsp != nullptr)
    {
#if HI_ENABLE_EXPANSION_EDITING
		int heightOfContent = jsp->getScriptingContent()->getContentHeight() + 50;
#else
		int heightOfContent = jsp->getScriptingContent()->getContentHeight();
#endif

        setSize(jsp->getScriptingContent()->getContentWidth(), heightOfContent);
    }
#endif
	
	startTimer(4125);

	originalSizeX = getWidth();
	originalSizeY = getHeight();

	const int availableHeight = Desktop::getInstance().getDisplays().getMainDisplay().userArea.getHeight();
	const float displayScaleFactor = (float)Desktop::getInstance().getDisplays().getMainDisplay().scale;
	const int unscaledInterfaceHeight = getHeight();
    
#if HISE_IOS
    
    if(!HiseDeviceSimulator::isAUv3())
    {
        const float iosScaleFactor = (float)availableHeight / (float)originalSizeY;
        setGlobalScaleFactor(iosScaleFactor);
    }
    else if (HiseDeviceSimulator::isiPhone())
    {
        setGlobalScaleFactor(1.15f);
    }
    
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
        
	if (FullInstrumentExpansion::isEnabled(getMainController()))
		getMainController()->getLockFreeDispatcher().addPresetLoadListener(this);
}

FrontendProcessorEditor::~FrontendProcessorEditor()
{
	detachOpenGl();

	if (FullInstrumentExpansion::isEnabled(getMainController()))
		getMainController()->getLockFreeDispatcher().removePresetLoadListener(this);

	dynamic_cast<FrontendProcessor*>(getAudioProcessor())->decActiveEditors();
	dynamic_cast<GlobalSettingManager*>(getAudioProcessor())->removeScaleFactorListener(this);
	
#if USE_RAW_FRONTEND
	container->removeChildComponent(rawEditor);
	rawEditor = nullptr;
#else
	container->removeChildComponent(rootTile);
	rootTile = nullptr;
#endif

	deactiveOverlay = nullptr;
	container = nullptr;
	loaderOverlay = nullptr;
	debugLoggerComponent = nullptr;
}

Component* FrontendProcessorEditor::getContentComponent()
{
#if USE_RAW_FRONTEND
	return rawEditor;
#else
	return rootTile;
#endif
}

void FrontendProcessorEditor::newHisePresetLoaded()
{
	if (FullInstrumentExpansion::isEnabled(getMainController()))
	{
		if (auto jsp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getMainController()))
		{
			originalSizeX = jsp->getScriptingContent()->getContentWidth();
			originalSizeY = jsp->getScriptingContent()->getContentHeight();
		}

		setGlobalScaleFactor(scaleFactor, true);
	}
}

void FrontendProcessorEditor::scaleFactorChanged(float newScaleFactor)
{
	setGlobalScaleFactor(newScaleFactor);
}

void FrontendProcessorEditor::setGlobalScaleFactor(float newScaleFactor, bool forceUpdate)
{
	LOG_START("Change scale factor");

    if (newScaleFactor > 0.2 && ((scaleFactor != newScaleFactor) || forceUpdate))
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
	int width = (int)((double)getWidth() / scaleFactor);
	int height = (int)((double)getHeight() / scaleFactor);
#endif

    container->setBounds(0, 0, width, height);
	getContentComponent()->setBounds(0, 0, width, height);

	if(deactiveOverlay != nullptr)
		deactiveOverlay->setBounds(0, 0, width, height);

	loaderOverlay->setBounds(0, 0, width, height);
	debugLoggerComponent->setBounds(0, height -90, width, 90);
}

} // namespace hise
