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

// This is a helper tool to print out definitions for the extern const char* variables with the sizeof([]) operator
#if 0
#define PRINT_DATA(ns, x) DBG(juce::String("DECLARE_DATA(") + #x + ", " + juce::String(sizeof(ns::x)) + ");")

void printData()
{
	PRINT_DATA(HiBinaryData::SpecialSymbols, midiData);
	PRINT_DATA(HiBinaryData::SpecialSymbols, masterEffect);
	PRINT_DATA(HiBinaryData::SpecialSymbols, macros);
	PRINT_DATA(HiBinaryData::SpecialSymbols, globalCableIcon);
	PRINT_DATA(HiBinaryData::SpecialSymbols, scriptProcessor);
	PRINT_DATA(HiBinaryData::SpecialSymbols, routingIcon);
}

#undef PRINT_DATA
#endif

namespace hise { using namespace juce;


	

	void ExampleAssetManager::initialise()
	{
		if(!initialised)
		{
			initialised = true;

			setWorkingProject(mainProjectHandler.getRootFolder());

			auto snippetSettings = getAppDataDirectory(getMainController()).getChildFile("snippetBrowser.xml");

			if(auto xml = XmlDocument::parse(snippetSettings))
			{
				if(auto sd = xml->getChildByName("snippetDirectory"))
				{
					auto snippetDirectory = sd->getStringAttribute("value");

					if(File::isAbsolutePath(snippetDirectory))
					{
						auto assetDirectory = File(snippetDirectory).getChildFile("Assets");

						if(!assetDirectory.getChildFile("SampleMaps").isDirectory())
						{
							debugError(getMainController()->getMainSynthChain(), "Uninitialised assets, please download the assets and reload the snippet");
							initialised = false;
							return;
						}

						if(assetDirectory.isDirectory())
						{
							rootDirectory = assetDirectory;

							for(auto d: getSubDirectoryIds())
								rootDirectory.getChildFile(getIdentifier(d)).createDirectory();

							checkSubDirectories();

							pool->getAudioSampleBufferPool().loadAllFilesFromProjectFolder();
							pool->getImagePool().loadAllFilesFromProjectFolder();
							pool->getMidiFilePool().loadAllFilesFromProjectFolder();
							pool->getSampleMapPool().loadAllFilesFromProjectFolder();
							return;
						}
					}
				}
			}

			debugError(getMainController()->getMainSynthChain(), "You need to download the assets using the snippet browser");
		}
	}

	File ExampleAssetManager::getSubDirectory(SubDirectories dir) const
	{
		auto redirected = getSubDirectoryIds();

		if(redirected.contains(dir))
			return ProjectHandler::getSubDirectory(dir);
		else
			return mainProjectHandler.getSubDirectory(dir);
	}

	Array<FileHandlerBase::SubDirectories> ExampleAssetManager::getSubDirectoryIds() const
	{
		return {
			FileHandlerBase::SubDirectories::AudioFiles,
			FileHandlerBase::SubDirectories::SampleMaps,
			FileHandlerBase::SubDirectories::Samples,
			FileHandlerBase::SubDirectories::Images,
			FileHandlerBase::SubDirectories::MidiFiles
		};
	}

bool PluginParameterRamp::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages,
	const ProcessCallback& f)
{
	PluginParameterSimulatorInfo thisInfo, thisGesture;

	{
		SimpleReadWriteLock::ScopedReadLock sl(lock);
		thisInfo = currentInfo;
		thisGesture = gestureInfo;
	}

	auto threadMatches = thisInfo.sourceThread == PluginParameterSimulatorInfo::SourceThread::Audio;

	if(gestureInfo && gestureAtNextCallback && gestureInfo.sourceThread == PluginParameterSimulatorInfo::SourceThread::Audio)
	{
		thisGesture.performGesture();
		gestureAtNextCallback = false;

		if(thisGesture.eventType == PluginParameterSimulatorInfo::EventType::EndGesture)
		{
			currentInfo = {};
			return false;
		}
	}

	if(!thisInfo || !threadMatches)
		return false;

	

	if(!thisInfo.useRamp)
	{
		thisInfo.performChange();

		SimpleReadWriteLock::ScopedWriteLock sl(lock);
		currentInfo = {};
		return false;
	}

	auto numSamples = thisInfo.bufferSize != -1 ? thisInfo.bufferSize : buffer.getNumSamples();
	auto rampTime =  (double)numSamples / getMainController()->getMainSynthChain()->getSampleRate() * 1000.0;

	if(thisInfo.bufferSize == -1)
	{
		bump(thisInfo, rampTime);
		currentInfo.currentValue = thisInfo.currentValue;
		return false;
	}
	else
	{
		int numTodo = buffer.getNumSamples();
		int pos = 0;

		while (numTodo > 0)
		{
			bump(thisInfo, rampTime);
			float* channels[HISE_NUM_PLUGIN_CHANNELS];
			f(channels, buffer, midiMessages, pos, thisInfo.bufferSize);

			numTodo -= thisInfo.bufferSize;
			pos += thisInfo.bufferSize;
		}
	}

	currentInfo.currentValue = thisInfo.currentValue;
	return true;
}

void PluginParameterRamp::setCurrentInfo(const PluginParameterSimulatorInfo& newInfo)
{
	auto rampWasActive = gestureInfo.useRamp;
	auto prevValue = currentInfo.currentValue;

	auto gestureWasActive = gestureInfo.isActiveGesture();

	{
		SimpleReadWriteLock::ScopedWriteLock sl(lock);

		if(newInfo.isGestureEvent())
			gestureInfo = newInfo;
		else
			currentInfo = newInfo;
	}

	if(!newInfo)
		return;

	if(rampWasActive)
		currentInfo.currentValue = prevValue;

	auto gestureShouldBeActive = gestureInfo.isActiveGesture();

	if(gestureWasActive != gestureShouldBeActive)
	{
		if(gestureInfo.sourceThread != PluginParameterSimulatorInfo::SourceThread::UI)
		{
			gestureAtNextCallback = true;
		}
		else
		{
			gestureInfo.performGesture();
		}
	}

	auto useTimer = gestureInfo.useRamp && newInfo.sourceThread == PluginParameterSimulatorInfo::SourceThread::UI;
	auto useThread = (gestureInfo.useRamp || gestureShouldBeActive || gestureWasActive) && newInfo.sourceThread == PluginParameterSimulatorInfo::SourceThread::Custom;

	if(useTimer)
		start();
	else
		stop();

	if(useThread)
		ThreadStarters::startHigh(this);
	else
		stopThread(1000);

	if(currentInfo.sourceThread == PluginParameterSimulatorInfo::SourceThread::UI && !currentInfo.useRamp)
	{
		currentInfo.performChange();
		currentInfo = {};
	}
}

void PluginParameterRamp::bump(PluginParameterSimulatorInfo& info, double milliSeconds)
{
	auto delta = (float)milliSeconds * 0.001f;

	if(!sign)
		delta *= -1.0f;

	auto nv = info.currentValue + delta;

	if(nv >= 1.0f)
		sign = false;
	if(nv <= 0.0f)
		sign = true;

	

	info.currentValue = jlimit(0.0f, 1.0f, nv);
	info.performChange();
}

int BackendProcessor::commandLineServerPort = 0;

RestServer::Response BackendProcessor::onAsyncRequest(RestServer::AsyncRequest::Ptr req)
{
	// Resolve which BackendProcessor the audio callback is currently driving
	// (main or snippet browser). All routes are dispatched against this active
	// instance so that requests target the BP that is producing audio.
	// Topology routes (Shutdown, SnippetBrowser) rebind to main themselves via
	// mc->getMainInstance() inside their handlers.
	BackendProcessor* active = this;
	if (callback != nullptr)
	{
		if (auto cur = dynamic_cast<BackendProcessor*>(callback->getCurrentProcessor()))
			active = cur;
	}

	debugToConsole(active->getMainSynthChain(), "\tincoming HTTP request: " + req->getRequest().url.toString(true));

	// Attach console handler to request (lifetime tied to request, not stack frame)
	req->setConsoleCapture(std::make_unique<RestHelpers::ScopedConsoleHandler>(active, req));

	auto subURL = req->getRequest().url.getSubPath(false);
	auto route = RestHelpers::findRoute(subURL);

	// Reject project/wizard mutations while the snippet browser drives audio,
	// so the user's real project cannot be modified by accident.
	if (route != RestHelpers::ApiRoute::numRoutes && active->isSnippetBrowser())
	{
		const auto& meta = RestHelpers::getRouteMetadata().getReference((int)route);
		if (meta.rejectInSnippetBrowser)
			return req->fail(409, "endpoint disabled while the snippet browser is active: " + meta.path);
	}

	switch (route)
	{
		case RestHelpers::ApiRoute::ListMethods:
			return RestHelpers::handleListMethods(active, req);
			
		case RestHelpers::ApiRoute::Status:
			return RestHelpers::handleStatus(active, req);

		case RestHelpers::ApiRoute::StatusPreprocessors:
			return RestHelpers::handleStatusPreprocessors(active, req);

		case RestHelpers::ApiRoute::GetScript:
			return RestHelpers::handleGetScript(active, req);
			
		case RestHelpers::ApiRoute::SetScript:
			return RestHelpers::handleSetScript(active, req);
			
		case RestHelpers::ApiRoute::Recompile:
			return RestHelpers::handleRecompile(active, req);
			
		case RestHelpers::ApiRoute::ListComponents:
			return RestHelpers::handleListComponents(active, req);

		case RestHelpers::ApiRoute::EvaluateREPL:
			return RestHelpers::handleEvaluateREPL(active, req);
			
		case RestHelpers::ApiRoute::GetComponentProperties:
			return RestHelpers::handleGetComponentProperties(active, req);
			
		case RestHelpers::ApiRoute::GetComponentValue:
			return RestHelpers::handleGetComponentValue(active, req);
			
		case RestHelpers::ApiRoute::SetComponentValue:
			return RestHelpers::handleSetComponentValue(active, req);
			
		case RestHelpers::ApiRoute::SetComponentProperties:
			return RestHelpers::handleSetComponentProperties(active, req);
			
		case RestHelpers::ApiRoute::TestingScreenshot:
			return RestHelpers::handleTestingScreenshot(active, req);
			
		case RestHelpers::ApiRoute::GetSelectedComponents:
			return RestHelpers::handleGetSelectedComponents(active, req);
			
		case RestHelpers::ApiRoute::TestingE2e:
			return RestHelpers::handleTestingE2e(active, req);
			
		case RestHelpers::ApiRoute::DiagnoseScript:
			return RestHelpers::handleDiagnoseScript(active, req);
			
		case RestHelpers::ApiRoute::GetIncludedFiles:
			return RestHelpers::handleGetIncludedFiles(active, req);
			
		case RestHelpers::ApiRoute::TestingProfile:
			return RestHelpers::handleTestingProfile(active, req);
			
		case RestHelpers::ApiRoute::ParseCSS:
			return RestHelpers::handleParseCSS(active, req);
			
	case RestHelpers::ApiRoute::Shutdown:
		return RestHelpers::handleShutdown(active, req);
	
	case RestHelpers::ApiRoute::BuilderTree:
		return RestHelpers::handleBuilderTree(active, req);
	
	case RestHelpers::ApiRoute::BuilderApply:
		return RestHelpers::handleBuilderApply(active, req);

	case RestHelpers::ApiRoute::BuilderReset:
		return RestHelpers::handleBuilderReset(active, req);

	case RestHelpers::ApiRoute::UndoPushGroup:
		return RestHelpers::handleUndoPushGroup(active, req);
	
	case RestHelpers::ApiRoute::UndoPopGroup:
		return RestHelpers::handleUndoPopGroup(active, req);
	
	case RestHelpers::ApiRoute::UndoBack:
		return RestHelpers::handleUndoBack(active, req);
	
	case RestHelpers::ApiRoute::UndoForward:
		return RestHelpers::handleUndoForward(active, req);
	
	case RestHelpers::ApiRoute::UndoDiff:
		return RestHelpers::handleUndoDiff(active, req);
	
	case RestHelpers::ApiRoute::UndoHistory:
		return RestHelpers::handleUndoHistory(active, req);
	
	case RestHelpers::ApiRoute::UndoClear:
		return RestHelpers::handleUndoClear(active, req);

	case RestHelpers::ApiRoute::WizardInitialise:
		return RestHelpers::handleWizardInitialise(active, req);

	case RestHelpers::ApiRoute::WizardExecute:
		return RestHelpers::handleWizardExecute(active, req);

	case RestHelpers::ApiRoute::WizardStatus:
		return RestHelpers::handleWizardStatus(active, req);

	case RestHelpers::ApiRoute::UITree:
		return RestHelpers::handleUITree(active, req);

	case RestHelpers::ApiRoute::UIApply:
		return RestHelpers::handleUIApply(active, req);

	case RestHelpers::ApiRoute::TestingSequence:
		return RestHelpers::handleTestingSequence(active, req);

	case RestHelpers::ApiRoute::DspList:
		return RestHelpers::handleDspList(active, req);

	case RestHelpers::ApiRoute::DspInit:
		return RestHelpers::handleDspInit(active, req);

	case RestHelpers::ApiRoute::DspTree:
		return RestHelpers::handleDspTree(active, req);

	case RestHelpers::ApiRoute::DspApply:
		return RestHelpers::handleDspApply(active, req);

	case RestHelpers::ApiRoute::DspSave:
		return RestHelpers::handleDspSave(active, req);

	case RestHelpers::ApiRoute::DspScreenshot:
		return RestHelpers::handleDspScreenshot(active, req);

	case RestHelpers::ApiRoute::ProjectList:
		return RestHelpers::handleProjectList(active, req);

	case RestHelpers::ApiRoute::ProjectTree:
		return RestHelpers::handleProjectTree(active, req);

	case RestHelpers::ApiRoute::ProjectFiles:
		return RestHelpers::handleProjectFiles(active, req);

	case RestHelpers::ApiRoute::ProjectSettingsList:
		return RestHelpers::handleProjectSettingsList(active, req);

	case RestHelpers::ApiRoute::ProjectSettingsSet:
		return RestHelpers::handleProjectSettingsSet(active, req);

	case RestHelpers::ApiRoute::ProjectSave:
		return RestHelpers::handleProjectSave(active, req);

	case RestHelpers::ApiRoute::ProjectLoad:
		return RestHelpers::handleProjectLoad(active, req);

	case RestHelpers::ApiRoute::ProjectSwitch:
		return RestHelpers::handleProjectSwitch(active, req);

	case RestHelpers::ApiRoute::ProjectExportSnippet:
		return RestHelpers::handleProjectExportSnippet(active, req);

	case RestHelpers::ApiRoute::ProjectImportSnippet:
		return RestHelpers::handleProjectImportSnippet(active, req);

	case RestHelpers::ApiRoute::ProjectPreprocessorList:
		return RestHelpers::handleProjectPreprocessorList(active, req);

	case RestHelpers::ApiRoute::ProjectPreprocessorSet:
		return RestHelpers::handleProjectPreprocessorSet(active, req);

	case RestHelpers::ApiRoute::SnippetBrowser:
		return RestHelpers::handleSnippetBrowser(active, req);

	default:
		return req->fail(404, "Unknown API endpoint: " + subURL);
	}
}

void BackendProcessor::serverStarted(int port)
{
	debugToConsole(getMainSynthChain(), "REST API Server started on port " + String(port));
	
	// Create interaction tester when server starts
	interactionTester = std::make_unique<InteractionTester>(this);

	// Create MIDI injector when server starts
	midiInjector = std::make_unique<MidiInjector>(this);
}

void BackendProcessor::serverStopped()
{
	debugToConsole(getMainSynthChain(), "REST API Server stopped");
	
	// Destroy interaction tester and MIDI injector when server stops
	interactionTester = nullptr;
	midiInjector = nullptr;
}

void BackendProcessor::requestReceived(const String& method, const String& path)
{
	// Request details are already logged in onAsyncRequest via debugToConsole
	ignoreUnused(method, path);
}

void BackendProcessor::serverError(const String& message)
{
	debugToConsole(getMainSynthChain(), "REST API Server error: " + message);
}


BackendProcessor::BackendProcessor(AudioDeviceManager *deviceManager_/*=nullptr*/, AudioProcessorPlayer *callback_/*=nullptr*/) :
  MainController(),
  AudioProcessorDriver(deviceManager_, callback_),
  scriptUnlocker(this),
  autosaver(this),
  pluginParameterRamp(this)
{
	// Register all REST API routes from the centralized metadata
	const auto& routes = RestHelpers::getRouteMetadata();
	for (const auto& route : routes)
	{
		auto routeUrl = restServer.getBaseURL().getChildURL(route.path);
		
		// Add query parameters with their default values
		for (const auto& param : route.queryParameters)
			routeUrl = routeUrl.withParameter(param.name.toString(), param.defaultValue);
		
		restServer.addAsyncRoute(route.method, routeUrl,
			BIND_MEMBER_FUNCTION_1(BackendProcessor::onAsyncRequest));
	}

	restServer.addListener(this);

	ExtendedApiDocumentation::init();

    synthChain = new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES);
    
	synthChain->addProcessorsWhenEmpty();

#if HISE_INCLUDE_PROFILING_TOOLKIT
	getDebugSession().syncRecordingBroadcaster.addListener(*synthChain, [](ModulatorSynthChain& c, bool isEnabled)
	{
		c.setEnableProfiling(isEnabled, &c.getMainController()->getDebugSession(), 0);
	}, false);
#endif

	getSampleManager().getModulatorSamplerSoundPool()->setDebugProcessor(synthChain);
	getMacroManager().setMacroChain(synthChain);

	getExpansionHandler().addListener(this);

	if (!inUnitTestMode())
	{
		handleEditorData(false);
		restoreGlobalSettings(this);
	}

	if (CompileExporter::isUsingWorkingDirectoryAsProjectFolder())
	{
		try
		{
			GET_PROJECT_HANDLER(synthChain).setWorkingProject(CompileExporter::getCurrentWorkDirectory());
		}
		catch (Result& r)
		{
			GET_PROJECT_HANDLER(synthChain).restoreWorkingProjects();
			jassertfalse;
		}
		
	}
	else
	{
		GET_PROJECT_HANDLER(synthChain).restoreWorkingProjects();
	}
	

	initData(this);

	getFontSizeChangeBroadcaster().sendMessage(sendNotification, getGlobalCodeFontSize());

	GET_PROJECT_HANDLER(synthChain).checkSubDirectories();

	dllManager = new BackendDllManager(this);

	if(getCurrentFileHandler().getRootFolder().isDirectory())
		refreshExpansionType();

	//getExpansionHandler().createAvailableExpansions();


if (!inUnitTestMode())
	{
		getAutoSaver().initialise();

		
	}
	
	clearPreset(dontSendNotification);
	getSampleManager().getProjectHandler().addListener(this);

	createInterface(600, 500);

	if (!inUnitTestMode())
	{
		auto tmp = getCurrentSampleMapPool();
		auto tmp2 = getCurrentMidiFilePool();

		auto f = [tmp, tmp2](Processor*)
		{
			tmp->loadAllFilesFromProjectFolder();
			tmp2->loadAllFilesFromProjectFolder();
			return SafeFunctionCall::OK;
		};

		getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}
    
    externalClockSim.bpm = dynamic_cast<GlobalSettingManager*>(this)->globalBPM;

#if HISE_INCLUDE_LORIS && !HISE_USE_LORIS_DLL

	lorisManager = new LorisManager(File(), [this](String message)
    {
        this->getConsoleHandler().writeToConsole(message, 1, getMainSynthChain(), Colour(HISE_ERROR_COLOUR));
    });

#else

    if(GET_HISE_SETTING(getMainSynthChain(), HiseSettings::Compiler::EnableLoris))
    {
#if HISE_USE_LORIS_DLL
        auto f = ProjectHandler::getAppDataDirectory(nullptr).getChildFile("loris_library");
        
        if(f.isDirectory())
        {
            lorisManager = new LorisManager(f, [this](String message)
            {
                this->getConsoleHandler().writeToConsole(message, 1, getMainSynthChain(), Colour(HISE_ERROR_COLOUR));
            });
        }
        else
        {
            if(PresetHandler::showYesNoWindow("Install Loris library", "In order to use Loris, you need to install the dll libraries in the app data directory of HISE. Press OK to create the folder, then download the precompiled dlls and put it in this directory"))
            {
                f.createDirectory();
                f.revealToUser();
            }
        }
#endif
    }
    else
    {
        auto f = ProjectHandler::getAppDataDirectory(this).getChildFile("loris_library");
        
        if(f.isDirectory())
            debugToConsole(getMainSynthChain(), "You seem to have installed the loris library, but you need to enable the setting `EnableLoris` in the HISE preferences");
    }

#endif

	AudioProcessor::addListener(&getUserPresetHandler());

}


BackendProcessor::~BackendProcessor()
{
	restServer.removeListener(this);
	restServer.stop();

	interactionTester = nullptr;

#if IS_STANDALONE_APP
	for(auto p: getParameters())
    {
        if(auto typed = dynamic_cast<HisePluginParameterBase*>(p))
            typed->cleanup();
    }

	setParameterTree({});
#endif

	AudioProcessor::removeListener(&getUserPresetHandler());

	getRootDispatcher().setState(dispatch::HashedPath(dispatch::CharPtr::Type::Wildcard), dispatch::State::Shutdown);

	docWindow = nullptr;
	docProcessor = nullptr;
	getDatabase().clear();

#if JUCE_ENABLE_AUDIO_GUARD
	AudioThreadGuard::setHandler(nullptr);
#endif

    getJavascriptThreadPool().stopThread(1000);
	getJavascriptThreadPool().getGlobalServer()->cleanup();


	getSampleManager().cancelAllJobs();

	getSampleManager().getProjectHandler().removeListener(this);
	getExpansionHandler().removeListener(this);

	deletePendingFlag = true;

	clearPreset(dontSendNotification);

	synthChain = nullptr;

	dllManager->unloadDll();

	handleEditorData(true);
}

InteractionTester* BackendProcessor::getInteractionTester()
{
	return interactionTester.get();
}

MidiInjector* BackendProcessor::getMidiInjector()
{
	return midiInjector.get();
}

void BackendProcessor::showInteractionTestWindow()
{
	// Create tester if it doesn't exist (independent of REST server)
	if (interactionTester == nullptr)
		interactionTester = std::make_unique<InteractionTester>(this);
	
	interactionTester->ensureWindowOpen();
}

void BackendProcessor::projectChanged(const File& /*newRootDirectory*/)
{
	getExpansionHandler().setCurrentExpansion("");

	clearExtraDefinitionCache();

	auto tmp = getCurrentSampleMapPool();
	auto tmp2 = getCurrentMidiFilePool();

	auto f = [tmp, tmp2](Processor*)
	{
		tmp->loadAllFilesFromProjectFolder();
		tmp2->loadAllFilesFromProjectFolder();
		return SafeFunctionCall::OK;
	};

	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);

	refreshExpansionType();
	
    dllManager->loadDll(true);
}

void BackendProcessor::refreshExpansionType()
{
	getSettingsObject().refreshProjectData();
	auto expType = dynamic_cast<GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Project::ExpansionType).toString();

	if (expType == "Disabled")
	{
		getExpansionHandler().setExpansionType<ExpansionHandler::Disabled>();
	}
	else if (expType == "FilesOnly" || expType == "Custom")
	{
		getExpansionHandler().setExpansionType<Expansion>();
		getExpansionHandler().setEncryptionKey({}, dontSendNotification);
	}
	else if (expType == "Full")
	{
		if(HISE_GET_PREPROCESSOR(this, HISE_USE_UNLOCKER_FOR_EXPANSIONS))
		{
			getExpansionHandler().setExpansionType<FullInstrumentExpansion>();
		}
		else
		{
			auto key = dynamic_cast<GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Project::EncryptionKey).toString();

			if (key.isNotEmpty())
			{
				getExpansionHandler().setEncryptionKey(key);
				getExpansionHandler().setExpansionType<FullInstrumentExpansion>();
			}

			else
			{
				PresetHandler::showMessageWindow("Can't initialise full expansions", "You need to specify the encryption key in the Project settings in order to use **Full** expansions", PresetHandler::IconType::Error);
				getExpansionHandler().setExpansionType<ExpansionHandler::Disabled>();
			}
		}
	}
	else if (expType == "Encrypted")
	{
		auto key = dynamic_cast<GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Project::EncryptionKey).toString();
		
		getExpansionHandler().setExpansionType<ScriptEncryptedExpansion>();
		getExpansionHandler().setEncryptionKey(key, dontSendNotification);
	}

	getExpansionHandler().resetAfterProjectSwitch();
}

void BackendProcessor::handleEditorData(bool save)
{
#if IS_STANDALONE_APP
	File jsonFile = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("editorData.json");

	if (save)
	{
		if (editorInformation.isObject())
			jsonFile.replaceWithText(JSON::toString(editorInformation));
		else
			jsonFile.deleteFile();
	}
	else
	{
		editorInformation = JSON::parse(jsonFile);
	}
#else
		ignoreUnused(save);
#endif

		
}

void BackendProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    

#if HISE_INCLUDE_PROFILING_TOOLKIT
	if(getDebugSession().isMidiTriggerEnabled())
	{
		if(!midiMessages.isEmpty())
		{
			MidiBuffer::Iterator iter(midiMessages);

			MidiMessage m;
			int pos;

			auto before = numPressedKeys;

			while(iter.getNextEvent(m, pos))
			{
				if(m.isNoteOn())
					++numPressedKeys;

				if(m.isNoteOff())
					numPressedKeys = jmax(0, numPressedKeys - 1);
			}

			if(before == 0 && numPressedKeys > 0)
			{
				// I know what I'm doing here...
				MainController::ScopedBadBabysitter sbs(this);

				// Cause the recording to start synchronously, let's live with the CPU peak
				MessageManagerLock mm;
				

				getDebugSession().startRecording(-1.0, &getDebugSession());
			}
			else if (before != 0 && numPressedKeys == 0)
			{
				getDebugSession().stopRecording();
			}
		}
	}
#endif

	TRACE_DSP();

	if(externalClockSim.bypassed)
	{
		processBlockBypassed(buffer, midiMessages);
		return;
	}
	
#if !HISE_BACKEND_AS_FX
	buffer.clear();
#endif


	handleLatencyCheck(buffer);

    auto processChunk = [this](float** channels, AudioSampleBuffer& original, MidiBuffer& mb, int offset, int numThisTime)
    {
        for (int i = 0; i < original.getNumChannels(); i++)
            channels[i] = original.getWritePointer(i, offset);

        MidiBuffer chunkMidiBuffer;
        chunkMidiBuffer.addEvents(mb, offset, numThisTime, -offset);

        AudioSampleBuffer chunk(channels, original.getNumChannels(), numThisTime);

#if IS_STANDALONE_APP
		externalClockSim.addTimelineData(chunk, chunkMidiBuffer);
#endif

        getDelayedRenderer().processWrapped(chunk, chunkMidiBuffer);
    };
    
#if IS_STANDALONE_APP
    setPlayHead(&externalClockSim);
    
    auto numBeforeWrap = externalClockSim.getLoopBeforeWrap(buffer.getNumSamples());
    
	// we need to align the loop points to the raster 
	numBeforeWrap -= numBeforeWrap % HISE_EVENT_RASTER;

    if(numBeforeWrap != 0)
    {
        auto numAfterWrap = buffer.getNumSamples() - numBeforeWrap;
        float* channels[HISE_NUM_PLUGIN_CHANNELS];

        processChunk(channels, buffer, midiMessages, 0, numBeforeWrap);
        
        externalClockSim.process(numBeforeWrap);
        
        if(numAfterWrap > 0)
            processChunk(channels, buffer, midiMessages, numBeforeWrap, numAfterWrap);
        
        externalClockSim.process(numAfterWrap);
        
		externalClockSim.sendLoopMessage();

        return;
    }
    
#endif
    
    
    
	if (isUsingDynamicBufferSize())
	{
		int numTodo = buffer.getNumSamples();
		int pos = 0;

		while (numTodo > 0)
		{
			// I'm sure that's how it looks inside there...
			int fruityLoopsBufferSize = Random::getSystemRandom().nextInt({ numTodo / 3, numTodo + 1 });
			
			if (fruityLoopsBufferSize == 0)
				continue;

			if (numTodo < 8)
				fruityLoopsBufferSize = numTodo;

			fruityLoopsBufferSize = jlimit(0, numTodo, fruityLoopsBufferSize);

			

			float* channels[HISE_NUM_PLUGIN_CHANNELS];

            
            
            processChunk(channels, buffer, midiMessages, pos, fruityLoopsBufferSize);

			numTodo -= fruityLoopsBufferSize;
			pos += fruityLoopsBufferSize;
		}
	}
	else
	{
#if IS_STANDALONE_APP
		externalClockSim.addTimelineData(buffer, midiMessages);
#endif

		ScopedAnalyser sa(this, nullptr, buffer, buffer.getNumSamples());

#if IS_STANDALONE_APP
		if(!pluginParameterRamp.processBlock(buffer, midiMessages, processChunk))
			getDelayedRenderer().processWrapped(buffer, midiMessages);
#else
		getDelayedRenderer().processWrapped(buffer, midiMessages);
#endif
			
		
#if IS_STANDALONE_APP
		externalClockSim.addPostTimelineData(buffer, midiMessages);
#endif
	}

#if IS_STANDALONE_APP
    externalClockSim.process(buffer.getNumSamples());
#endif

	handlePostLatencyCheck(buffer);
};

void BackendProcessor::processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
	midiMessages.clear();
}

void BackendProcessor::handleControllersForMacroKnobs(const MidiBuffer &/*midiMessages*/)
{
	
}


void BackendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    externalClockSim.prepareToPlay(newSampleRate);
    
	setRateAndBufferSizeDetails(newSampleRate, samplesPerBlock);
 
	handleLatencyInPrepareToPlay(newSampleRate);

	getDelayedRenderer().prepareToPlayWrapped(newSampleRate, samplesPerBlock);
}

void BackendProcessor::releaseResources()
{
		
};

void BackendProcessor::checkLatency()
{
	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), [](Processor* p)
	{
		auto bp = dynamic_cast<BackendProcessor*>(p->getMainController());

		bp->latencyCheckState = LatencyCheckState::WaitingForKillCounter;
		bp->killCounter = (int)bp->getMainSynthChain()->getSampleRate() * 0.5;

		return SafeFunctionCall::OK;
	}, KillStateHandler::TargetThread::SampleLoadingThread);
}

void BackendProcessor::getStateInformation(MemoryBlock &destData)
{
	if(forceSaveAsPluginState)
	{
		MainController::savePluginState(destData, 0);
	}
	else
	{
		MemoryOutputStream output(destData, false);

		ValueTree v = synthChain->exportAsValueTree();

		v.setProperty("ProjectRootFolder", GET_PROJECT_HANDLER(synthChain).getWorkDirectory().getFullPathName(), nullptr);

		if (auto root = dynamic_cast<BackendRootWindow*>(getActiveEditor()))
			root->saveInterfaceData();

		v.setProperty("InterfaceData", JSON::toString(editorInformation, true, DOUBLE_TO_STRING_DIGITS), nullptr);
		v.writeToStream(output);
	}
}

void BackendProcessor::handleLatencyCheck(AudioSampleBuffer& buffer)
{
	if(latencyCheckState == LatencyCheckState::WaitingForKillCounter)
	{
		killCounter -= buffer.getNumSamples();

		if(killCounter < 0)
		{
			killCounter = 0;
			latencyCheckState = LatencyCheckState::WaitingForProcessBlock;
		}
	}

	if(latencyCheckState == LatencyCheckState::WaitingForProcessBlock)
	{
		reportedLatency = 0.0;
		buffer.setSample(0, 0, 1.0f);
		buffer.setSample(0, 1, 1.0f);
	}
}

void BackendProcessor::handlePostLatencyCheck(AudioSampleBuffer& buffer)
{
	if(latencyCheckState == LatencyCheckState::WaitingForProcessBlock)
	{
		latencyCheckState = LatencyCheckState::WaitingForImpulse;
		reportedLatency = 0.0;
	}

	if(latencyCheckState == LatencyCheckState::WaitingForImpulse)
	{
		if(buffer.getMagnitude(0, 0, buffer.getNumSamples()) > 0.01f)
		{
			float maxPeak = 0.0f;
			float indexOfPeak = 0.0f;

			for(int i = 0; i < buffer.getNumSamples(); i++)
			{
				auto value = buffer.getSample(0, i);
				if(value > maxPeak)
				{
					maxPeak = value;
					indexOfPeak = i;
				}
			}

			reportedLatency += (double)indexOfPeak;

			latencyCheckState = LatencyCheckState::Done;

			MessageManager::callAsync([this]()
			{
				PresetHandler::showMessageWindow("Latency detected", "The latency of the processing chain is:  \n>`" + String((int)reportedLatency) + "` samples.", PresetHandler::IconType::Info);
				latencyCheckState = LatencyCheckState::Idle;
				reportedLatency = 0;
			});
		}
		else
		{
			reportedLatency += buffer.getNumSamples();
		}

		buffer.clear();
	}
}

void BackendProcessor::logMessage(const String& message, bool isCritical)
{
	if (isCritical)
	{
		debugError(getMainSynthChain(), message);
	}
	else
		debugToConsole(getMainSynthChain(), message);
}

void BackendProcessor::setStateInformation(const void *data, int sizeInBytes)
{
	tempLoadingData.setSize(sizeInBytes);

	tempLoadingData.copyFrom(data, 0, sizeInBytes);

	

	auto f = [](Processor* p)
	{
		auto bp = dynamic_cast<BackendProcessor*>(p->getMainController());

		auto& tmp = bp->tempLoadingData;

		ValueTree v = ValueTree::readFromData(tmp.getData(), tmp.getSize());

		String fileName = v.getProperty("ProjectRootFolder", String());

		if (fileName.isNotEmpty())
		{
			File root(fileName);
			if (root.exists() && root.isDirectory())
			{
				GET_PROJECT_HANDLER(p).setWorkingProject(root);

				bp->getSettingsObject().refreshProjectData();

			}
		}

		p->getMainController()->loadPresetFromValueTree(v);

		

		bp->editorInformation = JSON::parse(v.getProperty("InterfaceData", ""));

		tmp.reset();

		return SafeFunctionCall::OK;
	};

	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
}

AudioProcessorEditor* BackendProcessor::createEditor()
{
#if USE_WORKBENCH_EDITOR
	return new SnexWorkbenchEditor(this);
#else
	auto d = new BackendRootWindow(this, editorInformation);
    docWindow = d;
    return d;
#endif
}





juce::File BackendProcessor::getDatabaseRootDirectory() const
{
	if (databaseRoot.isDirectory())
		return databaseRoot;

	auto docRepo = getSettingsObject().getSetting(HiseSettings::Documentation::DocRepository).toString();

	File root;

	if (File::isAbsolutePath(docRepo))
	{
		auto f = File(docRepo);

		if (f.isDirectory())
			root = f;
	}

	return root;
}

hise::BackendProcessor* BackendProcessor::getDocProcessor()
{
    return this;
}

hise::BackendRootWindow* BackendProcessor::getDocWindow()
{
    return docWindow;
    
}

juce::Component* BackendProcessor::getRootComponent()
{
	return dynamic_cast<Component*>(getDocWindow());
}

hise::JavascriptProcessor* BackendProcessor::createInterface(int width, int height, bool compile)
{
	auto midiChain = dynamic_cast<MidiProcessorChain*>(getMainSynthChain()->getChildProcessor(ModulatorSynthChain::MidiProcessor));
	auto s = getMainSynthChain()->getMainController()->createProcessor(midiChain->getFactoryType(), "ScriptProcessor", "Interface");
	auto jsp = dynamic_cast<JavascriptProcessor*>(s);

	String code = "Content.makeFrontInterface(" + String(width) + ", " + String(width) + ");";

	jsp->getSnippet(0)->replaceContentAsync(code, false);

	if (compile)
	{
		jsp->compileScript();
	}
	

	midiChain->getHandler()->add(s, nullptr);

	midiChain->setEditorState(Processor::EditorState::Visible, true);
	s->setEditorState(Processor::EditorState::Folded, true);

	return jsp;
}

void BackendProcessor::setEditorData(var editorState)
{
	editorInformation = editorState;
}

hise::ControlledObject* BackendProcessor::getRestWizardRunner()
{
	if (wizardRunner == nullptr)
		wizardRunner = new RestHelpers::WizardExecutor::AsyncRunner(this);

	return wizardRunner.get();
}

void BackendProcessor::pushToAnalyserBuffer(AnalyserInfo::Ptr info, bool post, const AudioSampleBuffer& buffer, int numSamples)
{
	jassert(info != nullptr);

	if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(postAnalyserLock))
	{
		auto rb = info->ringBuffer[(int)post];

		if(rb != nullptr)
		{
			if(!post)
			{
				for(int i = 0; i < 128; i++)
				{
					if(getKeyboardState().isNoteOn(1, i))
					{
						currentNoteNumber = i;
						break;
					}
				}
			}

			if(!post && (bool)rb->getPropertyObject()->getProperty(scriptnode::PropertyIds::IsProcessingHiseEvent))
			{
				if(currentNoteNumber != -1 && currentNoteNumber != info->lastNoteNumber)
				{
					info->lastNoteNumber = currentNoteNumber;
					auto midiFreq = MidiMessage::getMidiNoteInHertz(info->lastNoteNumber);
					auto cycleLength = getMainSynthChain()->getSampleRate() / midiFreq;
					info->ringBuffer[0]->setMaxLength(cycleLength);
					info->ringBuffer[1]->setMaxLength(cycleLength);
				}
			}

			if(rb->getPropertyObject()->getProperty("ShowCpuUsage"))
			{
				auto numMsForBuffer = ((double)buffer.getNumSamples() / getMainSynthChain()->getSampleRate()) * 1000.0;
				auto usage = jlimit(0.0, 1.0, info->duration / numMsForBuffer);

				rb->write(post ? usage : (getCpuUsage() * 0.01), numSamples);
			}
			else
			{
				jassert(isPositiveAndBelow(info->currentlyAnalysedProcessor.second * 2, buffer.getNumChannels()+1));
				const float* data[2] = { buffer.getReadPointer(info->currentlyAnalysedProcessor.second * 2), buffer.getReadPointer(info->currentlyAnalysedProcessor.second * 2 +1) };
				rb->write(data, 2, buffer.getNumSamples());
			}
		}
	}
}

AnalyserInfo::Ptr BackendProcessor::getAnalyserInfoForProcessor(Processor* p)
{
	if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(postAnalyserLock))
	{
		for(auto i: currentAnalysers)
		{
			if(i->currentlyAnalysedProcessor.first == p)
				return i;
		}
	}

	return nullptr;
}

Result BackendProcessor::setAnalysedProcessor(AnalyserInfo::Ptr newInfo, bool add)
{
	SimpleReadWriteLock::ScopedWriteLock sl(postAnalyserLock);

	if(add)
	{
		for(auto i: currentAnalysers)
		{
			if(i->currentlyAnalysedProcessor.first == newInfo->currentlyAnalysedProcessor.first)
				return Result::fail("Another analyser is already assigned to the module " + i->currentlyAnalysedProcessor.first->getId());
		}
		currentAnalysers.add(newInfo);
	}
			
	else
		currentAnalysers.removeObject(newInfo);

	return Result::ok();
}
} // namespace hise


