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

	hostInfo = new DynamicObject();

};

MainController::SampleManager::SampleManager(MainController *mc):
	samplerLoaderThreadPool(new SampleThreadPool()),
	projectHandler(),
	globalSamplerSoundPool(new ModulatorSamplerSoundPool(mc)),
	globalAudioSampleBufferPool(new AudioSampleBufferPool(&projectHandler)),
	globalImagePool(new ImagePool(&projectHandler)),
	sampleClipboard(ValueTree("clipboard")),
	useRelativePathsToProjectFolder(true)
{
	samplerLoaderThreadPool->setThreadPriorities(10);
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
	ScopedLock sl(lock);

	++voiceAmount;


}

void MainController::decreaseVoiceCounter()
{
	ScopedLock sl(lock);

	--voiceAmount;
	 
	if(voiceAmount < 0) voiceAmount = 0;

}

void MainController::resetVoiceCounter()
{
	ScopedLock sl(lock);
 
	voiceAmount = 0;

}

const CriticalSection &MainController::getLock() const
{
#if STANDALONE_CONVOLUTION
    
    return dynamic_cast<const AudioProcessor*>(this)->getCallbackLock();
#else
    return const_cast<ModulatorSynthChain*>(getMainSynthChain())->getLock();
#endif
}


void MainController::loadPreset(const File &f, Component *mainEditor)
{
	clearPreset();

	PresetLoadingThread *presetLoader = new PresetLoadingThread(this, f);

	if (mainEditor != nullptr)
	{
		presetLoader->setModalComponentOfMainEditor(mainEditor);
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

	for (int i = 0; i < 127; i++)
	{
		setKeyboardCoulour(i, Colours::transparentBlack);
	}
}

void MainController::loadPreset(ValueTree &v, Component* /*mainEditor*/)
{
	if (v.isValid() && v.getProperty("Type", var::undefined()).toString() == "SynthChain")
	{
		//ScopedLock sl(lock);

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
		synthChain->restoreFromValueTree(v);
		synthChain->prepareToPlay(sampleRate, bufferSize);
		synthChain->compileAllScripts();
        synthChain->loadMacrosFromValueTree(v);

		getSampleManager().getAudioSampleBufferPool()->clearData();

		synthChain->setBypassed(false);

	}
	else
	{
		PresetHandler::showMessageWindow("No valid container", "This preset is not a container file");
	}
}

int MainController::addPluginParameter(PluginParameterModulator *p)		{ return dynamic_cast<PluginParameterAudioProcessor*>(this)->addPluginParameter(p); }
	
void MainController::removePluginParameter(PluginParameterModulator *p) { dynamic_cast<PluginParameterAudioProcessor*>(this)->removePluginParameter(p); }

void MainController::startCpuBenchmark(int bufferSize_)
{
	ScopedLock sl(lock);

	//if(console == nullptr) return;
	bufferSize = bufferSize_;
	temp_usage = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
}

void MainController::compileAllScripts()
{
	Processor::Iterator<ScriptProcessor> it(getMainSynthChain());

	ScriptProcessor *sp;
		
	while((sp = it.getNextProcessor()) != nullptr)
	{
		sp->compileScript();
	}
};

void MainController::stopCpuBenchmark()
{
	ScopedLock sl(lock);

	const double usage = (Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()) - temp_usage) * sampleRate / bufferSize;
	usagePercent = (int) (usage * 100);

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
void MainController::setWatchedScriptProcessor(ScriptProcessor *p, Component *editor)
{
	if(scriptWatchTable.getComponent() != nullptr)
	{
		scriptWatchTable->setScriptProcessor(p, dynamic_cast<ScriptingEditor*>(editor));
	}
};


void MainController::setEditedScriptComponent(DynamicObject *object, Component *listener)
{


	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(object);
	

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

	ScopedLock sl(lock);
	globalVariableArray.setUnchecked(index, newVariable.clone());
}

var MainController::getGlobalVariable(int index) const
{
	if (index >= NUM_GLOBAL_VARIABLES || index < 0)
	{
		jassertfalse;
		return var::undefined();
	}

	ScopedLock sl(lock);
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

	ScopedLock sl(lock);

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
	ScopedNoDenormals snd;

	AudioPlayHead::CurrentPositionInfo newTime;

	AudioProcessor *thisAsProcessor = dynamic_cast<AudioProcessor*>(this);

	ModulatorSynthChain *synthChain = getMainSynthChain();

	if (buffer.getNumSamples() != bufferSize)
	{
		debugError(synthChain, "Block size mismatch (old: " + String(bufferSize) + ", new: " + String(buffer.getNumSamples()));
		prepareToPlay(sampleRate, buffer.getNumSamples());
	}

	if (thisAsProcessor->getPlayHead() != nullptr && thisAsProcessor->getPlayHead()->getCurrentPosition(newTime))
	{
		lastPosInfo = newTime;
	}
	else
	{
		lastPosInfo.resetToDefault();
	};

	storePlayheadIntoDynamicObject(lastPosInfo);

	setBpm(lastPosInfo.bpm);

#if USE_HI_DEBUG_TOOLS 
	startCpuBenchmark(buffer.getNumSamples());
#endif

	if (thisAsProcessor->isSuspended())
	{
		buffer.clear();
	}

	checkAllNotesOff(midiMessages);

#if USE_MIDI_CONTROLLERS_FOR_MACROS

	handleControllersForMacroKnobs(midiMessages);

#endif

	keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

	if (!midiMessages.isEmpty()) setMidiInputFlag();

	multiChannelBuffer.clear();

	synthChain->renderNextBlockWithModulators(multiChannelBuffer, midiMessages);

	FloatVectorOperations::copy(buffer.getWritePointer(0), multiChannelBuffer.getReadPointer(0), buffer.getNumSamples());
	FloatVectorOperations::copy(buffer.getWritePointer(1), multiChannelBuffer.getReadPointer(1), buffer.getNumSamples());

	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		FloatVectorOperations::clip(buffer.getWritePointer(i, 0), buffer.getReadPointer(i, 0), -1.0f, 1.0f, buffer.getNumSamples());
	}

	midiMessages.clear();

#if USE_HI_DEBUG_TOOLS
	stopCpuBenchmark();
#endif

	uptime += double(buffer.getNumSamples()) / sampleRate;
}

void MainController::prepareToPlay(double sampleRate_, int samplesPerBlock)
{
	bufferSize = samplesPerBlock;
	sampleRate = sampleRate_;

	multiChannelBuffer.setSize(getMainSynthChain()->getMatrix().getNumDestinationChannels(), samplesPerBlock);
    
    getMainSynthChain()->prepareToPlay(sampleRate, samplesPerBlock);
}

void MainController::setBpm(double bpm_)
{
	if(bpm != bpm_)
	{
		ScopedLock sl(lock);

		bpm = bpm_;	

		for(int i = 0; i < tempoListeners.size(); i++)
		{
			if(tempoListeners[i].get() != nullptr)
			{
				tempoListeners[i].get()->tempoChanged(bpm);
			}
			else
			{
				// delete it with removeTempoListener!
				jassertfalse;
			}
		}
	}
};

ControlledObject::ControlledObject(MainController *m):	
	controller(m)	{jassert(m != nullptr);};

ControlledObject::~ControlledObject()	
{ 
	// Oops, this ControlledObject was not connected to a MainController
	jassert(controller != nullptr);

	masterReference.clear();
	
};


ValueTree MidiControllerAutomationHandler::exportAsValueTree() const
{
	ValueTree v("MidiAutomation");

	for (int i = 0; i < 128; i++)
	{
		const AutomationData *a = automationData + i;
		if (a->used)
		{
			ValueTree cc("Controller");

			cc.setProperty("Controller", i, nullptr);
			cc.setProperty("Processor", a->processor->getId(), nullptr);
			cc.setProperty("MacroIndex", a->macroIndex, nullptr);
			cc.setProperty("Start", a->parameterRange.start, nullptr);
			cc.setProperty("End", a->parameterRange.end, nullptr);
			cc.setProperty("Skew", a->parameterRange.skew, nullptr);
			cc.setProperty("Interval", a->parameterRange.interval, nullptr);
			cc.setProperty("Attribute", a->attribute, nullptr);

			v.addChild(cc, -1, nullptr);
		}
	}

	return v;
}

void MidiControllerAutomationHandler::restoreFromValueTree(const ValueTree &v)
{
	if (v.getType() != Identifier("MidiAutomation")) return;

	clear();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree cc = v.getChild(i);

		int controller = cc.getProperty("Controller", i);

		AutomationData *a = automationData + controller;

		a->processor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), cc.getProperty("Processor"));
		a->macroIndex = cc.getProperty("MacroIndex");
		a->attribute = cc.getProperty("Attribute", a->attribute);

		double start = cc.getProperty("Start");
		double end = cc.getProperty("End");
		double skew = cc.getProperty("Skew", a->parameterRange.skew);
		double interval = cc.getProperty("Interval", a->parameterRange.interval);

		a->parameterRange = NormalisableRange<double>(start, end, interval, skew);

		a->used = true;
	}

	refreshAnyUsedState();
}

void MidiControllerAutomationHandler::handleParameterData(MidiBuffer &b)
{
	const bool bufferEmpty = b.isEmpty();
	const bool noCCsUsed = !anyUsed && !unlearnedData.used;

	if (bufferEmpty || noCCsUsed) return;

	ScopedLock sl(lock);

	tempBuffer.clear();

	

	MidiBuffer::Iterator mb(b);

	MidiMessage m;

	int samplePos;

	while (mb.getNextEvent(m, samplePos))
	{
		bool consumed = false;

		if (m.isController())
		{
			const int number = m.getControllerNumber();

			if (isLearningActive())
			{
				setUnlearndedMidiControlNumber(number);
			}

			AutomationData *a = automationData + number;

			if (a->used)
			{
				jassert(a->processor.get() != nullptr);

				const float value = (float)a->parameterRange.convertFrom0to1((double)m.getControllerValue() / 127.0);

				if (a->macroIndex != -1)
				{
					a->processor->getMainController()->getMacroManager().getMacroChain()->setMacroControl(a->macroIndex, (double)m.getControllerValue(), sendNotification);
				}
				else
				{
					a->processor->setAttribute(a->attribute, value, sendNotification);
				}

				

				consumed = true;
			}
		}

		if (!consumed)
		{
			tempBuffer.addEvent(m, samplePos);
		}
	}

	b.clear();

	b.addEvents(tempBuffer, 0, -1, 0);
}