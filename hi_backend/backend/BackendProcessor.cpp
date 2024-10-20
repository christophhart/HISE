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

	BackendProcessor::BackendProcessor(AudioDeviceManager *deviceManager_/*=nullptr*/, AudioProcessorPlayer *callback_/*=nullptr*/) :
MainController(),
AudioProcessorDriver(deviceManager_, callback_),
scriptUnlocker(this)
{
	//printData();
    
	ExtendedApiDocumentation::init();

    synthChain = new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES);
    
	synthChain->addProcessorsWhenEmpty();

	getSampleManager().getModulatorSamplerSoundPool()->setDebugProcessor(synthChain);
	getMacroManager().setMacroChain(synthChain);

	getExpansionHandler().addListener(this);

	if (!inUnitTestMode())
	{
		handleEditorData(false);
		restoreGlobalSettings(this);
	}

	GET_PROJECT_HANDLER(synthChain).restoreWorkingProjects();

	initData(this);

	getFontSizeChangeBroadcaster().sendMessage(sendNotification, getGlobalCodeFontSize());

	GET_PROJECT_HANDLER(synthChain).checkSubDirectories();

	dllManager = new BackendDllManager(this);

	if(getCurrentFileHandler().getRootFolder().isDirectory())
		refreshExpansionType();

	//getExpansionHandler().createAvailableExpansions();


	if (!inUnitTestMode())
	{
		getAutoSaver().updateAutosaving();
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
    
}


BackendProcessor::~BackendProcessor()
{
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

	handleEditorData(true);
}



void BackendProcessor::projectChanged(const File& /*newRootDirectory*/)
{
	getExpansionHandler().setCurrentExpansion("");
	
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
    TRACE_DSP();

	if(externalClockSim.bypassed)
	{
		processBlockBypassed(buffer, midiMessages);
		return;
	}

	
	
#if !HISE_BACKEND_AS_FX
	buffer.clear();
#endif

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

		getDelayedRenderer().processWrapped(buffer, midiMessages);
		
#if IS_STANDALONE_APP
		externalClockSim.addPostTimelineData(buffer, midiMessages);
#endif
	}

#if IS_STANDALONE_APP
    externalClockSim.process(buffer.getNumSamples());
#endif
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
};

void BackendProcessor::getStateInformation(MemoryBlock &destData)
{
	MemoryOutputStream output(destData, false);

	ValueTree v = synthChain->exportAsValueTree();

	v.setProperty("ProjectRootFolder", GET_PROJECT_HANDLER(synthChain).getWorkDirectory().getFullPathName(), nullptr);

	if (auto root = dynamic_cast<BackendRootWindow*>(getActiveEditor()))
	{
		root->saveInterfaceData();
	}

	v.setProperty("InterfaceData", JSON::toString(editorInformation, true, DOUBLE_TO_STRING_DIGITS), nullptr);

	v.writeToStream(output);
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

hise::JavascriptProcessor* BackendProcessor::createInterface(int width, int height)
{
	auto midiChain = dynamic_cast<MidiProcessorChain*>(getMainSynthChain()->getChildProcessor(ModulatorSynthChain::MidiProcessor));
	auto s = getMainSynthChain()->getMainController()->createProcessor(midiChain->getFactoryType(), "ScriptProcessor", "Interface");
	auto jsp = dynamic_cast<JavascriptProcessor*>(s);

	String code = "Content.makeFrontInterface(" + String(width) + ", " + String(width) + ");";

	jsp->getSnippet(0)->replaceContentAsync(code);
	jsp->compileScript();

	midiChain->getHandler()->add(s, nullptr);

	midiChain->setEditorState(Processor::EditorState::Visible, true);
	s->setEditorState(Processor::EditorState::Folded, true);

	return jsp;
}

void BackendProcessor::setEditorData(var editorState)
{
	editorInformation = editorState;
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
				for(int i = 0; i < 127; i++)
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


