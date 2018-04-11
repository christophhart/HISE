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

void FrontendProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
#if USE_COPY_PROTECTION || USE_TURBO_ACTIVATE
	if (!keyFileCorrectlyLoaded)
		return;

	if (((unlockCounter++ & 1023) == 0) && !unlocker.isUnlocked()) return;
#endif

	getDelayedRenderer().processWrapped(buffer, midiMessages);
};

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

#if JUCE_WINDOWS
#define TURBOACTIVATE_FILE_PATH ProjectHandler::Frontend::getAppDataDirectory().getFullPathName().toUTF16().getAddress()
#else
#if ENABLE_APPLE_SANDBOX
#define TURBOACTIVATE_FILE_PATH ProjectHandler::Frontend::getAppDataDirectory().getChildFile("Resources/").getFullPathName().toUTF8().getAddress()
#else
#define TURBOACTIVATE_FILE_PATH ProjectHandler::Frontend::getAppDataDirectory().getFullPathName().toUTF8().getAddress()
#endif
#endif

FrontendProcessor::FrontendProcessor(ValueTree &synthData, AudioDeviceManager* manager, AudioProcessorPlayer* callback_, ValueTree *imageData_/*=nullptr*/, ValueTree *impulseData/*=nullptr*/, ValueTree *externalFiles/*=nullptr*/, ValueTree *) :
MainController(),
PluginParameterAudioProcessor(ProjectHandler::Frontend::getProjectName()),
AudioProcessorDriver(manager, callback_),
synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES)),
keyFileCorrectlyLoaded(true),
currentlyLoadedProgram(0),
#if USE_TURBO_ACTIVATE
unlockCounter(0),
unlocker(TURBOACTIVATE_FILE_PATH)
#else
unlockCounter(0)
#endif
{
	LOG_START("Checking license");

    HiseDeviceSimulator::init(wrapperType);
    
	GlobalSettingManager::initData(this);

#if USE_COPY_PROTECTION

	if (!unlocker.loadKeyFile())
	{
		keyFileCorrectlyLoaded = false;
	}
	
#elif USE_TURBO_ACTIVATE
	
	keyFileCorrectlyLoaded = unlocker.isUnlocked();

#endif
    
	LOG_START("Load images");

	loadImages(imageData_);

	if (externalFiles != nullptr)
	{
		setExternalScriptData(externalFiles->getChildWithName("ExternalScripts"));
		restoreCustomFontValueTree(externalFiles->getChildWithName("CustomFonts"));

		sampleMaps = externalFiles->getChildWithName("SampleMaps");
	}
    
	if (impulseData != nullptr)
	{
		getSampleManager().getAudioSampleBufferPool()->restoreFromValueTree(*impulseData);
	}
	else
	{
		File audioResourceFile(ProjectHandler::Frontend::getAppDataDirectory().getChildFile("AudioResources.dat"));

		if (audioResourceFile.existsAsFile())
		{
			FileInputStream fis(audioResourceFile);

			LOG_START("Load impulses");

			ValueTree impulseDataFile = ValueTree::readFromStream(fis);

			if (impulseDataFile.isValid())
			{
				getSampleManager().getAudioSampleBufferPool()->restoreFromValueTree(impulseDataFile);
			}
		}
	}

	numParameters = 0;

	getMacroManager().setMacroChain(synthChain);

	synthChain->setId(synthData.getProperty("ID", String()));

	{
		MainController::ScopedSuspender ss(this);

		getSampleManager().setShouldSkipPreloading(true);

		setSkipCompileAtPresetLoad(true);

		LOG_START("Restoring main container");

		synthChain->restoreFromValueTree(synthData);

		setSkipCompileAtPresetLoad(false);

		LOG_START("Compiling all scripts");

		synthChain->compileAllScripts();

		synthChain->loadMacrosFromValueTree(synthData);

		LOG_START("Adding plugin parameters");

		addScriptedParameters();

		CHECK_COPY_AND_RETURN_6(synthChain);

		if (getSampleRate() > 0)
		{
			LOG_START("Initialising audio callback");

			synthChain->prepareToPlay(getSampleRate(), getBlockSize());
		}

#if !DONT_EMBED_FILES_IN_FRONTEND
		if (sampleMaps.getNumChildren() == 0)
		{
			createSampleMapValueTreeFromPreset(synthData);
		}
#endif

		createUserPresetData();
	}

    

	updateUnlockedSuspendStatus();
}

const String FrontendProcessor::getName(void) const
{
	return ProjectHandler::Frontend::getProjectName();
}

void FrontendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    MainController::ScopedSuspender ss(this);

	getDelayedRenderer().prepareToPlayWrapped(newSampleRate, samplesPerBlock);
};

AudioProcessorEditor* FrontendProcessor::createEditor()
{
	return new FrontendProcessorEditor(this);
}

void FrontendProcessor::setCurrentProgram(int /*index*/)
{
	return;
}

void FrontendProcessor::loadImages(ValueTree *imageData)
{
#if HISE_IOS
    
    // The images are loaded from actual files here...
    return;
#endif
    
	if (imageData == nullptr)
	{
		File imageResources = ProjectHandler::Frontend::getAppDataDirectory().getChildFile("ImageResources.dat");

		if (imageResources.existsAsFile())
		{
			FileInputStream fis(imageResources);

			auto t = ValueTree::readFromStream(fis);

			if (t.isValid())
				getSampleManager().getImagePool()->restoreFromValueTree(t);
			else
				sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, "The image resources are corrupt. Contact support");
		}
		else
			sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, "The image resources can't be located. Contact support");
	}
	else
	{
		getSampleManager().getImagePool()->restoreFromValueTree(*imageData);
	}
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

#if !HISE_IOS
	context = new OpenGLContext();

	if (dynamic_cast<GlobalSettingManager*>(editor->getAudioProcessor())->useOpenGL)
		context->attachTo(*editor);
#endif

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
	{
		setSize(870, 653);
	}
	else
	{
		setSize(1024, 768);
	}
	

	

    
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
#if !HISE_IOS

	if(context->isAttached())
		context->detach();

	context = nullptr;
#endif

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
