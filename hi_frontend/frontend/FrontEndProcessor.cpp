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


FrontendProcessor* FrontendFactory::createPluginWithAudioFiles(AudioDeviceManager* deviceManager, AudioProcessorPlayer* callback)
{
	ValueTree presetData; 
	zstd::ZCompressor<PresetDictionaryProvider> pdec; 
	MemoryBlock pBlock;
	ScopedPointer<MemoryInputStream> pis = getEmbeddedData(FileHandlerBase::Presets);
	pis->readIntoMemoryBlock(pBlock);
	pdec.expand(pBlock, presetData);
	
	/*ValueTree presetData = ValueTree::readFromData(PresetData::preset PresetData::presetSize);\*/ 
	LOG_START("Loading embedded image data"); 
	auto imageData = getEmbeddedData(FileHandlerBase::Images);
	LOG_START("Loading embedded impulse responses"); 
	auto impulseData = getEmbeddedData(FileHandlerBase::AudioFiles);
	auto sampleMapData = getEmbeddedData(FileHandlerBase::SampleMaps);
	auto midiData = getEmbeddedData(FileHandlerBase::MidiFiles);

	LOG_START("Loading embedded other data")

	ValueTree externalFiles;
	MemoryBlock eBlock;
	ScopedPointer<MemoryInputStream> eis = getEmbeddedData(FileHandlerBase::Scripts);
	eis->readIntoMemoryBlock(eBlock);
	zstd::ZCompressor<JavascriptDictionaryProvider> edec;
	edec.expand(eBlock, externalFiles);

	//ValueTree externalFiles =  hise::PresetHandler::loadValueTreeFromData(PresetData::externalFiles, PresetData::externalFilesSize, true); 
	LOG_START("Creating Frontend Processor")
	auto fp = new hise::FrontendProcessor(presetData, deviceManager, callback, imageData, impulseData, sampleMapData, midiData, &externalFiles, nullptr); 

	ScopedPointer<MemoryInputStream> uis = getEmbeddedData(FileHandlerBase::UserPresets);

    try
    {
        UserPresetHelpers::extractUserPresets((const char*)uis->getData(), uis->getDataSize());
        AudioProcessorDriver::restoreSettings(fp);
        GlobalSettingManager::restoreGlobalSettings(fp);
        
        GET_PROJECT_HANDLER(fp->getMainSynthChain()).loadSamplesAfterSetup();
    }
    catch(String& s)
    {
        fp->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
    }
	 
	

	return fp;
}

FrontendProcessor* FrontendFactory::createPlugin(AudioDeviceManager* deviceManager, AudioProcessorPlayer* callback)
{
	return createPluginWithAudioFiles(deviceManager, callback);
}


void FrontendProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
#if USE_COPY_PROTECTION
	if (!keyFileCorrectlyLoaded)
		return;

	if (((unlockCounter++ & 1023) == 0) && !unlocker.isUnlocked()) return;
#endif

#if FRONTEND_IS_PLUGIN && HI_SUPPORT_MONO_CHANNEL_LAYOUT
	
	if (buffer.getNumChannels() == 1)
	{
		stereoCopy.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
		stereoCopy.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

        AudioSampleBuffer smallBuffer(stereoCopy.getArrayOfWritePointers(), 2, buffer.getNumSamples());
        
        getDelayedRenderer().processWrapped(smallBuffer, midiMessages);
        
        buffer.copyFrom(0, 0, stereoCopy, 0, 0, buffer.getNumSamples());
        
	}
	else
	{
#if HI_SUPPORT_MONO_TO_STEREO
		buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
#endif

		getDelayedRenderer().processWrapped(buffer, midiMessages);
	}

#else
	getDelayedRenderer().processWrapped(buffer, midiMessages);
#endif
};

void FrontendProcessor::incActiveEditors()
{
    if (numActiveEditors <= 0)
    {
        updater.suspendState = false;
        updateSuspendState();
    }
    
    numActiveEditors++;
}

void FrontendProcessor::updateSuspendState()
{
    if(updater.suspendState != getGlobalUIUpdater()->isSuspended())
    {
        Processor::Iterator<SuspendableTimer::Manager> iter(getMainSynthChain());
        
        while (auto sm = iter.getNextProcessor())
            sm->suspendStateChanged(updater.suspendState);
        
        getGlobalUIUpdater()->suspendTimer(updater.suspendState);
    }
}
    
void FrontendProcessor::decActiveEditors()
{
	numActiveEditors = jmax<int>(0, numActiveEditors - 1);

    
    
	if (numActiveEditors == 0)
	{
		updater.suspendState = true;
        updater.updateDelayed();
	}
}

void FrontendProcessor::handleControllersForMacroKnobs(const MidiBuffer &midiMessages)
{
	if(!getMacroManager().macroControlMidiLearnModeActive() && !getMacroManager().midiMacroControlActive()) return;

	MidiBuffer::Iterator it(midiMessages);

	int samplePos;
	MidiMessage message;

	while(it.getNextEvent(message, samplePos))
	{
		if(message.isController())
		{
			const int controllerNumber = message.getControllerNumber();

			if(getMacroManager().macroControlMidiLearnModeActive())
			{
				getMacroManager().setMidiControllerForMacro(controllerNumber);
			}

			const int macroNumber = getMacroManager().getMacroControlForMidiController(controllerNumber);

			if(macroNumber != -1)
			{
				getMacroManager().getMacroChain()->setMacroControl(macroNumber, (float)message.getControllerValue(), sendNotification);
			}
		}
	}

}

    
    
void FrontendProcessor::restorePool(InputStream* inputStream, FileHandlerBase::SubDirectories directory, const String& fileNameToLook)
{
    ScopedPointer<FileInputStream> fis;
    InputStream* streamToUse = inputStream;
        
    if(streamToUse == nullptr)
    {
        auto resourceFile = getSampleManager().getProjectHandler().getEmbeddedResourceDirectory().getChildFile(fileNameToLook);

		if (!resourceFile.existsAsFile())
		{
			sendOverlayMessage(DeactiveOverlay::CriticalCustomErrorMessage,
				"The file " + resourceFile.getFullPathName() + " can't be found.");
			return;
		}
            
        fis = new FileInputStream(resourceFile);
        streamToUse = fis.release();
    }
    
    jassert(streamToUse != nullptr);
    
    switch(directory)
    {
        case FileHandlerBase::Images: getCurrentImagePool()->getDataProvider()->restorePool(streamToUse); break;
        case FileHandlerBase::AudioFiles: getCurrentAudioSampleBufferPool()->getDataProvider()->restorePool(streamToUse); break;
        case FileHandlerBase::SampleMaps: getCurrentSampleMapPool()->getDataProvider()->restorePool(streamToUse); break;
		case FileHandlerBase::SubDirectories::MidiFiles: getCurrentMidiFilePool()->getDataProvider()->restorePool(streamToUse); break;
        default: jassertfalse; break;
    }
}
    
static int numInstances = 0;

FrontendProcessor::FrontendProcessor(ValueTree &synthData, AudioDeviceManager* manager, AudioProcessorPlayer* callback_, MemoryInputStream *imageData/*=nullptr*/, MemoryInputStream *impulseData/*=nullptr*/, MemoryInputStream* sampleMapData, MemoryInputStream* midiFileData, ValueTree *externalFiles/*=nullptr*/, ValueTree *) :
MainController(),
PluginParameterAudioProcessor(FrontendHandler::getProjectName()),
AudioProcessorDriver(manager, callback_),
synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES)),
keyFileCorrectlyLoaded(true),
currentlyLoadedProgram(0),
unlockCounter(0),
updater(*this)
{
	ignoreUnused(synthData);

	LOG_START("Checking license");

    HiseDeviceSimulator::init(wrapperType);
    
	GlobalSettingManager::initData(this);

    numInstances++;
    
    if(HiseDeviceSimulator::isAUv3() && numInstances > HISE_AUV3_MAX_INSTANCE_COUNT)
    {
        deactivatedBecauseOfMemoryLimitation = true;
        keyFileCorrectlyLoaded = true;
        return;
    }
    
#if USE_COPY_PROTECTION
	if (!unlocker.loadKeyFile())
		keyFileCorrectlyLoaded = false;
#endif
    
	LOG_START("Load images");
    restorePool(imageData, FileHandlerBase::Images, "ImageResources.dat");
    
   	LOG_START("Load embedded audio files");
    restorePool(impulseData, FileHandlerBase::AudioFiles, "AudioResources.dat");
    
  	LOG_START("Load samplemaps");
    restorePool(sampleMapData, FileHandlerBase::SampleMaps, "SampleMapResources.dat");


    
	LOG_START("Load Midi Files");
	restorePool(midiFileData, FileHandlerBase::MidiFiles, "MidiFilesResources.dat");

#if HI_ENABLE_EXPANSION_EDITING
	getCurrentFileHandler().pool->getSampleMapPool().loadAllFilesFromDataProvider();
	getCurrentFileHandler().pool->getMidiFilePool().loadAllFilesFromDataProvider();
#endif

	getExpansionHandler().createAvailableExpansions();

	if (externalFiles != nullptr)
	{
		setExternalScriptData(externalFiles->getChildWithName("ExternalScripts"));
		restoreCustomFontValueTree(externalFiles->getChildWithName("CustomFonts"));
		restoreEmbeddedMarkdownDocs(externalFiles->getChildWithName("MarkdownDocs"));

	}
    
	numParameters = 0;

	getMacroManager().setMacroChain(synthChain);

#if USE_RAW_FRONTEND
	rawDataHolder = createPresetRaw();
#else
	synthChain->setId(synthData.getProperty("ID", String()));
	createPreset(synthData);
#endif
	
#if FRONTEND_IS_PLUGIN && HI_SUPPORT_MONO_CHANNEL_LAYOUT
	stereoCopy.setSize(2, 0);
#endif
    
    updater.suspendState = true;
    updater.updateDelayed();
}

FrontendProcessor::~FrontendProcessor()
{
	numInstances--;

	notifyShutdownToRegisteredObjects();
	getKillStateHandler().deinitialise();
	deletePendingFlag = true;

	storeAllSamplesFound(GET_PROJECT_HANDLER(getMainSynthChain()).areSamplesLoadedCorrectly());

	
	getJavascriptThreadPool().cancelAllJobs();
	getSampleManager().cancelAllJobs();

	setEnabledMidiChannels(synthChain->getActiveChannelData()->exportData());
	
	clearPreset();
	
	synthChain = nullptr;

#if USE_RAW_FRONTEND
	rawDataHolder = nullptr;
#endif
};


    
void FrontendProcessor::createPreset(const ValueTree& synthData)
{
	getSampleManager().setShouldSkipPreloading(true);

	setSkipCompileAtPresetLoad(true);

	LOG_START("Restoring main container");

	ScopedSoftBypassDisabler ssbd(this);

	synthChain->restoreFromValueTree(synthData);

	setSkipCompileAtPresetLoad(false);

	{
		LOG_START("Compiling all scripts");
		LockHelpers::SafeLock sl(this, LockHelpers::ScriptLock);
		synthChain->compileAllScripts();
	}


	ValueTree autoData = synthData.getChildWithName("MidiAutomation");

	if (autoData.isValid())
		getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);

	synthChain->loadMacrosFromValueTree(synthData);

	LOG_START("Adding plugin parameters");

    try
    {
        addScriptedParameters();
    }
    catch(String& s)
    {
		ignoreUnused(s);
        DBG("Error: " + s);
    }

	CHECK_COPY_AND_RETURN_6(synthChain);

	if (getSampleRate() > 0)
	{
		LOG_START("Initialising audio callback");
		synthChain->prepareToPlay(getSampleRate(), getBlockSize());
	}
}

const String FrontendProcessor::getName(void) const
{
	return FrontendHandler::getProjectName();
}

void FrontendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{


    getDelayedRenderer().prepareToPlayWrapped(newSampleRate, samplesPerBlock);

#if FRONTEND_IS_PLUGIN
	handleLatencyInPrepareToPlay(newSampleRate);
	
#if HI_SUPPORT_MONO_CHANNEL_LAYOUT
	ProcessorHelpers::increaseBufferIfNeeded(stereoCopy, samplesPerBlock);
#endif
#endif
};

AudioProcessorEditor* FrontendProcessor::createEditor()
{
	return new FrontendProcessorEditor(this);
}

void FrontendProcessor::setCurrentProgram(int /*index*/)
{
	return;
}

const String FrontendStandaloneApplication::getApplicationName()
{
	return ProjectInfo::projectName;
}

const String FrontendStandaloneApplication::getApplicationVersion()
{
	return ProjectInfo::versionString;
}

void FrontendStandaloneApplication::AudioWrapper::init()
{
	LOG_START("Initialising Standalone Wrapper");

	setOpaque(true);
	standaloneProcessor = new StandaloneProcessor();

	editor = standaloneProcessor->createEditor();

	addAndMakeVisible(editor);

	if (splashScreen != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(splashScreen, 600);
		splashScreen = nullptr;
	}

#if HISE_IOS
	resized();
#else
	float sf = standaloneProcessor->getScaleFactor();

	int newWidth = (int)((float)editor->getWidth()*sf);
	int newHeight = (int)((float)editor->getHeight() * sf);

	setSize(newWidth, newHeight);
#endif
}

FrontendStandaloneApplication::AudioWrapper::AudioWrapper()
{
#if USE_SPLASH_SCREEN
    Image imgiPhone = ImageCache::getFromMemory(BinaryData::SplashScreeniPhone_png, BinaryData::SplashScreeniPhone_pngSize);
	Image imgiPad = ImageCache::getFromMemory(BinaryData::SplashScreen_png, BinaryData::SplashScreen_pngSize);

    const bool isIPhone = SystemStats::getDeviceDescription() == "iPhone";
    
	addAndMakeVisible(splashScreen = new ImageComponent());

    splashScreen->setImage(isIPhone ? imgiPhone : imgiPad);

#if HISE_IOS
    auto size = Desktop::getInstance().getDisplays().getMainDisplay().totalArea;
    
    setSize(size.getWidth(), size.getHeight());
#else

	auto userHeight = Desktop::getInstance().getDisplays().getMainDisplay().userArea.getHeight();

	if (userHeight < 768)
		setSize(870, 653);
	else
		setSize(1024, 768);
#endif
    
	startTimer(100);

#else
    
#if HISE_IOS
    auto size = Desktop::getInstance().getDisplays().getMainDisplay().totalArea;
    
    setSize(size.getWidth(), size.getHeight());
#endif
    
	init();
#endif


	
}

FrontendStandaloneApplication::AudioWrapper::~AudioWrapper()
{
	editor = nullptr;
	standaloneProcessor = nullptr;
}

FrontendStandaloneApplication::MainWindow::MainWindow(String name) : DocumentWindow(name,
	Colours::lightgrey,
	DocumentWindow::allButtons)
{
	setUsingNativeTitleBar(true);
	setContentOwned(new AudioWrapper(), true);

	centreWithSize(getWidth(), getHeight());
		
	setResizable(false, false);
	setVisible(true);
}

} // namespace hise
