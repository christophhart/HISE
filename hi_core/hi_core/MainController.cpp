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

#if  JUCE_MAC

struct FileLimitInitialiser
{
    FileLimitInitialiser()
    {
        rlimit lim;
        
        getrlimit (RLIMIT_NOFILE, &lim);
        lim.rlim_cur = lim.rlim_max = 200000;
        setrlimit (RLIMIT_NOFILE, &lim);
    }
};



static FileLimitInitialiser fileLimitInitialiser;
#endif

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
#if USE_BACKEND
	popupConsole(nullptr),
	usePopupConsole(false),
#endif
	shownComponents(0),
	plotter(nullptr),
	usagePercent(0),
	scriptWatchTable(nullptr)
{
	
#if USE_BACKEND

	shownComponents.setBit(BackendProcessorEditor::Keyboard, 1);
	shownComponents.setBit(BackendProcessorEditor::Macros, 0);

#endif

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


MainController::MacroManager::MacroManager():
	macroIndexForCurrentLearnMode(-1),
	macroChain(nullptr)
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

PresetLoadingThread::PresetLoadingThread(MainController *mc, const ValueTree v_) :
ThreadWithAsyncProgressWindow("Loading Preset " + v_.getProperty("ID").toString()),
v(v_),
mc(mc),
fileNeedsToBeParsed(false)
{
	addBasicComponents(false);
}

PresetLoadingThread::PresetLoadingThread(MainController *mc, const File &presetFile):
ThreadWithAsyncProgressWindow("Loading Preset " + presetFile.getFileName()),
file(presetFile),
fileNeedsToBeParsed(true),
mc(mc)
{
    
    
	addBasicComponents(false);
}

void PresetLoadingThread::run()
{
	if (fileNeedsToBeParsed)
	{
		setProgress(0.0);
		showStatusMessage("Parsing preset file");
		FileInputStream fis(file);

		v = ValueTree::readFromStream(fis);

		if (v.isValid() && v.getProperty("Type", var::undefined()).toString() == "SynthChain")
		{
			if (v.getType() != Identifier("Processor"))
			{
				v = PresetHandler::changeFileStructureToNewFormat(v);
			}
		}

        const int presetVersion = v.getProperty("BuildVersion", 0);
        
        if(presetVersion > BUILD_SUB_VERSION)
        {
            PresetHandler::showMessageWindow("Version mismatch", "The preset was built with a newer the build of HISE: " + String(presetVersion) + ". To ensure perfect compatibility, update to at least this build.");
        }
        
		setProgress(0.5);
		if (threadShouldExit())
		{
			mc->clearPreset();
			return;
		}
		
	}

	ModulatorSynthChain *synthChain = mc->getMainSynthChain();

	

	sampleRate = synthChain->getSampleRate();
	bufferSize = synthChain->getBlockSize();

	synthChain->setBypassed(true);

	// Reset the sample rate so that prepareToPlay does not get called in restoreFromValueTree
	synthChain->setCurrentPlaybackSampleRate(-1.0);

	synthChain->setId(v.getProperty("ID", "MainSynthChain"));

	if (threadShouldExit()) return;

	showStatusMessage("Loading modules");

	synthChain->restoreFromValueTree(v);

	if (threadShouldExit()) return;
	
}

void PresetLoadingThread::threadFinished()
{
	ModulatorSynthChain *synthChain = mc->getMainSynthChain();

	ScopedLock sl(synthChain->getLock());

	synthChain->prepareToPlay(sampleRate, bufferSize);
	synthChain->compileAllScripts();

	mc->getSampleManager().getAudioSampleBufferPool()->clearData();

	synthChain->setBypassed(false);
    
	Processor::Iterator<ModulatorSampler> iter(synthChain, false);

	int i = 0;

	while (ModulatorSampler *sampler = iter.getNextProcessor())
	{
		showStatusMessage("Loading samples from " + sampler->getId());
		setProgress((double)i / (double)iter.getNumProcessors());
		sampler->refreshPreloadSizes();
		sampler->refreshMemoryUsage();
		if (threadShouldExit())
		{
			mc->clearPreset();
			return;
		}
	}
}

void GlobalScriptCompileBroadcaster::fillExternalFileList(Array<File> &files, StringArray &processors)
{
	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<ScriptProcessor> iter(mainChain);

	while (ScriptProcessor *sp = iter.getNextProcessor())
	{
		for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			if (!files.contains(sp->getWatchedFile(i)))
			{
				files.add(sp->getWatchedFile(i));
				processors.add(sp->getId());
			}

			
			
		}
	}
}

void GlobalScriptCompileBroadcaster::setExternalScriptData(ValueTree &collectedExternalScripts)
{
	externalScripts = collectedExternalScripts;
}

String GlobalScriptCompileBroadcaster::getExternalScriptFromCollection(const String &fileName)
{


	for (int i = 0; i < externalScripts.getNumChildren(); i++)
	{
		const String thisName = externalScripts.getChild(i).getProperty("FileName").toString();

		if (thisName == fileName)
		{
			return externalScripts.getChild(i).getProperty("Content").toString();
		}
	}

	// Hitting this assert means you try to get a script that wasn't exported.
	jassertfalse;
	return String::empty;
}

void CustomKeyboardState::setLowestKeyToDisplay(int lowestKeyToDisplay)
{
	lowestKey = lowestKeyToDisplay;
}
