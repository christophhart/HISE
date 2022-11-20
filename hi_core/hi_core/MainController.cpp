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

/** The entire HISE codebase


*/
namespace hise { using namespace juce;

#if HI_RUN_UNIT_TESTS
bool MainController::unitTestMode = false;
#endif

MainController::MainController() :

	sampleManager(new SampleManager(this)),
	javascriptThreadPool(new JavascriptThreadPool(this)),
	expansionHandler(this),
	allNotesOffFlag(false),
	processingBufferSize(-1),
	cpuBufferSize(0),
	processingSampleRate(-1.0),
	temp_usage(0.0f),
	uptime(0.0),
	bpm(120.0),
	bpmFromHost(120.0),
	hostIsPlaying(false),
	console(nullptr),
	voiceAmount(0),
	scrollY(0),
	mainLookAndFeel(new GlobalHiseLookAndFeel()),
	mainCommandManager(new ApplicationCommandManager()),
	shownComponents(0),
	plotter(nullptr),
	usagePercent(0),
	scriptWatchTable(nullptr),
	globalPitchFactor(1.0),
	midiInputFlag(false),
	macroManager(this),
	autoSaver(this),
	delayedRenderer(this),
	enablePluginParameterUpdate(true),
	customTypeFaceData(ValueTree("CustomFonts")),
	masterEventBuffer(),
	eventIdHandler(masterEventBuffer),
	lockfreeDispatcher(this),
	userPresetHandler(this),
	codeHandler(this),
	processorChangeHandler(this),
	killStateHandler(this),
	debugLogger(this),
	globalAsyncModuleHandler(this),
	//presetLoadRampFlag(OldUserPresetHandler::Active),
	controlUndoManager(new UndoManager()),
	xyzPool(new MultiChannelAudioBuffer::XYZPool())
{
	PresetHandler::setCurrentMainController(this);

	globalFont = GLOBAL_FONT();

	BACKEND_ONLY(popupConsole = nullptr);
	BACKEND_ONLY(usePopupConsole = false);

	BACKEND_ONLY(shownComponents.setBit(BackendCommandTarget::Keyboard, 1));
	BACKEND_ONLY(shownComponents.setBit(BackendCommandTarget::Macros, 0));

	LOG_START("Initialising MainController"); 

	TempoSyncer::initTempoData();
    
	globalVariableObject = new DynamicObject();

	hostInfo = new DynamicObject();
    
	startTimer(500);
};


MainController::~MainController()
{
	//getControlUndoManager()->clearUndoHistory();

	PresetHandler::setCurrentMainController(nullptr);

	notifyShutdownToRegisteredObjects();

	javascriptThreadPool->cancelAllJobs();
	sampleManager->cancelAllJobs();

	Logger::setCurrentLogger(nullptr);
	logger = nullptr;
	masterReference.clear();
	customTypeFaces.clear();

	sampleManager = nullptr;
	javascriptThreadPool = nullptr;
}


void MainController::notifyShutdownToRegisteredObjects()
{
	for (auto obj : registeredObjects)
	{
		if (obj.get() != nullptr)
			obj->mainControllerIsDeleted();
	}

	registeredObjects.clear();
}

hise::ProjectDocDatabaseHolder* MainController::getProjectDocHolder()
{
	if (projectDocHolder == nullptr)
	{
		projectDocHolder = new ProjectDocDatabaseHolder(this);
#if USE_FRONTEND
		projectDocHolder->setForceCachedDataUse(true);
		projectDocHolder->rebuildDatabase();
#endif
	}

	return projectDocHolder;
}

void MainController::initProjectDocsWithURL(const String& projectDocURL)
{
	getProjectDocHolder()->setProjectURL(URL(projectDocURL));
}


hise::SampleMapPool* MainController::getCurrentSampleMapPool()
{
	if (FullInstrumentExpansion::isEnabled(this))
	{
		if (auto ce = getExpansionHandler().getCurrentExpansion())
			return &ce->pool->getSampleMapPool();
	}

	return &getSampleManager().getProjectHandler().pool->getSampleMapPool();
}

const hise::SampleMapPool* MainController::getCurrentSampleMapPool() const
{
	if (FullInstrumentExpansion::isEnabled(this))
	{
		if (auto ce = getExpansionHandler().getCurrentExpansion())
			return &ce->pool->getSampleMapPool();
	}

	return &getSampleManager().getProjectHandler().pool->getSampleMapPool();
}

const CriticalSection & MainController::getLock() const
{
	if (getDebugLogger().isLogging() && MessageManager::getInstance()->isThisTheMessageThread())
	{
		ScopedTryLock sl(processLock);

		if (sl.isLocked())
		{
			getDebugLogger().setStackBacktrace(SystemStats::getStackBacktrace());
		}
	}

	return processLock;
}

void MainController::loadPresetFromFile(const File &f, Component* /*mainEditor*/)
{
	auto f2 = [f](Processor* p)
	{
		FileInputStream fis(f);

		ValueTree v = ValueTree::readFromStream(fis);
		p->getMainController()->loadPresetFromValueTree(v);
		return SafeFunctionCall::OK;
	};

#if USE_BACKEND
	const bool synchronous = CompileExporter::isExportingFromCommandLine();

	if (synchronous)
		f2(getMainSynthChain());
	else
		killAndCallOnLoadingThread(f2);
#else
	jassertfalse;
#endif
}

void MainController::clearPreset()
{
	Processor::Iterator<Processor> iter(getMainSynthChain(), false);

    
	while (auto p = iter.getNextProcessor())
		p->cleanRebuildFlagForThisAndParents();

	auto f = [](Processor* p)
	{
		auto mc = p->getMainController();
		LockHelpers::freeToGo(mc);

		mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().clear();
		mc->getScriptComponentEditBroadcaster()->getUndoManager().clearUndoHistory();
		mc->getControlUndoManager()->clearUndoHistory();
        mc->getLocationUndoManager()->clearUndoHistory();
        mc->getMasterClock().reset();
        
		mc->setGlobalRoutingManager(nullptr);

		BACKEND_ONLY(mc->getJavascriptThreadPool().getGlobalServer()->setInitialised());
		mc->getMainSynthChain()->reset();
		mc->globalVariableObject->clear();

		for (int i = 0; i < 127; i++)
		{
			mc->setKeyboardCoulour(i, Colours::transparentBlack);
		}

		mc->setCurrentScriptLookAndFeel(nullptr);
		mc->clearIncludedFiles();
		mc->changed = false;
        
        mc->prepareToPlay(mc->processingSampleRate, mc->numSamplesThisBlock);
        
		return SafeFunctionCall::OK;
	};

	if (isBeingDeleted())
		f(getMainSynthChain());
	else
		getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, KillStateHandler::SampleLoadingThread);
}

void MainController::loadPresetFromValueTree(const ValueTree &v, Component* /*mainEditor*/)
{
#if USE_BACKEND
    const bool isCommandLine = CompileExporter::isExportingFromCommandLine();
    const bool isSampleLoadingThread = killStateHandler.getCurrentThread() == KillStateHandler::SampleLoadingThread;
    
	jassert(isCommandLine || isSampleLoadingThread || !isInitialised());
    ignoreUnused(isCommandLine, isSampleLoadingThread);
#endif

	if (v.isValid() && v.getProperty("Type", var::undefined()).toString() == "SynthChain")
	{

		if (v.getType() != Identifier("Processor"))
		{
			jassertfalse;
			
		}

		loadPresetInternal(v);
	}
	else
	{
		PresetHandler::showMessageWindow("No valid container", "This preset is not a container file", PresetHandler::IconType::Error);
	}
}


void MainController::loadPresetInternal(const ValueTree& v)
{
	auto f = [this, v](Processor* )
	{
		LockHelpers::freeToGo(this);

		try
		{
			getSampleManager().setPreloadFlag();

			ModulatorSynthChain *synthChain = getMainSynthChain();

#if USE_BACKEND
			const bool isCommandLine = CompileExporter::isExportingFromCommandLine();
			const bool isSampleLoadingThread = killStateHandler.getCurrentThread() == KillStateHandler::SampleLoadingThread;

			jassert(!isInitialised() || isCommandLine || isSampleLoadingThread);
			ignoreUnused(isCommandLine, isSampleLoadingThread);
#endif

			getSampleManager().setCurrentPreloadMessage("Closing...");

			clearPreset();

			getSampleManager().setShouldSkipPreloading(true);

			

			// Reset the sample rate so that prepareToPlay does not get called in restoreFromValueTree
			// synthChain->setCurrentPlaybackSampleRate(-1.0);
			synthChain->setId(v.getProperty("ID", "MainSynthChain"));

			skipCompilingAtPresetLoad = true;
			getSampleManager().setCurrentPreloadMessage("Building modules...");
			synthChain->restoreFromValueTree(v);
            
            Processor::Iterator<GlobalModulator> gi(synthChain, false);
            
            while(auto m = gi.getNextProcessor())
                m->connectIfPending();
            
            
			skipCompilingAtPresetLoad = false;

			getSampleManager().setCurrentPreloadMessage("Compiling scripts...");

			getMacroManager().getMidiControlAutomationHandler()->setUnloadedData(v.getChildWithName("MidiAutomation"));

			synthChain->compileAllScripts();

			if (processingSampleRate > 0.0)
			{
				LOG_START("Initialising audio callback");

				getSampleManager().setCurrentPreloadMessage("Initialising audio...");
				prepareToPlay(processingSampleRate, processingBufferSize.get());
			}

			// We need to postpone this until after compilation in order to resolve the 
			// attribute indexes for the CC mappings
			getMacroManager().getMidiControlAutomationHandler()->loadUnloadedData();

			synthChain->loadMacrosFromValueTree(v);

#if USE_BACKEND
			Processor::Iterator<ModulatorSynth> iter(synthChain, false);

			while (ModulatorSynth *synth = iter.getNextProcessor())
				synth->setEditorState(Processor::EditorState::Folded, true);

			changed = false;
            
            getJavascriptThreadPool().getGlobalServer()->setInitialised();
#endif

			auto f = [](Dispatchable* obj)
			{
				auto p = static_cast<Processor*>(obj);
			
				p->getMainController()->getSampleManager().setCurrentPreloadMessage("Building UI...");
				p->sendRebuildMessage(true);
				p->getMainController()->getSampleManager().setCurrentPreloadMessage("Done...");
				p->getMainController()->getLockFreeDispatcher().sendPresetReloadMessage();

				return Dispatchable::Status::OK;
			};

			if(USE_BACKEND || FullInstrumentExpansion::isEnabled(this))
				getLockFreeDispatcher().callOnMessageThreadAfterSuspension(synthChain, f);

			allNotesOff(true);
		}
		catch (String& errorMessage)
		{
			ignoreUnused(errorMessage);

#if USE_BACKEND
			writeToConsole(errorMessage, 1, getMainSynthChain());
#else
			DBG(errorMessage);
#endif
		}

		BACKEND_ONLY(getSampleManager().preloadEverything());

		return SafeFunctionCall::OK;
	};
	
	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, KillStateHandler::SampleLoadingThread);
}




void MainController::startCpuBenchmark(int bufferSize_)
{
	cpuBufferSize.set(bufferSize_);
	temp_usage = (Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()));
}

void MainController::compileAllScripts()
{
	Processor::Iterator<JavascriptProcessor> it(getMainSynthChain());

	auto& set = globalVariableObject->getProperties();

	for (int i = 0; i < set.size(); i++)
	{
		set.set(set.getName(i), var());
	}

	JavascriptProcessor *sp;
		
	while((sp = it.getNextProcessor()) != nullptr)
	{
		if (sp->isConnectedToExternalFile())
		{
			sp->reloadFromFile();
		}
		else
		{
			sp->compileScript();
		}
	}
};

void MainController::allNotesOff(bool resetSoftBypassState/*=false*/)
{
#if HI_RUN_UNIT_TESTS
	// Skip the all notes off command for the unit test mode
	if (processingSampleRate == -1.0)
		return;
#endif

	if (resetSoftBypassState)
	{
		auto f = [](Processor* p)
		{
			Processor::Iterator<ModulatorSynth> iter(p);

			while (auto s = iter.getNextProcessor())
			{
				s->updateSoftBypassState();
			}

			// Set the all notes off flag here again
			p->getMainController()->allNotesOff(false);

			return SafeFunctionCall::OK;
		};

		getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, KillStateHandler::TargetThread::SampleLoadingThread);

		
	}
	else
	{
		allNotesOffFlag = true;
	}
}

void MainController::stopCpuBenchmark()
{
	const float thisUsage = 100.0f * (float)((Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()) - temp_usage) * getOriginalSamplerate() / cpuBufferSize.get());
	
	const float lastUsage = usagePercent.load();
	
	if (thisUsage > lastUsage)
	{
		usagePercent.store(thisUsage);
	}
	else
	{
		usagePercent.store(lastUsage*0.99f);
	}
}

void MainController::killAndCallOnAudioThread(const ProcessorFunction& f)
{
	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, KillStateHandler::AudioThread);
}

void MainController::killAndCallOnLoadingThread(const ProcessorFunction& f)
{
	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, KillStateHandler::SampleLoadingThread);
}

void MainController::sendToMidiOut(const HiseEvent& e)
{
	// Only send out artificial notes
	jassert(e.isArtificial());

	SimpleReadWriteLock::ScopedWriteLock sl(midiOutputLock);

	outputMidiBuffer.addEvent(e);
}

bool MainController::refreshOversampling()
{
	auto requiredOversamplingFactor = (double)jlimit(1, 8, nextPowerOfTwo((int)(minimumSamplerate / getOriginalSamplerate())));

	int numChannelsOfOversampler = oversampler != nullptr ? (int)oversampler->numChannels : -1;

	bool channelsNeedUpdating = numChannelsOfOversampler > 0 && numChannelsOfOversampler != multiChannelBuffer.getNumChannels();

	if (requiredOversamplingFactor != getOversampleFactor() || channelsNeedUpdating)
	{
		auto f = [this, requiredOversamplingFactor](Processor* p)
		{
			ScopedPointer<juce::dsp::Oversampling<float>> newOversampler;
			
			if(requiredOversamplingFactor != 1)
				newOversampler = new juce::dsp::Oversampling<float>(multiChannelBuffer.getNumChannels(),
				log2(requiredOversamplingFactor),
				juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR);

			{
				ScopedLock sl(getLock());
				std::swap(oversampler, newOversampler);

				auto originalSampleRate = getOriginalSamplerate();
				auto originalBufferSize = getOriginalBufferSize();

				currentOversampleFactor = requiredOversamplingFactor;

				prepareToPlay(originalSampleRate, originalBufferSize);

				return SafeFunctionCall::OK;
			}
		};

		allNotesOff(false);
		getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, KillStateHandler::SampleLoadingThread);

		return true;
	}

	return false;
}



void MainController::processMidiOutBuffer(MidiBuffer& mb, int numSamples)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(midiOutputLock))
	{
		if (!outputMidiBuffer.isEmpty())
		{
			HiseEventBuffer thisTime;
			outputMidiBuffer.moveEventsBelow(thisTime, numSamples);

			HiseEventBuffer::Iterator it(thisTime);

			while (auto e = it.getNextEventPointer(true, false))
			{
				mb.addEvent(e->toMidiMesage(), e->getTimeStamp());
			}

			outputMidiBuffer.subtractFromTimeStamps(numSamples);
		}
	}
}

bool MainController::shouldUseSoftBypassRamps() const noexcept
{
#if USE_BACKEND || !FRONTEND_IS_PLUGIN
	return true;
#else
	return allowSoftBypassRamps;
#endif
}

void callOnAllChildren(Component* c, const std::function<void(Component*)>& f)
{
	f(c);

	for (int i = 0; i < c->getNumChildComponents(); i++)
		callOnAllChildren(c->getChildComponent(i), f);
}

void MainController::resetLookAndFeelToDefault(Component* c)
{
	GlobalScriptCompileBroadcaster::setCurrentScriptLookAndFeel(nullptr);

	auto newLaf = new hise::GlobalHiseLookAndFeel();
	newLaf->setComboBoxFont(globalFont);

	callOnAllChildren(c, [newLaf, this](Component* c)
	{
		if (dynamic_cast<ScriptingObjects::ScriptedLookAndFeel::Laf*>(&c->getLookAndFeel()))
		{
			if(dynamic_cast<MacroControlledObject*>(c) != nullptr)
				skin(*c);

			c->setLookAndFeel(newLaf);
		}
	});

	mainLookAndFeel = newLaf;
}

void MainController::setCurrentScriptLookAndFeel(ReferenceCountedObject* newLaf)
{
	GlobalScriptCompileBroadcaster::setCurrentScriptLookAndFeel(newLaf);

	if(newLaf == nullptr && dynamic_cast<ScriptingObjects::ScriptedLookAndFeel::Laf*>(mainLookAndFeel.get()) != nullptr)
		mainLookAndFeel = new hise::GlobalHiseLookAndFeel();
	else if(newLaf != nullptr)
		mainLookAndFeel = new ScriptingObjects::ScriptedLookAndFeel::Laf(this);
}

void MainController::setMaximumBlockSize(int newBlockSize)
{
	newBlockSize -= (newBlockSize % HISE_EVENT_RASTER);

	if (newBlockSize != maximumBlockSize)
	{
		maximumBlockSize = jlimit(16, HISE_MAX_PROCESSING_BLOCKSIZE, newBlockSize);

		if(originalBufferSize > 0)
			prepareToPlay(getOriginalSamplerate(), getOriginalBufferSize());
	}
}

int MainController::getNumActiveVoices() const
{
	return getMainSynthChain()->getNumActiveVoices();
}

void MainController::beginParameterChangeGesture(int index)			{ dynamic_cast<PluginParameterAudioProcessor*>(this)->beginParameterChangeGesture(index); }

void MainController::endParameterChangeGesture(int index)			{ dynamic_cast<PluginParameterAudioProcessor*>(this)->endParameterChangeGesture(index); }

void MainController::setPluginParameter(int index, float newValue)  { dynamic_cast<PluginParameterAudioProcessor*>(this)->setParameterNotifyingHost(index, newValue); }

Processor *MainController::createProcessor(FactoryType *factory,
											 const Identifier &typeName,
											 const String &id)
{		
	// Every chain must have a factory type!
	jassert(factory != nullptr);

	// Create the processor using the factory type of the parent chain
	Processor *p = factory->createProcessor(factory->getProcessorTypeIndex(typeName), id);

	return p;
};


void MainController::stopBufferToPlay()
{
	if (previewBufferIndex != -1)
	{
		bool sendToListeners = true;

		{
			LockHelpers::SafeLock sl(this, LockHelpers::AudioLock);

			previewFunction = {};

			if (previewBufferIndex != -1 && !fadeOutPreviewBuffer)
			{
				fadeOutPreviewBufferGain = 1.0f;
				fadeOutPreviewBuffer = true;
				sendToListeners = false;
			}
		}

		if (sendToListeners)
		{
			for (auto pl : previewListeners)
			{
				pl->previewStateChanged(false, previewBuffer);
			}
		}
	}
}

void MainController::setBufferToPlay(const AudioSampleBuffer& buffer, const std::function<void(int)>& pf)
{
	if (buffer.getNumSamples() > 400000 && getKillStateHandler().getCurrentThread() != KillStateHandler::SampleLoadingThread)
	{
		AudioSampleBuffer copy;
		copy.makeCopyOf(buffer);

		killAndCallOnLoadingThread([copy, pf](Processor* p)
		{
			p->getMainController()->setBufferToPlay(copy, pf);
			return SafeFunctionCall::OK;
		});

		return;
	}

	{
		LockHelpers::SafeLock sl(this, LockHelpers::AudioLock);

		previewBufferIndex = 0;
		previewBuffer = buffer;
		previewFunction = pf;
		fadeOutPreviewBuffer = false;
		fadeOutPreviewBufferGain = 1.0f;
	}

	for (auto pl : previewListeners)
	{
		pl->previewStateChanged(true, previewBuffer);
	}
}

int MainController::getPreviewBufferPosition() const
{
	return previewBufferIndex;
}

void MainController::setKeyboardCoulour(int keyNumber, Colour colour)
{
	keyboardState.setColourForSingleKey(keyNumber, colour);
}

CustomKeyboardState & MainController::getKeyboardState()
{
	return keyboardState;
}

void MainController::setLowestKeyToDisplay(int lowestKeyToDisplay)
{
	keyboardState.setLowestKeyToDisplay(lowestKeyToDisplay);
};

float MainController::getVoiceAmountMultiplier() const
{
    if(HiseDeviceSimulator::isAUv3())
    {
        return 0.25f;
    }
    
	auto m = dynamic_cast<const GlobalSettingManager*>(this)->voiceAmountMultiplier;

	switch (m)
	{
	case 8:  return 0.125f;
	case 4:  return 0.25f;
	case 2:  return 0.5f;
	case 1: return 1.0f;
	default:  return 1.0f;
	}
}

#if HISE_INCLUDE_RLOTTIE
hise::RLottieManager::Ptr MainController::getRLottieManager()
{
	if (rLottieManager == nullptr)
	{
		rLottieManager = new HiseRLottieManager(this);
		rLottieManager->init();

		auto r = rLottieManager->getInitResult();

		if (!r.wasOk())
		{
			sendOverlayMessage(OverlayMessageBroadcaster::CustomErrorMessage, r.getErrorMessage());
		}
	}

	return rLottieManager.get();
}
#endif

void MainController::processBlockCommon(AudioSampleBuffer &buffer, MidiBuffer &midiMessages)
{
	if (getKillStateHandler().getStateLoadFlag())
		return;

	AudioThreadGuard audioThreadGuard(&getKillStateHandler());

	getSampleManager().handleNonRealtimeState();

	ADD_GLITCH_DETECTOR(getMainSynthChain(), DebugLogger::Location::MainRenderCallback);
    
	getDebugLogger().checkAudioCallbackProperties(thisAsProcessor->getSampleRate(), numSamplesThisBlock);

	ScopedNoDenormals snd;

	getDebugLogger().checkPriorityInversion(processLock);

	numSamplesThisBlock = buffer.getNumSamples();

#if !FRONTEND_IS_PLUGIN
	if (!getKillStateHandler().handleKillState())
	{
		buffer.clear();

		MidiBuffer::Iterator it(midiMessages);

		MidiMessage m;
		int samplePos;
		while(it.getNextEvent(m, samplePos))
		{
			if (m.isNoteOff())
				suspendedNoteOns.insert(HiseEvent(m));
		}

		midiMessages.clear();

		if (processingSampleRate > 0.0)
			uptime += double(numSamplesThisBlock) / getOriginalSamplerate() ;

		return;
	}

#else
	getKillStateHandler().handleKillState();

	if (!getKillStateHandler().handleBufferDuringSuspension(buffer))
		return;
#endif

	ScopedTryLock sl(processLock);

	if (!sl.isLocked())
	{
		// Something long is taking the audio lock without suspending.
		// Use killVoicesAndCall for this
		jassertfalse;

		usagePercent.store(0.0);
		buffer.clear();
		midiMessages.clear();

		if (processingSampleRate > 0.0)
			uptime += double(numSamplesThisBlock) / getOriginalSamplerate();

		return;
	}

	ModulatorSynthChain *synthChain = getMainSynthChain();

	jassert(getOriginalBufferSize() >= numSamplesThisBlock);

#if !FRONTEND_IS_PLUGIN || HISE_ENABLE_MIDI_INPUT_FOR_FX
    
	keyboardState.processNextMidiBuffer(midiMessages, 0, numSamplesThisBlock, true);

	getMacroManager().getMidiControlAutomationHandler()->handleParameterData(midiMessages); // TODO_BUFFER: Move this after the next line...

	masterEventBuffer.addEvents(midiMessages);
	masterEventBuffer.alignEventsToRaster<HISE_EVENT_RASTER>(numSamplesThisBlock);
	
	if (maxEventTimestamp != 0)
	{
		int maxAligned = maxEventTimestamp - maxEventTimestamp % HISE_EVENT_RASTER;

		for (auto& e : masterEventBuffer)
		{
			if (e.getTimeStamp() > maxAligned)
				e.setTimeStamp(maxAligned);
		}
	}

	handleSuspendedNoteOffs();

    if (!masterEventBuffer.isEmpty()) setMidiInputFlag();
    
	checkAllNotesOff();
	eventIdHandler.handleEventIds();

	getDebugLogger().logEvents(masterEventBuffer);

#else
	ignoreUnused(midiMessages);

	masterEventBuffer.clear();
#endif

#if ENABLE_HOST_INFO
	AudioPlayHead::CurrentPositionInfo newTime;
	MasterClock::GridInfo gridInfo;

	bool useTime = false;

	if (getMasterClock().allowExternalSync() && thisAsProcessor->getPlayHead() != nullptr)
	{
		useTime = thisAsProcessor->getPlayHead()->getCurrentPosition(newTime);

		// the time creation failed (probably because we're exporting
		// so we use the time info from the internal clock...
		if (!useTime)
			newTime = getMasterClock().createInternalPlayHead();

	}

	if (getMasterClock().shouldCreateInternalInfo(newTime))
	{
		gridInfo = getMasterClock().processAndCheckGrid(buffer.getNumSamples(), newTime);
		newTime = getMasterClock().createInternalPlayHead();
		useTime = true;
	}
	else 
	{
		gridInfo = getMasterClock().updateFromExternalPlayHead(newTime, buffer.getNumSamples());
	}

	if (useTime)
	{
		handleTransportCallbacks(newTime, gridInfo);
		lastPosInfo = newTime;
	}
	else
		lastPosInfo.resetToDefault();

	storePlayheadIntoDynamicObject(lastPosInfo);
	
	bpmFromHost = lastPosInfo.bpm;

	if (hostIsPlaying != lastPosInfo.isPlaying)
	{
		hostIsPlaying = lastPosInfo.isPlaying;

		FX_ONLY(masterEventBuffer.addEvent(HiseEvent(hostIsPlaying ? HiseEvent::Type::NoteOn :
															 HiseEvent::Type::NoteOff, 
											 60, 127, 1));)

	}

	if (bpmFromHost == 0.0)
		bpmFromHost = 120.0;

	auto otherBpm = *hostBpmPointer;

	if (otherBpm > 0)
		setBpm((double)otherBpm);
	else
	{
		setBpm(bpmFromHost);
	}
	
#endif

	

#if ENABLE_CPU_MEASUREMENT
	startCpuBenchmark(numSamplesThisBlock);
#endif

#if !FRONTEND_IS_PLUGIN

#if !FORCE_INPUT_CHANNELS
	buffer.clear();
#endif

	

#endif


#if USE_MIDI_CONTROLLERS_FOR_MACROS
	handleControllersForMacroKnobs(midiMessages);
#endif

	if (oversampler != nullptr)
		masterEventBuffer.multiplyTimestamps(getOversampleFactor());
	
#if FRONTEND_IS_PLUGIN

    const bool isUsingMultiChannel = multiChannelBuffer.getNumChannels() > 2;
    
    if(isUsingMultiChannel)
    {
		AudioSampleBuffer thisMultiChannelBuffer(multiChannelBuffer.getArrayOfWritePointers(), multiChannelBuffer.getNumChannels(), 0, numSamplesThisBlock);
		thisMultiChannelBuffer.clear();

		FloatVectorOperations::copy(thisMultiChannelBuffer.getWritePointer(0), buffer.getReadPointer(0), numSamplesThisBlock);
		FloatVectorOperations::copy(thisMultiChannelBuffer.getWritePointer(1), buffer.getReadPointer(1), numSamplesThisBlock);

		if (oversampler == nullptr)
		{
			synthChain->renderNextBlockWithModulators(thisMultiChannelBuffer, masterEventBuffer);
		}
		else
		{
			dsp::AudioBlock<float> osInput(thisMultiChannelBuffer);

			auto osOutput = oversampler->processSamplesUp(osInput);
			auto data = (float**)alloca(sizeof(void*) * multiChannelBuffer.getNumChannels());

			for (int i = 0; i < osOutput.getNumChannels(); i++)
				data[i] = osOutput.getChannelPointer(i);

			AudioSampleBuffer thisMultiChannelBufferOs(data, osOutput.getNumChannels(), osOutput.getNumSamples());
			synthChain->renderNextBlockWithModulators(thisMultiChannelBufferOs, masterEventBuffer);
			oversampler->processSamplesDown(osInput);
		}

		buffer.clear();

		// Just use the first two channels. You need to route back all your send channels to the first stereo pair.
		FloatVectorOperations::copy(buffer.getWritePointer(0), thisMultiChannelBuffer.getReadPointer(0), numSamplesThisBlock);
		FloatVectorOperations::copy(buffer.getWritePointer(1), thisMultiChannelBuffer.getReadPointer(1), numSamplesThisBlock);
    }
    else
    {
		if (oversampler == nullptr)
			synthChain->renderNextBlockWithModulators(buffer, masterEventBuffer);
		else
		{
			dsp::AudioBlock<float> osInput(buffer);

			auto osOutput = oversampler->processSamplesUp(osInput);
			buffer.clear();
			float* data[2] = { osOutput.getChannelPointer(0), osOutput.getChannelPointer(1) };

			AudioSampleBuffer rBuffer(data, 2, numSamplesThisBlock * getOversampleFactor());
			synthChain->renderNextBlockWithModulators(rBuffer, masterEventBuffer);
			oversampler->processSamplesDown(osInput);
		}
    }

#elif HISE_MIDIFX_PLUGIN

	synthChain->processHiseEventBuffer(masterEventBuffer, numSamplesThisBlock);

	midiMessages.clear();

	HiseEventBuffer::Iterator it(synthChain->eventBuffer);

	while (auto e = it.getNextConstEventPointer(true, false))
	{
		if (e->isTimerEvent())
			continue;

		auto m = e->toMidiMesage();

		midiMessages.addEvent(m, e->getTimeStamp());
	}

	processMidiOutBuffer(midiMessages, numSamplesThisBlock);

#else


	AudioSampleBuffer thisMultiChannelBuffer(multiChannelBuffer.getArrayOfWritePointers(), multiChannelBuffer.getNumChannels(), 0, numSamplesThisBlock);

	

	

	jassert(thisMultiChannelBuffer.getNumSamples() <= multiChannelBuffer.getNumSamples());

#if FORCE_INPUT_CHANNELS
	jassert(thisMultiChannelBuffer.getNumSamples() >= buffer.getNumSamples());
	jassert(thisMultiChannelBuffer.getNumChannels() >= buffer.getNumChannels());
	thisMultiChannelBuffer.makeCopyOf(buffer, true);
#else
	thisMultiChannelBuffer.clear();
#endif

	

	if (previewBufferIndex != -1)
	{
		int numToPlay = jmin<int>(numSamplesThisBlock, previewBuffer.getNumSamples() - previewBufferIndex);

		if (numToPlay > 0)
		{
            for(int i = 0; i < multiChannelBuffer.getNumChannels(); i++)
            {
                if(isPositiveAndBelow(i, previewBuffer.getNumChannels()))
                {
                    FloatVectorOperations::copy(multiChannelBuffer.getWritePointer(i, 0), previewBuffer.getReadPointer(i, previewBufferIndex), numToPlay);
                }
            }
            
			if (previewFunction)
				previewFunction(previewBufferIndex);

			previewBufferIndex += numToPlay;
		}

		if (fadeOutPreviewBuffer)
		{
			float thisGain = fadeOutPreviewBufferGain;
			fadeOutPreviewBufferGain = jmax<float>(thisGain * 0.75f, 0.0f);

			multiChannelBuffer.applyGainRamp(0, numSamplesThisBlock, thisGain, fadeOutPreviewBufferGain);

			if (fadeOutPreviewBufferGain <= 0.001f)
			{
				previewBuffer = AudioSampleBuffer();
				previewBufferIndex = -1;
                
                for(auto pl: previewListeners)
                {
                    if(pl != nullptr)
                        pl->previewStateChanged(false, previewBuffer);
                }
			}
		}
		
		if (previewBufferIndex >= previewBuffer.getNumSamples())
		{
			previewBuffer = AudioSampleBuffer();
			previewBufferIndex = -1;
            
            for(auto pl: previewListeners)
            {
                if(pl != nullptr)
                    pl->previewStateChanged(false, previewBuffer);
            }
		}
	}

	if (getOversampleFactor() != 1)
	{
		dsp::AudioBlock<float> osInput(thisMultiChannelBuffer);
		auto osOutput = oversampler->processSamplesUp(osInput);

		auto d = (float**)alloca(sizeof(void*) * osOutput.getNumChannels());

		for (int i = 0; i < osOutput.getNumChannels(); i++)
			d[i] = osOutput.getChannelPointer(i);
		
		thisMultiChannelBuffer.setDataToReferTo(d, (int)osOutput.getNumChannels(), (int)osOutput.getNumSamples());
	}




	synthChain->renderNextBlockWithModulators(thisMultiChannelBuffer, masterEventBuffer);

	const bool isUsingMultiChannel = buffer.getNumChannels() != 2;

	if (!isUsingMultiChannel)
	{
		if (getOversampleFactor() != 1)
		{
			dsp::AudioBlock<float> output(buffer);
			oversampler->processSamplesDown(output);
		}
		else
		{
			FloatVectorOperations::copy(buffer.getWritePointer(0), thisMultiChannelBuffer.getReadPointer(0), numSamplesThisBlock);
			FloatVectorOperations::copy(buffer.getWritePointer(1), thisMultiChannelBuffer.getReadPointer(1), numSamplesThisBlock);
		}
	}
	else
	{
		auto& matrix = getMainSynthChain()->getMatrix();

        int maxNumChannelAmount = jmin(matrix.getNumSourceChannels(), buffer.getNumChannels(), thisMultiChannelBuffer.getNumChannels());
     
		if (getOversampleFactor() != 1)
		{
			dsp::AudioBlock<float> output(buffer.getArrayOfWritePointers(), maxNumChannelAmount, 0, numSamplesThisBlock);
			oversampler->processSamplesDown(output);
		}
		else
		{
			for (int i = 0; i < maxNumChannelAmount; i++)
				FloatVectorOperations::copy(buffer.getWritePointer(i), thisMultiChannelBuffer.getReadPointer(i), numSamplesThisBlock);
		}

		
	}

#if USE_HARD_CLIPPER
	
#else
	// on iOS samples above 1.0f create a nasty digital distortion
	if (HiseDeviceSimulator::isMobileDevice())
	{
		for (int i = 0; i < buffer.getNumChannels(); i++)
			FloatVectorOperations::clip(buffer.getWritePointer(i, 0), buffer.getReadPointer(i, 0), -1.0f, 1.0f, numSamplesThisBlock);
	}
#endif

	

#endif

#if ENABLE_CPU_MEASUREMENT
	stopCpuBenchmark();
#endif

    if(processingSampleRate > 0.0)
    {
        uptime += double(numSamplesThisBlock) / getOriginalSamplerate();
    }

#if USE_BACKEND
	getDebugLogger().recordOutput(buffer);
#endif

#if !HISE_MIDIFX_PLUGIN
	midiMessages.clear();

	processMidiOutBuffer(midiMessages, numSamplesThisBlock);
#endif

}

void MainController::setPlotter(Plotter *p)
{
	plotter = p;
};

void MainController::skin(Component &c)
{
	c.setLookAndFeel(mainLookAndFeel);
    
	GlobalHiseLookAndFeel::setDefaultColours(c);
	

#if 0
    if(dynamic_cast<Slider*>(&c) != nullptr) 
		dynamic_cast<Slider*>(&c)->setScrollWheelEnabled(false);
#endif
};


void MainController::storePlayheadIntoDynamicObject(AudioPlayHead::CurrentPositionInfo &/*newPosition*/)
{
	//static const Identifier bpmId("bpm");
	//static const Identifier timeSigNumerator("timeSigNumerator");
	//static const Identifier timeSigDenominator("timeSigDenominator");
	//static const Identifier timeInSamples("timeInSamples");
	//static const Identifier timeInSeconds("timeInSeconds");
	//static const Identifier editOriginTime("editOriginTime");
	//static const Identifier ppqPosition("ppqPosition");
	//static const Identifier ppqPositionOfLastBarStart("ppqPositionOfLastBarStart");
	//static const Identifier frameRate("frameRate");
	//static const Identifier isPlaying("isPlaying");
	//static const Identifier isRecording("isRecording");
	//static const Identifier ppqLoopStart("ppqLoopStart");
	//static const Identifier ppqLoopEnd("ppqLoopEnd");
	//static const Identifier isLooping("isLooping");

	//ScopedLock sl(getLock());

	//hostInfo->setProperty(bpmId, newPosition.bpm);
	//hostInfo->setProperty(timeSigNumerator, newPosition.timeSigNumerator);
	//hostInfo->setProperty(timeSigDenominator, newPosition.timeSigDenominator);
	//hostInfo->setProperty(timeInSamples, newPosition.timeInSamples);
	//hostInfo->setProperty(timeInSeconds, newPosition.timeInSeconds);
	//hostInfo->setProperty(editOriginTime, newPosition.editOriginTime);
	//hostInfo->setProperty(ppqPosition, newPosition.ppqPosition);
	//hostInfo->setProperty(ppqPositionOfLastBarStart, newPosition.ppqPositionOfLastBarStart);
	//hostInfo->setProperty(frameRate, newPosition.frameRate);
	//hostInfo->setProperty(isPlaying, newPosition.isPlaying);
	//hostInfo->setProperty(isRecording, newPosition.isRecording);
	//hostInfo->setProperty(ppqLoopStart, newPosition.ppqLoopStart);
	//hostInfo->setProperty(ppqLoopEnd, newPosition.ppqLoopEnd);
	//hostInfo->setProperty(isLooping, newPosition.isLooping);
}

void MainController::prepareToPlay(double sampleRate_, int samplesPerBlock)
{
	auto oldSampleRate = processingSampleRate;
	auto oldBlockSize = processingBufferSize.get();

	originalBufferSize = samplesPerBlock;
	originalSampleRate = sampleRate_;

	LOG_START("Preparing playback");
    
	processingBufferSize = jmin(maximumBlockSize, originalBufferSize) * currentOversampleFactor;
	processingSampleRate = originalSampleRate * currentOversampleFactor;
 
	hostBpmPointer = &dynamic_cast<GlobalSettingManager*>(this)->globalBPM;

	// Prevent high buffer sizes from blowing up the 350MB limitation...
	if (HiseDeviceSimulator::isAUv3())
	{
		processingBufferSize = jmin<int>(processingBufferSize.get(), 1024);
	}

	if ((processingBufferSize.get() % HISE_EVENT_RASTER != 0) && HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE)
	{
		sendOverlayMessage(State::CustomErrorMessage, "The buffer size " + String(processingBufferSize.get()) + " is not supported. Use a multiple of " + String(HISE_EVENT_RASTER));
	}

    thisAsProcessor = dynamic_cast<AudioProcessor*>(this);
    
#if ENABLE_CONSOLE_OUTPUT && !HI_RUN_UNIT_TESTS
	if (logger == nullptr)
	{
		logger = new ConsoleLogger(getMainSynthChain());
		Logger::setCurrentLogger(logger);
	}

#endif
    
	updateMultiChannelBuffer(getMainSynthChain()->getMatrix().getNumSourceChannels());
	

#if IS_STANDALONE_APP || IS_STANDALONE_FRONTEND
	getMainSynthChain()->getMatrix().setNumDestinationChannels(2);
#else
    
#if HISE_IOS
    getMainSynthChain()->getMatrix().setNumDestinationChannels(2);
#else
	getMainSynthChain()->getMatrix().setNumDestinationChannels(HISE_NUM_PLUGIN_CHANNELS);
#endif
    
#endif

	getMainSynthChain()->prepareToPlay(processingSampleRate, processingBufferSize.get());

	AudioThreadGuard guard(&getKillStateHandler());

	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

	LockHelpers::SafeLock itLock(this, LockHelpers::IteratorLock);
	LockHelpers::SafeLock audioLock(this, LockHelpers::AudioLock);

	getMainSynthChain()->setIsOnAir(true);

	if (oversampler != nullptr)
		oversampler->initProcessing(getOriginalBufferSize());

	auto changed = oldBlockSize != processingBufferSize.get() ||
				   oldSampleRate != processingSampleRate;

	if (changed)
	{
		String s;
		s << "New Buffer Specifications: ";
		s << "Samplerate: " << processingSampleRate;
		s << ", Buffersize: " << String(processingBufferSize.get());
		getConsoleHandler().writeToConsole(s, 0, getMainSynthChain(), Colours::white.withAlpha(0.4f));
	}

	getMasterClock().setSamplerate(processingSampleRate);
}

void MainController::setBpm(double newTempo)
{
	if(bpm != newTempo)
	{
		getMasterClock().setBpm(newTempo);

		bpm = newTempo;

		for (auto& t : tempoListeners)
		{
			if (auto t_ = t.get())
				t_->tempoChanged(bpm);
			else
			{
				// delete it with removeTempoListener!
				jassertfalse;
			}
		}
	}
};

void MainController::setHostBpm(double newTempo)
{
	if (newTempo > 0.0)
	{
		auto nt = jlimit(32.0, 280.0, newTempo);

		dynamic_cast<GlobalSettingManager*>(this)->globalBPM = nt;
		
		setBpm(newTempo);
	}
	else
	{
		dynamic_cast<GlobalSettingManager*>(this)->globalBPM = -1;
		
		setBpm(bpmFromHost);
	}
}

bool MainController::isSyncedToHost() const
{
#if IS_STANDALONE_FRONTEND
	return false;
#else
	return dynamic_cast<const GlobalSettingManager*>(this)->globalBPM == -1;
#endif
}

void MainController::handleTransportCallbacks(const AudioPlayHead::CurrentPositionInfo& newInfo, const MasterClock::GridInfo& gi)
{
	if (lastPosInfo.isPlaying != newInfo.isPlaying || (gi.change && gi.firstGridInPlayback))
	{
		for (auto tl : tempoListeners)
			if(tl != nullptr)
				tl->onTransportChange(newInfo.isPlaying, newInfo.ppqPosition);
	}

	if (lastPosInfo.timeSigNumerator != newInfo.timeSigNumerator ||
		lastPosInfo.timeSigDenominator != newInfo.timeSigDenominator)
	{
		for (auto tl : tempoListeners)
			if(tl != nullptr)
				tl->onSignatureChange(newInfo.timeSigNumerator, newInfo.timeSigDenominator);
	}

	if (!pulseListener.isEmpty())
	{
		auto timeMultiplier = (double)newInfo.timeSigDenominator / 4.0;

		auto realPos = newInfo.ppqPosition * timeMultiplier;
		auto lastPos = (int)(lastPosInfo.ppqPosition * (double)timeMultiplier);


		int beats = (int)realPos;
		if (beats != lastPos)
		{
			bool isBar = false;

			if (newInfo.ppqPositionOfLastBarStart != 0.0)
				isBar = (newInfo.ppqPosition - newInfo.ppqPositionOfLastBarStart) < (1.0 / timeMultiplier);
			else
			{
				isBar = beats % newInfo.timeSigDenominator == 0;
			}

			for (auto tl : pulseListener)
				if(tl != nullptr)
					tl->onBeatChange(beats, isBar);
		}

		if (gi.change)
		{
			for (auto tl : pulseListener)
				if(tl != nullptr)
					tl->onGridChange(gi.gridIndex, gi.timestamp, gi.firstGridInPlayback);
		}
	}
}

void MainController::addTempoListener(TempoListener *t)
{
	{
		LockHelpers::SafeLock sl(this, LockHelpers::AudioLock);
		tempoListeners.addIfNotAlreadyThere(t);
	}
	
	t->tempoChanged(getBpm());
	t->onSignatureChange(lastPosInfo.timeSigNumerator, lastPosInfo.timeSigDenominator);
	t->onTransportChange(lastPosInfo.isPlaying, lastPosInfo.ppqPosition);
}

void MainController::removeTempoListener(TempoListener *t)
{
	LockHelpers::SafeLock sl(this, LockHelpers::AudioLock);
	tempoListeners.removeAllInstancesOf(t);
}

void MainController::addMusicalUpdateListener(TempoListener* t)
{
	LockHelpers::SafeLock sl(this, LockHelpers::AudioLock);
	pulseListener.addIfNotAlreadyThere(t);
}

void MainController::removeMusicalUpdateListener(TempoListener* t)
{
	LockHelpers::SafeLock sl(this, LockHelpers::AudioLock);
	pulseListener.removeAllInstancesOf(t);
}

juce::Typeface* MainController::getFont(const String &fontName) const
{
	for (auto& tf: customTypeFaces)
	{
		auto nameToUse = tf.id.isValid() ? tf.id.toString() : tf.typeface->getName();

		if (nameToUse == fontName)
			return tf.typeface.get();
	}

	return nullptr;
}

Font MainController::getFontFromString(const String& fontName, float fontSize) const
{
	if (fontName == "Default")
		return globalFont;

	const Identifier id(fontName);

	for (auto& tf : customTypeFaces)
	{
		if (tf.id.isValid() && tf.id == id)
		{
			Font currentFont;
			juce::Typeface::Ptr typeface = tf.typeface;
			return Font(typeface).withHeight(fontSize);
		}
	}

	static const String boldString(" Bold");
	static const String italicString(" Italic");

	bool isBold = fontName.contains(boldString);
	bool isItalic = fontName.contains(italicString);

	auto fn = fontName.replace(boldString, "");
	fn = fn.replace(italicString, "");

	Font currentFont;

	juce::Typeface::Ptr typeface = getFont(fn);

	if (typeface != nullptr)	currentFont = Font(typeface).withHeight(fontSize);
	else						currentFont = Font(fn, fontSize, Font::plain);

	if (isBold)					currentFont = currentFont.boldened();
	if (isItalic)				currentFont = currentFont.italicised();

	return currentFont;
}


void MainController::setGlobalFont(const String& fontName)
{
	if (fontName.isEmpty())
		globalFont = GLOBAL_FONT();
	else 
		globalFont = getFontFromString(fontName, 14.0f);

	mainLookAndFeel->setComboBoxFont(globalFont);
}

void MainController::checkAndAbortMessageThreadOperation()
{
	jassert_dispatched_message_thread(this);

	if (shouldAbortMessageThreadOperation())
	{
		throw LockFreeDispatcher::AbortSignal();
	}
}

void MainController::fillWithCustomFonts(StringArray &fontList)
{
	for (auto& tf : customTypeFaces)
	{
		auto nameToUse = tf.id.isValid() ? tf.id.toString() : tf.typeface->getName();
		fontList.addIfNotAlreadyThere(nameToUse);
	}
}

void MainController::loadTypeFace(const String& fileName, const void* fontData, size_t fontDataSize, const String& fontId/*=String()*/)
{
	if (customTypeFaceData.getChildWithProperty("Name", fileName).isValid()) return;

	if (fontId.isNotEmpty() && customTypeFaceData.getChildWithProperty("FontId", fontId).isValid()) return;

	Identifier id_ = fontId.isEmpty() ? Identifier() : Identifier(fontId);

	if (fileName.endsWith(".woff"))
	{
		throw String("Error loading font " + fileName + ": unsupported format. Use .TTF");
	}

	customTypeFaces.add(CustomTypeFace(juce::Typeface::createSystemTypefaceFor(fontData, fontDataSize), id_));

	MemoryBlock mb(fontData, fontDataSize);
	
	ValueTree v("Font");
	v.setProperty("Name", fileName, nullptr);
	v.setProperty("Data", var(mb), nullptr);
	v.setProperty("Size", var((int)mb.getSize()), nullptr);
	
	if (fontId.isNotEmpty())
		v.setProperty("FontId", fontId, nullptr);


	customTypeFaceData.addChild(v, -1, nullptr);
}

int MainController::getBufferSizeForCurrentBlock() const noexcept
{
	jassert(getKillStateHandler().getCurrentThread() == KillStateHandler::AudioThread);

	return numSamplesThisBlock;
}

ValueTree MainController::exportCustomFontsAsValueTree() const
{
	return customTypeFaceData;
}


juce::ValueTree MainController::exportAllMarkdownDocsAsValueTree() const
{
	ValueTree v("MarkdownDocs");

	auto r = getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts);

	Array<File> docFiles;

	r.findChildFiles(docFiles, File::findFiles, true, "*.md");

	for (const auto& f : docFiles)
	{
		if (f.getFileName().startsWith("."))
			continue;

		ValueTree doc("MarkdownContent");
		doc.setProperty("ID", "{PROJECT_FOLDER}" + f.getRelativePathFrom(r), nullptr);
		doc.setProperty("Content", f.loadFileAsString(), nullptr);

		v.addChild(doc, -1, nullptr);
	}

	return v;
}

void MainController::restoreEmbeddedMarkdownDocs(const ValueTree& v)
{
	if (v.isValid())
		embeddedMarkdownDocs = v;
}



juce::String MainController::getEmbeddedMarkdownContent(const String& url) const
{
	for (auto c : embeddedMarkdownDocs)
	{
		auto id = c.getProperty("ID").toString().replace("\\", "/");

		if (id == url)
			return c.getProperty("Content").toString();
	}

	return {};
}

void MainController::restoreCustomFontValueTree(const ValueTree &v)
{
	customTypeFaceData = v;

	for (int i = 0; i < customTypeFaceData.getNumChildren(); i++)
	{
		ValueTree child = customTypeFaceData.getChild(i);

		if (!child.isValid())
		{
			jassertfalse;
			return;
		}

		var c = child.getProperty("Data", var::undefined());

		if (!c.isBinaryData())
		{
			jassertfalse;
			return;
		}

		MemoryBlock *mb = c.getBinaryData();

		if (mb != nullptr)
		{
			auto fontId = child.getProperty("FontId", "").toString();

			if (fontId.isNotEmpty())
			{
				Identifier id_(fontId);
				customTypeFaces.add(CustomTypeFace(juce::Typeface::createSystemTypefaceFor(mb->getData(), mb->getSize()), id_));
			}
			else
			{
				customTypeFaces.add(CustomTypeFace(juce::Typeface::createSystemTypefaceFor(mb->getData(), mb->getSize()), Identifier()));
			}
		}
		else
		{
			jassertfalse;
		}
	}
}


bool MainController::isInitialised() const noexcept
{
	return getKillStateHandler().initialised();
}

void MainController::insertStringAtLastActiveEditor(const String &string, bool selectArguments)
{
#if USE_BACKEND
	if (lastActiveEditor.getComponent() != nullptr)
	{
		auto ed = dynamic_cast<PopupIncludeEditor::EditorType*>(lastActiveEditor.getComponent());

#if !HISE_USE_NEW_CODE_EDITOR

		ed->getDocument().deleteSection(ed->getSelectionStart(), ed->getSelectionEnd());
        ed->moveCaretTo(CodeDocument::Position(ed->getDocument(), lastCharacterPositionOfSelectedEditor), false);
		ed->insertTextAtCaret(string);

		if (selectArguments)
		{
			ed->moveCaretLeft(false, false);

			while (!ed->getTextInRange(ed->getHighlightedRegion()).contains("("))
				ed->moveCaretLeft(false, true);

			ed->moveCaretRight(false, true);
		}

#endif
		ed->grabKeyboardFocus();
	}
#endif
}

bool MainController::checkAndResetMidiInputFlag()
{
	const bool returnValue = midiInputFlag;
	midiInputFlag = false;

	return returnValue;
}

ReferenceCountedObject* MainController::getGlobalPreprocessor()
{
    if(preprocessor == nullptr)
    {
        auto pp = new HiseJavascriptPreprocessor();

#if USE_BACKEND
        
        auto& settings = dynamic_cast<GlobalSettingManager*>(this)->getSettingsObject();
        
        pp->setEnableGlobalPreprocessor(settings.getSetting(HiseSettings::Project::EnableGlobalPreprocessor));
        
        auto obj = settings.getExtraDefinitionsAsObject();

        for(const auto& p: obj.getDynamicObject()->getProperties())
        {
            auto key = p.name.toString();
            auto v = p.value.toString();
            
            snex::jit::ExternalPreprocessorDefinition def;
            
            def.t = snex::jit::ExternalPreprocessorDefinition::Type::Definition;
            def.name = key;
            def.value = v;
            def.fileName = "EXTERNAL_DEFINITION";
            
            pp->definitions.add(def);
        }
        
#endif
        
        preprocessor = pp;
    }
    
    return preprocessor.get();
}

float MainController::getGlobalCodeFontSize() const
{
	return jmax<float>(14.0f, (float)dynamic_cast<const GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Scripting::CodeFontSize));
}


void MainController::loadUserPresetAsync(const ValueTree& v)
{
	//getMainSynthChain()->killAllVoices();
	//presetLoadRampFlag.set(OldUserPresetHandler::FadeOut);
	userPresetHandler.loadUserPreset(v);
}

#if USE_BACKEND

void MainController::writeToConsole(const String &message, int warningLevel, const Processor *p, Colour c)
{
	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

	codeHandler.writeToConsole(message, warningLevel, p, c);
}

void MainController::setWatchedScriptProcessor(JavascriptProcessor *p, Component *editor)
{
	if (scriptWatchTable.getComponent() != nullptr)
	{
		scriptWatchTable->setHolder(p);
	}
};;

void MainController::setScriptWatchTable(ScriptWatchTable *table)
{
	scriptWatchTable = table;
}


#endif

void MainController::rebuildVoiceLimits()
{
	Processor::Iterator<ModulatorSynth> iter(getMainSynthChain());

	while (auto synth = iter.getNextProcessor())
	{
		synth->setVoiceLimit((int)synth->getAttribute(ModulatorSynth::VoiceLimit));
	}
}

void MainController::timerCallback()
{
	getControlUndoManager()->beginNewTransaction();
#if USE_BACKEND
	getScriptComponentEditBroadcaster()->getUndoManager().beginNewTransaction();
#endif
}

void MainController::handleSuspendedNoteOffs()
{
	if (!suspendedNoteOns.isEmpty())
	{
		for (int i = 0; i < suspendedNoteOns.size(); i++)
			masterEventBuffer.addEvent(suspendedNoteOns[i]);
		
		suspendedNoteOns.clearQuick();
	}
}

void MainController::updateMultiChannelBuffer(int numNewChannels)
{
    if(processingBufferSize.get() == -1)
        return;
    
	ScopedLock sl(processLock);

	// Updates the channel amount
	multiChannelBuffer.setSize(numNewChannels, processingBufferSize.get(), true, true, true);

	refreshOversampling();	
}

double MainController::SampleManager::getPreloadProgressConst() const
{
	return internalPreloadJob.progress;
}

void MainController::SampleManager::handleNonRealtimeState()
{
	if (isNonRealtime() != internalsSetToNonRealtime)
	{
		Processor::Iterator<NonRealtimeProcessor> iter(mc->getMainSynthChain());

		LockHelpers::SafeLock sl(mc, LockHelpers::AudioLock);

		

		while (auto nrt = iter.getNextProcessor())
			nrt->nonRealtimeModeChanged(isNonRealtime());

		internalsSetToNonRealtime = isNonRealtime();
	}
}




hise::MainController::UserPresetHandler::CustomAutomationData::Ptr MainController::UserPresetHandler::getCustomAutomationData(int index) const
{
	if (auto p = customAutomationData[index])
	{
		jassert(p->index == index);
		return p;
	}

	return nullptr;
}

void removePropertyRecursive(NamedValueSet& removedProperties, String currentPath, ValueTree v, const Identifier& id)
{
	if (!currentPath.isEmpty())
		currentPath << ":";

	currentPath << v.getType();

	if (v.hasProperty(id))
	{
		auto value = v.getProperty(id);
		v.removeProperty(id, nullptr);
		removedProperties.set(Identifier(currentPath + ":" + id.toString()), value);
	}
	
	for (auto c : v)
		removePropertyRecursive(removedProperties, currentPath, c, id);
}

MainController::UserPresetHandler::StoredModuleData::StoredModuleData(var moduleId, Processor* pToRestore) :
	p(pToRestore)
{
	if (moduleId.isString())
		id = moduleId.toString();
	else
	{
		id = moduleId["ID"].toString();

		auto rp = moduleId["RemovedProperties"];
		auto rc = moduleId["RemovedChildElements"];

		if (rp.isArray() || rc.isArray())
		{
			auto v = p->exportAsValueTree();

			if (rp.isArray())
			{
				for (auto propertyToRemove : *rp.getArray())
				{
					auto pid_ = propertyToRemove.toString();
					if (pid_.isNotEmpty())
					{
						Identifier pid(pid_);
						removePropertyRecursive(removedProperties, {}, v, pid);
					}
				}
			}
			
			if (rc.isArray())
			{
				for (auto childToRemove : *rc.getArray())
				{
					auto pid_ = childToRemove.toString();

					if (pid_.isNotEmpty())
					{
						Identifier pid(pid_);
						removedChildElements.add(v.getChildWithName(pid).createCopy());
					}
				}
			}

			removedProperties.remove(Identifier("Processor:ID"));
		}
	}
}

void restorePropertiesRecursive(ValueTree v, StringArray path, const var& value, bool restore)
{
	if (path.size() == 2)
	{
		if (Identifier(path[0]) == v.getType())
		{
			auto id = Identifier(path[1]);

			if (restore)
				v.setProperty(id, value, nullptr);
			else
				v.removeProperty(id, nullptr);
		}
	}
	else
	{
		path.remove(0);

		for (auto c : v)
			restorePropertiesRecursive(c, path, value, restore);
	}
}

void MainController::UserPresetHandler::StoredModuleData::stripValueTree(ValueTree& v)
{
	for (const auto& rp : removedProperties)
	{
		auto path = StringArray::fromTokens(rp.name.toString(), ":", "\"");
		
		restorePropertiesRecursive(v, path, {}, false);
	}
		

	for (const auto& rc : removedChildElements)
	{
		auto cToRemove = v.getChildWithName(rc.getType());

		if (cToRemove.isValid())
			v.removeChild(cToRemove, nullptr);
	}
}



void MainController::UserPresetHandler::StoredModuleData::restoreValueTree(ValueTree& v)
{
	stripValueTree(v);

	for (const auto& rp : removedProperties)
	{
		auto path = StringArray::fromTokens(rp.name.toString(), ":", "\"");
		auto value = rp.value;
		restorePropertiesRecursive(v, path, value, true);
	}
		

	for (const auto& rc : removedChildElements)
		v.addChild(rc.createCopy(), -1, nullptr);
}



} // namespace hise
