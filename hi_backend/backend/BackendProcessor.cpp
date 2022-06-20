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

namespace hise { using namespace juce;

BackendProcessor::BackendProcessor(AudioDeviceManager *deviceManager_/*=nullptr*/, AudioProcessorPlayer *callback_/*=nullptr*/) :
MainController(),
AudioProcessorDriver(deviceManager_, callback_),
viewUndoManager(new UndoManager()),
scriptUnlocker(this)
{
	ExtendedApiDocumentation::init();

    synthChain = new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES, viewUndoManager);
    
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

	refreshExpansionType();

	//getExpansionHandler().createAvailableExpansions();


	if (!inUnitTestMode())
	{
		getAutoSaver().updateAutosaving();
	}
	
	clearPreset();
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

		getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::SampleLoadingThread);
	}
}


BackendProcessor::~BackendProcessor()
{
	docWindow = nullptr;
	docProcessor = nullptr;
	getDatabase().clear();

#if JUCE_ENABLE_AUDIO_GUARD
	AudioThreadGuard::setHandler(nullptr);
#endif

	getSampleManager().cancelAllJobs();

	getSampleManager().getProjectHandler().removeListener(this);
	getExpansionHandler().removeListener(this);

	deletePendingFlag = true;

	clearPreset();

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

	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::SampleLoadingThread);

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

void BackendProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	getDelayedRenderer().processWrapped(buffer, midiMessages);
};

void BackendProcessor::processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
	midiMessages.clear();
	//allNotesOff();
}

void BackendProcessor::handleControllersForMacroKnobs(const MidiBuffer &/*midiMessages*/)
{
	
}


void BackendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
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

	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::SampleLoadingThread);
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

void BackendProcessor::registerItemGenerators()
{
	AutogeneratedDocHelpers::addItemGenerators(*this);
}

void BackendProcessor::registerContentProcessor(MarkdownContentProcessor* processor)
{
	AutogeneratedDocHelpers::registerContentProcessor(processor);
}

juce::File BackendProcessor::getCachedDocFolder() const
{
	return AutogeneratedDocHelpers::getCachedDocFolder();
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
    
	if (isFlakyThreadingAllowed())
		return this;

	if (docProcessor == nullptr)
	{
		docProcessor = new BackendProcessor(deviceManager, callback);
		docProcessor->setAllowFlakyThreading(true);
		docProcessor->prepareToPlay(44100.0, 512);
		
	}

	return docProcessor;
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



} // namespace hise


