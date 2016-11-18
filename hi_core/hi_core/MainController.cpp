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

MainController::MainController():
	sampleManager(new SampleManager(this)),
	allNotesOffFlag(false),
	bufferSize(-1),
	sampleRate(-1.0),
	temp_usage(0.0f),
	uptime(0.0),
	bpm(120.0),
	console(nullptr),
	voiceAmount(0),
	scrollY(0),
	mainLookAndFeel(new KnobLookAndFeel()),
	mainCommandManager(new ApplicationCommandManager()),
	shownComponents(0),
	plotter(nullptr),
	usagePercent(0),
	scriptWatchTable(nullptr),
    globalPitchFactor(1.0),
    midiInputFlag(false),
	macroManager(this),
	autoSaver(this),
	enablePluginParameterUpdate(true),
	customTypeFaceData(ValueTree("CustomFonts")),
	masterEventBuffer(),
	eventIdHandler(masterEventBuffer),
#if JUCE_WINDOWS
    globalCodeFontSize(14.0f)
#else
    globalCodeFontSize(13.0f)
#endif
{
	BACKEND_ONLY(popupConsole = nullptr);
	BACKEND_ONLY(usePopupConsole = false);

	BACKEND_ONLY(shownComponents.setBit(BackendProcessorEditor::Keyboard, 1));
	BACKEND_ONLY(shownComponents.setBit(BackendProcessorEditor::Macros, 0));

	TempoSyncer::initTempoData();
    
	globalVariableArray.insertMultiple(0, var::undefined(), NUM_GLOBAL_VARIABLES);
	globalVariableObject = new DynamicObject();

	toolbarProperties = DefaultFrontendBar::createDefaultProperties();

	hostInfo = new DynamicObject();
    
#if HI_RUN_UNIT_TESTS

	UnitTestRunner runner;

	runner.setAssertOnFailure(false);

	runner.runAllTests();

	

#endif
};


MainController::~MainController()
{
	Logger::setCurrentLogger(nullptr);
	logger = nullptr;
	masterReference.clear();
	customTypeFaces.clear();
	userPresetData = nullptr;
}


MainController::SampleManager::SampleManager(MainController *mc):
	samplerLoaderThreadPool(new SampleThreadPool()),
	projectHandler(),
	globalSamplerSoundPool(new ModulatorSamplerSoundPool(mc)),
	globalAudioSampleBufferPool(new AudioSampleBufferPool(&projectHandler)),
	globalImagePool(new ImagePool(&projectHandler)),
	sampleClipboard(ValueTree("clipboard")),
	useRelativePathsToProjectFolder(true)
{
	
}


MainController::MacroManager::MacroManager(MainController *mc_) :
	macroIndexForCurrentLearnMode(-1),
	macroChain(nullptr),
	mc(mc_),
	midiControllerHandler(mc_)
{
	for(int i = 0; i < 8; i++)
	{
		macroControllerNumbers[i] = -1;
	};
}

#if USE_BACKEND
void MainController::writeToConsole(const String &message, int warningLevel, const Processor *p, Colour c)
{
	CHECK_KEY(this);

	Console *currentConsole = usePopupConsole ? popupConsole.get() : console.get();

	if (currentConsole != nullptr) currentConsole->logMessage(message, (Console::WarningLevel)warningLevel, p, (p != nullptr && c.isTransparent()) ? p->getColour() : c);


}
#endif

void MainController::increaseVoiceCounter()
{
	voiceAmount.set(voiceAmount.get() + 1);
}

void MainController::decreaseVoiceCounter()
{
	voiceAmount.set(voiceAmount.get() - 1);
	if (voiceAmount.get() < 0) voiceAmount.set(0);
}

void MainController::resetVoiceCounter()
{
	voiceAmount.set(0);
}

const CriticalSection & MainController::getLock() const
{
	return dynamic_cast<AudioProcessor*>(const_cast<MainController*>(this))->getCallbackLock();
}


void MainController::loadPreset(const File &f, Component *mainEditor)
{
	clearPreset();

	PresetLoadingThread *presetLoader = new PresetLoadingThread(this, f);

	if (mainEditor != nullptr)
	{
		presetLoader->setModalBaseWindowComponent(mainEditor);
	}
	else
	{
		presetLoader->showOnDesktop();
	}

	presetLoader->runSynchronous();
}

void MainController::clearPreset()
{
    getMainSynthChain()->reset();

	globalVariableObject->clear();

	toolbarProperties = DefaultFrontendBar::createDefaultProperties();

	for (int i = 0; i < 127; i++)
	{
		setKeyboardCoulour(i, Colours::transparentBlack);
	}
    
    changed = false;
}

void MainController::loadPreset(ValueTree &v, Component* /*mainEditor*/)
{
	if (v.isValid() && v.getProperty("Type", var::undefined()).toString() == "SynthChain")
	{
		//ScopedLock sl(getLock());

		clearPreset();

		if (v.getType() != Identifier("Processor"))
		{
			v = PresetHandler::changeFileStructureToNewFormat(v);
		}

		ModulatorSynthChain *synthChain = getMainSynthChain();

		sampleRate = synthChain->getSampleRate();
		bufferSize = synthChain->getBlockSize();

		synthChain->setBypassed(true);

		// Reset the sample rate so that prepareToPlay does not get called in restoreFromValueTree
		synthChain->setCurrentPlaybackSampleRate(-1.0);
		synthChain->setId(v.getProperty("ID", "MainSynthChain"));

		skipCompilingAtPresetLoad = true;


		synthChain->restoreFromValueTree(v);

		skipCompilingAtPresetLoad = false;

		synthChain->prepareToPlay(sampleRate, bufferSize.get());
		synthChain->compileAllScripts();

        synthChain->loadMacrosFromValueTree(v);

		getSampleManager().getAudioSampleBufferPool()->clearData();

		synthChain->setBypassed(false);

        
        Processor::Iterator<ModulatorSynth> iter(synthChain, false);
        
        while(ModulatorSynth *synth = iter.getNextProcessor())
        {
            synth->setEditorState(Processor::EditorState::Folded, true);
        }
        
        changed = false;
        
	}
	else
	{
		PresetHandler::showMessageWindow("No valid container", "This preset is not a container file", PresetHandler::IconType::Error);
	}
}


void MainController::startCpuBenchmark(int bufferSize_)
{
	bufferSize.set(bufferSize_);
	temp_usage.set(Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()));
}

void MainController::compileAllScripts()
{
	Processor::Iterator<JavascriptProcessor> it(getMainSynthChain());

	JavascriptProcessor *sp;
		
	while((sp = it.getNextProcessor()) != nullptr)
	{
		sp->compileScript();
	}
};

void MainController::stopCpuBenchmark()
{
	const double usage = (Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()) - temp_usage.get()) * sampleRate / bufferSize.get();
	usagePercent.set((int) (usage * 100));

}

void MainController::clearConsole()
{
#if USE_BACKEND
    if (usePopupConsole)
    {
        popupConsole->clear();
    }
    else
    {
        console->clear();
    }
#endif
    
}


void MainController::showConsole(bool consoleShouldBeShown)
{
#if USE_BACKEND
	if (console.get() != nullptr)
	{
		console->showComponentInDebugArea(consoleShouldBeShown);
	}
#else
	ignoreUnused(consoleShouldBeShown);
#endif
}

void MainController::replaceReferencesToGlobalFolder()
{
	ModulatorSynthChain *root = getMainSynthChain();

	Processor::Iterator<ExternalFileProcessor> it(root);

	ExternalFileProcessor *p = nullptr;

	while ((p = it.getNextProcessor()) != nullptr)
	{
		p->replaceReferencesWithGlobalFolder();
	}
}

void MainController::beginParameterChangeGesture(int index)			{ dynamic_cast<PluginParameterAudioProcessor*>(this)->beginParameterChangeGesture(index); }

void MainController::endParameterChangeGesture(int index)			{ dynamic_cast<PluginParameterAudioProcessor*>(this)->endParameterChangeGesture(index); }

void MainController::setPluginParameter(int index, float newValue)  { dynamic_cast<PluginParameterAudioProcessor*>(this)->setParameterNotifyingHost(index, newValue); }

void MainController::SampleManager::copySamplesToClipboard(const Array<WeakReference<ModulatorSamplerSound>> &soundsToCopy)
{
	sampleClipboard.removeAllChildren(nullptr);

	for(int i = 0; i < soundsToCopy.size(); i++)
	{
		if(soundsToCopy[i].get() != nullptr)
		{
            ValueTree soundTree = soundsToCopy[i]->exportAsValueTree();
            
            static Identifier duplicate("Duplicate");
            soundTree.setProperty(duplicate, true, nullptr);
            
			sampleClipboard.addChild(soundTree, -1, nullptr);
		}
	}
}

const ValueTree &MainController::SampleManager::getSamplesFromClipboard() const { return sampleClipboard;}

void MainController::SampleManager::saveAllSamplesToGlobalFolder(const String &packageName)
{
	StringArray filesInAudioPool =  getAudioSampleBufferPool()->getFileNameList();
	SampleMapExporter audioFileExporter(filesInAudioPool, true, PresetPlayerHandler::AudioFiles);
	audioFileExporter.exportSamples(PresetPlayerHandler::getSpecialFolder(PresetPlayerHandler::AudioFiles, packageName), packageName, true);
	
	StringArray filesInSamplerPool = getModulatorSamplerSoundPool()->getFileNameList();
	SampleMapExporter sampleExporter(filesInSamplerPool, true, PresetPlayerHandler::StreamedSampleFolder);
	sampleExporter.exportSamples(PresetPlayerHandler::getSpecialFolder(PresetPlayerHandler::StreamedSampleFolder, packageName), packageName, true);

	StringArray filesInImagePool = getImagePool()->getFileNameList();
	SampleMapExporter imageExporter(filesInImagePool, true, PresetPlayerHandler::ImageResources);
	imageExporter.exportSamples(PresetPlayerHandler::getSpecialFolder(PresetPlayerHandler::ImageResources, packageName), packageName, true);

}

const ValueTree MainController::SampleManager::getLoadedSampleMap(const String &fileName) const
{
	for (int i = 0; i < sampleMaps.getNumChildren(); i++)
	{
		String childFileName = sampleMaps.getChild(i).getProperty("SampleMapIdentifier", String::empty);
		if (childFileName == fileName) return sampleMaps.getChild(i);
	}

	return ValueTree::invalid;
}

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

void MainController::MacroManager::removeMacroControlsFor(Processor *p)
{
	if(p == macroChain) return; // it will delete itself

	if(macroChain == nullptr) return;

	for(int i = 0; i < 8; i++)
	{
		macroChain->getMacroControlData(i)->removeAllParametersWithProcessor(p);
	}

	macroChain->sendSynchronousChangeMessage();
}

void MainController::MacroManager::removeMacroControlsFor(Processor *p, Identifier name)
{
	if(p == macroChain) return; // it will delete itself

	if(macroChain == nullptr) return;

	for(int i = 0; i < 8; i++)
	{
		MacroControlBroadcaster::MacroControlData *data = macroChain->getMacroControlData(i);

		for(int j = 0; j < data->getNumParameters(); j++)
		{
			if(data->getParameter(j)->getParameterName() == name.toString() && data->getParameter(j)->getProcessor() == p)
			{
				data->removeParameter(j);
				macroChain->sendChangeMessage();
				return;
			}
		}
	}

	macroChain->sendSynchronousChangeMessage();
}

void MainController::MacroManager::setMidiControllerForMacro(int midiControllerNumber)
{
	if(macroIndexForCurrentMidiLearnMode >= 0 && macroIndexForCurrentMidiLearnMode < 8)
	{
		macroControllerNumbers[macroIndexForCurrentMidiLearnMode] = midiControllerNumber;
			
		getMacroChain()->getMacroControlData(macroIndexForCurrentMidiLearnMode)->setMidiController(midiControllerNumber);

		macroIndexForCurrentMidiLearnMode = -1;
	}
}

void MainController::MacroManager::setMidiControllerForMacro(int macroIndex, int midiControllerNumber)
{
	if (macroIndex < 8)
	{
		macroControllerNumbers[macroIndex] = midiControllerNumber;
	}
}

bool MainController::MacroManager::midiMacroControlActive() const
{
	for (int i = 0; i < 8; i++)
	{
		if (macroControllerNumbers[i] != -1) return true;
	}

	return false;
}

void MainController::MacroManager::setMacroControlMidiLearnMode(ModulatorSynthChain *chain, int index)
{
	macroChain = chain;
	macroIndexForCurrentMidiLearnMode = index;
}

int MainController::MacroManager::getMacroControlForMidiController(int midiController)
{
	for (int i = 0; i < 8; i++)
	{
		if (macroControllerNumbers[i] == midiController) return i;
	}

	return -1;
}

int MainController::MacroManager::getMidiControllerForMacro(int macroIndex)
{
	if (macroIndex < 8)
	{
		return macroControllerNumbers[macroIndex];
	}
	else
	{
		return -1;
	}
}

bool MainController::MacroManager::midiControlActiveForMacro(int macroIndex) const
{
	if (macroIndex < 8)
	{
		return macroControllerNumbers[macroIndex] != -1;
	}
	else
	{
		jassertfalse;
		return false;
	}
}

void MainController::MacroManager::removeMidiController(int macroIndex)
{
	if (macroIndex < 8)
	{
		macroControllerNumbers[macroIndex] = -1;
	}
}

void MainController::MacroManager::setMacroControlLearnMode(ModulatorSynthChain *chain, int index)
{
	macroChain = chain;
	macroIndexForCurrentLearnMode = index;
}

int MainController::MacroManager::getMacroControlLearnMode() const
{
	return macroIndexForCurrentLearnMode;
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
}

void MainController::addPlottedModulator(Modulator *m)
{
	if(plotter.getComponent() != nullptr)
	{
		plotter->addPlottedModulator(m);
	}
};

void MainController::removePlottedModulator(Modulator *m)
{
	if(plotter.getComponent() != nullptr)
	{
		plotter->removePlottedModulator(m);
	}
};

#if USE_BACKEND
void MainController::setWatchedScriptProcessor(JavascriptProcessor *p, Component *editor)
{
	if(scriptWatchTable.getComponent() != nullptr)
	{
		scriptWatchTable->setScriptProcessor(p, dynamic_cast<ScriptingEditor*>(editor));
	}
};


void MainController::setEditedScriptComponent(ReferenceCountedObject* c, Component *listener)
{


	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(c);
	

	if(scriptComponentEditPanel.getComponent() != nullptr)
	{
		scriptComponentEditPanel->setEditedComponent(sc);

		ScriptComponentEditListener *l = dynamic_cast<ScriptComponentEditListener *>(listener);

		if(l != nullptr)
		{
			scriptComponentEditPanel->removeAllListeners();
			scriptComponentEditPanel->addListener(l);
		}

		ScriptingEditor *editor = dynamic_cast<ScriptingEditor *>(listener);

		if(editor != nullptr)
		{
			editor->setEditedScriptComponent(sc);
		}
	}
	



};
#endif


void MainController::setPlotter(Plotter *p)
{
	plotter = p;
};

void MainController::skin(Component &c)
{
    c.setLookAndFeel(mainLookAndFeel);
    
    c.setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colour(0x66333333));
    c.setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colour(0xfb111111));
    c.setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::white.withAlpha(0.3f));
    
    if(dynamic_cast<Slider*>(&c) != nullptr) dynamic_cast<Slider*>(&c)->setScrollWheelEnabled(false);
};



void MainController::setCurrentViewChanged()
{
#if USE_BACKEND
	if(getMainSynthChain() != nullptr)
	{
		getMainSynthChain()->setCurrentViewChanged();
	}
#endif
}

void MainController::setGlobalVariable(int index, var newVariable)
{
	if (index >= NUM_GLOBAL_VARIABLES || index < 0)
	{
		jassertfalse;
		return;
	}

	ScopedLock sl(getLock());
	globalVariableArray.setUnchecked(index, newVariable.clone());
}

var MainController::getGlobalVariable(int index) const
{
	if (index >= NUM_GLOBAL_VARIABLES || index < 0)
	{
		jassertfalse;
		return var::undefined();
	}

	ScopedLock sl(getLock());
	return globalVariableArray.getUnchecked(index);
}

void MainController::storePlayheadIntoDynamicObject(AudioPlayHead::CurrentPositionInfo &lastPosInfo)
{
	static Identifier bpm("bpm");
	static Identifier timeSigNumerator("timeSigNumerator");
	static Identifier timeSigDenominator("timeSigDenominator");
	static Identifier timeInSamples("timeInSamples");
	static Identifier timeInSeconds("timeInSeconds");
	static Identifier editOriginTime("editOriginTime");
	static Identifier ppqPosition("ppqPosition");
	static Identifier ppqPositionOfLastBarStart("ppqPositionOfLastBarStart");
	static Identifier frameRate("frameRate");
	static Identifier isPlaying("isPlaying");
	static Identifier isRecording("isRecording");
	static Identifier ppqLoopStart("ppqLoopStart");
	static Identifier ppqLoopEnd("ppqLoopEnd");
	static Identifier isLooping("isLooping");

	ScopedLock sl(getLock());

	hostInfo->setProperty(bpm, lastPosInfo.bpm);
	hostInfo->setProperty(timeSigNumerator, lastPosInfo.timeSigNumerator);
	hostInfo->setProperty(timeSigDenominator, lastPosInfo.timeSigDenominator);
	hostInfo->setProperty(timeInSamples, lastPosInfo.timeInSamples);
	hostInfo->setProperty(timeInSeconds, lastPosInfo.timeInSeconds);
	hostInfo->setProperty(editOriginTime, lastPosInfo.editOriginTime);
	hostInfo->setProperty(ppqPosition, lastPosInfo.ppqPosition);
	hostInfo->setProperty(ppqPositionOfLastBarStart, lastPosInfo.ppqPositionOfLastBarStart);
	hostInfo->setProperty(frameRate, lastPosInfo.frameRate);
	hostInfo->setProperty(isPlaying, lastPosInfo.isPlaying);
	hostInfo->setProperty(isRecording, lastPosInfo.isRecording);
	hostInfo->setProperty(ppqLoopStart, lastPosInfo.ppqLoopStart);
	hostInfo->setProperty(ppqLoopEnd, lastPosInfo.ppqLoopEnd);
	hostInfo->setProperty(isLooping, lastPosInfo.isLooping);
}

#if USE_BACKEND
void MainController::setScriptWatchTable(ScriptWatchTable *table)
{
	scriptWatchTable = table;
}

void MainController::setScriptComponentEditPanel(ScriptComponentEditPanel *panel)
{
	scriptComponentEditPanel = panel;
}
#endif

void MainController::processBlockCommon(AudioSampleBuffer &buffer, MidiBuffer &midiMessages)
{
    ADD_GLITCH_DETECTOR("MainRoutine");
    
	ScopedNoDenormals snd;
    
	AudioProcessor *thisAsProcessor = dynamic_cast<AudioProcessor*>(this);

	ModulatorSynthChain *synthChain = getMainSynthChain();

	if (buffer.getNumSamples() != bufferSize.get())
	{
		debugError(synthChain, "Block size mismatch (old: " + String(bufferSize.get()) + ", new: " + String(buffer.getNumSamples()));
		prepareToPlay(sampleRate, buffer.getNumSamples());
	}

#if !FRONTEND_IS_PLUGIN
    
    
    
	keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);


	if (!midiMessages.isEmpty()) setMidiInputFlag();

	getMacroManager().getMidiControlAutomationHandler()->handleParameterData(midiMessages); // TODO_BUFFER: Move this after the next line...

	masterEventBuffer.addEvents(midiMessages);

	eventIdHandler.handleEventIds();
#endif

#if ENABLE_HOST_INFO
	AudioPlayHead::CurrentPositionInfo newTime;

	if (thisAsProcessor->getPlayHead() != nullptr && thisAsProcessor->getPlayHead()->getCurrentPosition(newTime))
	{
		lastPosInfo = newTime;
	}
	else lastPosInfo.resetToDefault();

	storePlayheadIntoDynamicObject(lastPosInfo);
	setBpm(lastPosInfo.bpm);
#endif

#if ENABLE_CPU_MEASUREMENT
	startCpuBenchmark(buffer.getNumSamples());
#endif

#if !FRONTEND_IS_PLUGIN
	buffer.clear();

	checkAllNotesOff();

#endif


#if USE_MIDI_CONTROLLERS_FOR_MACROS
	handleControllersForMacroKnobs(midiMessages);
#endif

	
#if FRONTEND_IS_PLUGIN

	synthChain->renderNextBlockWithModulators(buffer, masterEventBuffer);

#else
	multiChannelBuffer.clear();

	synthChain->renderNextBlockWithModulators(multiChannelBuffer, masterEventBuffer);

	FloatVectorOperations::copy(buffer.getWritePointer(0), multiChannelBuffer.getReadPointer(0), buffer.getNumSamples());
	FloatVectorOperations::copy(buffer.getWritePointer(1), multiChannelBuffer.getReadPointer(1), buffer.getNumSamples());

#if USE_HARD_CLIPPER
	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		FloatVectorOperations::clip(buffer.getWritePointer(i, 0), buffer.getReadPointer(i, 0), -1.0f, 1.0f, buffer.getNumSamples());
	}
#endif



	midiMessages.clear();
#endif

#if ENABLE_CPU_MEASUREMENT
	stopCpuBenchmark();
#endif

	uptime += double(buffer.getNumSamples()) / sampleRate;
}

void MainController::prepareToPlay(double sampleRate_, int samplesPerBlock)
{
	bufferSize = samplesPerBlock;
	sampleRate = sampleRate_;
    
#if ENABLE_CONSOLE_OUTPUT
	if (logger == nullptr)
	{
		logger = new ConsoleLogger(getMainSynthChain());
		Logger::setCurrentLogger(logger);
	}
#if USE_GLITCH_DETECTION
	ScopedGlitchDetector::setMaxTimeOutFromBufferSize(sampleRate_, (double)samplesPerBlock);
#endif
#endif
    
	multiChannelBuffer.setSize(getMainSynthChain()->getMatrix().getNumDestinationChannels(), samplesPerBlock);
    getMainSynthChain()->prepareToPlay(sampleRate, samplesPerBlock);
}

void MainController::setBpm(double bpm_)
{
	if(bpm.get() != bpm_)
	{
		bpm.set(bpm_);	

		for(int i = 0; i < tempoListeners.size(); i++)
		{
			if(tempoListeners[i].get() != nullptr)
			{
				tempoListeners[i].get()->tempoChanged(bpm.get());
			}
			else
			{
				// delete it with removeTempoListener!
				jassertfalse;
			}
		}
	}
};

void MainController::addTempoListener(TempoListener *t)
{
	ScopedLock sl(getLock());
	tempoListeners.addIfNotAlreadyThere(t);
}

void MainController::removeTempoListener(TempoListener *t)
{
	ScopedLock sl(getLock());
	tempoListeners.removeAllInstancesOf(t);
}

juce::Typeface* MainController::getFont(const String &fontName) const
{
	for (int i = 0; i < customTypeFaces.size(); i++)
	{
		if (customTypeFaces[i]->getName() == fontName)
		{
			return customTypeFaces[i].get();
		}
	}

	return nullptr;
}

void MainController::fillWithCustomFonts(StringArray &fontList)
{
	for (int i = 0; i < customTypeFaces.size(); i++)
	{
		fontList.addIfNotAlreadyThere(customTypeFaces[i]->getName());
	}
}

void MainController::loadTypeFace(const String& fileName, const void* fontData, size_t fontDataSize)
{
	if (customTypeFaceData.getChildWithProperty("Name", fileName).isValid()) return;

	customTypeFaces.add(juce::Typeface::createSystemTypefaceFor(fontData, fontDataSize));

	MemoryBlock mb(fontData, fontDataSize);
	
	ValueTree v("Font");
	v.setProperty("Name", fileName, nullptr);
	v.setProperty("Data", var(mb), nullptr);
	v.setProperty("Size", var((int)mb.getSize()), nullptr);

	customTypeFaceData.addChild(v, -1, nullptr);
}

ValueTree MainController::exportCustomFontsAsValueTree() const
{
	return customTypeFaceData;
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
			customTypeFaces.add(juce::Typeface::createSystemTypefaceFor(mb->getData(), mb->getSize()));
		}
		else
		{
			jassertfalse;
		}
	}
}

void MainController::insertStringAtLastActiveEditor(const String &string, bool selectArguments)
{
	if (lastActiveEditor.getComponent() != nullptr)
	{
		lastActiveEditor->getDocument().deleteSection(lastActiveEditor->getSelectionStart(), lastActiveEditor->getSelectionEnd());
        lastActiveEditor->moveCaretTo(CodeDocument::Position(lastActiveEditor->getDocument(), lastCharacterPositionOfSelectedEditor), false);

		lastActiveEditor->insertTextAtCaret(string);



		if (selectArguments)
		{
			lastActiveEditor->moveCaretLeft(false, false);

			while (!lastActiveEditor->getTextInRange(lastActiveEditor->getHighlightedRegion()).contains("("))
			{
				lastActiveEditor->moveCaretLeft(false, true);
			}

			lastActiveEditor->moveCaretRight(false, true);
		}

		lastActiveEditor->grabKeyboardFocus();
	}
}

bool MainController::checkAndResetMidiInputFlag()
{
	const bool returnValue = midiInputFlag;
	midiInputFlag = false;

	return returnValue;
}


ControlledObject::ControlledObject(MainController *m):	
	controller(m)	{jassert(m != nullptr);};

ControlledObject::~ControlledObject()	
{ 
	// Oops, this ControlledObject was not connected to a MainController
	jassert(controller != nullptr);

	masterReference.clear();
	
};
