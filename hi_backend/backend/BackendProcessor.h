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

#ifndef BACKEND_PROCESSOR_H_INCLUDED
#define BACKEND_PROCESSOR_H_INCLUDED

namespace hise { using namespace juce;



class BackendProcessor;
class InteractionTester;
class MidiInjector;

/** A base class that is used both by the HTTP wizards as well as the multipage dialog library.

	This encapsulates the read / writing to the state as well as error reporting / logging.

	In order to use that class, implement two static functions in your subclass that implement the logic
	for initialisation and execution, then pass in a BaseStateManager that reads / writes the values.

*/
struct BaseStateManager : public ControlledObject
{
	using InitFunction = std::function<void(BaseStateManager*)>;
	using Executor = std::function<void(BaseStateManager*)>;

	BaseStateManager(MainController* mc) :
		ControlledObject(mc)
	{}

	virtual ~BaseStateManager() {};
	virtual void write(const Identifier& id, const var& newValue, NotificationType n=dontSendNotification) = 0;
	virtual var read(const Identifier& id) const = 0;

	virtual void addToLog(const String& message) = 0;
};


struct AnalyserInfo: public ReferenceCountedObject
{
	AnalyserInfo(SimpleRingBuffer::PropertyObject* o1, SimpleRingBuffer::PropertyObject* o2)
	{
		ringBuffer[0] = new SimpleRingBuffer();
		ringBuffer[0]->setPropertyObject(o1);
		ringBuffer[1] = new SimpleRingBuffer();
		ringBuffer[1]->setPropertyObject(o2);
	}
	
	using Ptr = ReferenceCountedObjectPtr<AnalyserInfo>;

	SimpleRingBuffer::Ptr getAnalyserBuffer(bool getPost) const
	{
		return ringBuffer[(int)getPost];
	}
	
	std::pair<WeakReference<Processor>, int> currentlyAnalysedProcessor = { nullptr, 0 };
	int lastNoteNumber = -1;
	double duration = 0.0;
	SimpleRingBuffer::Ptr ringBuffer[2];
};

class AutoSaver : private Timer,
				  public ControlledObject,
				  public ProjectHandler::Listener
{
public:

	~AutoSaver() override
	{
		getMainController()->getSampleManager().getProjectHandler().removeListener(this);
	}

	AutoSaver(MainController* mc) :
	  ControlledObject(mc),
	  currentAutoSaveIndex(0)
	{
		
	}

	void updateAutosaving()
	{
		if (isAutoSaving())
			enableAutoSaving();
		else
			disableAutoSaving();
	}

	void initialise()
	{
		getMainController()->getSampleManager().getProjectHandler().addListener(this);
		projectChanged(File());
	}

private:

	int getIntervalInMinutes() const
	{
		auto value = (int)dynamic_cast<const GlobalSettingManager*>(getMainController())->getSettingsObject().getSetting(HiseSettings::Other::AutosaveInterval);

		if (value >= 1 && value <= 30)
			return value;

		return 5;
	}

	void enableAutoSaving()
	{
		IF_NOT_HEADLESS(startTimer(1000 * 60 * getIntervalInMinutes())); // autosave all 5 minutes
	}

	void disableAutoSaving()
	{
		stopTimer();
	}

	bool isAutoSaving() const
	{
		return dynamic_cast<const GlobalSettingManager*>(getMainController())->getSettingsObject().getSetting(HiseSettings::Other::EnableAutosave);
	}

	void projectChanged(const File&) override
	{
		currentAutoSaveIndex = 0;
		fileList.clear();
		updateAutosaving();
	}

	

	void timerCallback() override
	{
		Processor* mainSynthChain = getMainController()->getMainSynthChain();

		File backupFile = getAutoSaveFile();

		ValueTree v = mainSynthChain->exportAsValueTree();

		v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
		FileOutputStream fos(backupFile);
		v.writeToStream(fos);

		debugToConsole(mainSynthChain, "Autosaving as " + backupFile.getFileName());
	}

	File getAutoSaveFile()
	{
		File presetDirectory = getPresetFolder();

		if (presetDirectory.isDirectory())
		{
			if (fileList.size() == 0)
			{
				fileList.add(presetDirectory.getChildFile("Autosave_1.hip"));
				fileList.add(presetDirectory.getChildFile("Autosave_2.hip"));
				fileList.add(presetDirectory.getChildFile("Autosave_3.hip"));
				fileList.add(presetDirectory.getChildFile("Autosave_4.hip"));
				fileList.add(presetDirectory.getChildFile("Autosave_5.hip"));
			}

			File toReturn = fileList[currentAutoSaveIndex];

			if (toReturn.existsAsFile()) toReturn.deleteFile();

			currentAutoSaveIndex = (currentAutoSaveIndex + 1) % 5;

			return toReturn;
		}

		return File();
	}

	File getPresetFolder() const 
	{
		return getMainController()->getSampleManager().getProjectHandler().getSubDirectory(FileHandlerBase::Presets);
	}

	Array<File> fileList;

	int currentAutoSaveIndex;
};

struct ExampleAssetManager: public ReferenceCountedObject,
							public ProjectHandler
						    
{
	ExampleAssetManager(MainController* mc):
	  ProjectHandler(mc),
	  mainProjectHandler(mc->getCurrentFileHandler())
	{
		
	}

	bool initialised = false;

	void initialise();

	FileHandlerBase& mainProjectHandler;

	File getSubDirectory(SubDirectories dir) const override;

	Array<SubDirectories> getSubDirectoryIds() const override;

	File getRootFolder() const override
	{
		return rootDirectory;
	}

	File rootDirectory;

	using Ptr = ReferenceCountedObjectPtr<ExampleAssetManager>;
};

struct PluginParameterSimulatorInfo
{
	enum class SourceThread
	{
		UI,
		Custom,
		Audio,
		numSourceThreads
	};

	enum class EventType
	{
		Undefined,
		BeginGesture,
		ValueChange,
		EndGesture
	};

	operator bool() const { return currentParameter != nullptr && eventType != EventType::Undefined; }

	void performChange()
	{
		jassert(currentValue >= 0.0f && currentValue <= 1.0f);

		if(eventType == EventType::ValueChange)
			currentParameter->asJuceParameter()->setValueNotifyingHost(currentValue);
	}

	void performGesture()
	{
		if(eventType == EventType::BeginGesture)
			currentParameter->asJuceParameter()->beginChangeGesture();
		else if (eventType == EventType::EndGesture)
			currentParameter->asJuceParameter()->endChangeGesture();
	}

	bool isActiveGesture() const
	{
		return eventType == EventType::BeginGesture || eventType == EventType::ValueChange;
	}

	bool isGestureEvent() const
	{
		return eventType == EventType::BeginGesture || eventType == EventType::EndGesture;
	}

	SourceThread sourceThread = SourceThread::UI;
	bool useRamp = false;
	int bufferSize = -1;
	WeakReference<HisePluginParameterBase> currentParameter;
	float currentValue = 0.0f;
	EventType eventType = EventType::Undefined;
};

struct PluginParameterRamp: public PooledUIUpdater::SimpleTimer,
						    public ControlledObject,
							public Thread
{
	PluginParameterRamp(MainController* mc):
	  SimpleTimer(mc->getGlobalUIUpdater(), false),
	  ControlledObject(mc),
	  Thread("Custom Automation Thread")
	{}

	~PluginParameterRamp()
	{
		stopThread(1000);
	}

	void run() override
	{
		while(!threadShouldExit())
		{
			PluginParameterSimulatorInfo thisInfo;

			{
				SimpleReadWriteLock::ScopedReadLock sl(lock);
				thisInfo = currentInfo;
			}

			if(gestureAtNextCallback)
			{
				gestureInfo.performGesture();
				gestureAtNextCallback = false;

				if(gestureInfo.eventType == PluginParameterSimulatorInfo::EventType::EndGesture)
				{
					currentInfo = {};
					thisInfo = {};
					gestureInfo = {};
				}
			}

			if(thisInfo && thisInfo.sourceThread == PluginParameterSimulatorInfo::SourceThread::Custom)
			{
				if(currentInfo.useRamp)
					performTimer(thisInfo);
				else
				{
					thisInfo.performChange();
					currentInfo = {};
				}
			}
				
			sleep(5);
		}
	}

	void performTimer(PluginParameterSimulatorInfo& infoToUse)
	{
		auto thisTime = Time::getMillisecondCounterHiRes();
		auto delta = jlimit(3.0, 60.0, thisTime - lastTimer);
		lastTimer = thisTime;
		hise::SimpleReadWriteLock::ScopedReadLock sl(lock);
		bump(infoToUse, delta);
		currentInfo.currentValue = infoToUse.currentValue;
	}

	void timerCallback() override
	{
		performTimer(currentInfo);
	}

	using ProcessCallback = std::function<void(float**, AudioSampleBuffer&, MidiBuffer&, int, int)>;

	bool processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages, const ProcessCallback& f);

	void setCurrentInfo(const PluginParameterSimulatorInfo& newInfo);

	void bump(PluginParameterSimulatorInfo& info, double milliSeconds);

private:

	double lastTimer = 0.0f;

	bool gestureAtNextCallback = false;
	bool sign = true;

	hise::SimpleReadWriteLock lock;

	PluginParameterSimulatorInfo gestureInfo;
	PluginParameterSimulatorInfo currentInfo;
};

/** This is the main audio processor for the backend application. 
*
*	It connects to a BackendProcessorEditor and has extensive development features.
*
*	It is a wrapper for all plugin types and provides 8 parameters for the macro controls.
*	It also acts as global MainController to allow every child object to get / set certain global information
*/
class BackendProcessor: public AudioProcessorDriver,
					    public PluginParameterAudioProcessor,
						public MainController,
						public ProjectHandler::Listener,
						public MarkdownDatabaseHolder,
						public ExpansionHandler::Listener,
						public SimpleRingBuffer::WriterBase,
						public RestServer::Listener
{
public:
	BackendProcessor(AudioDeviceManager *deviceManager_=nullptr, AudioProcessorPlayer *callback_=nullptr);

	~BackendProcessor();

	void projectChanged(const File& newRootDirectory) override;

	void refreshExpansionType();

	void handleEditorData(bool save);

	void prepareToPlay (double sampleRate, int samplesPerBlock);
	void releaseResources();

	void checkLatency();;

	void getStateInformation	(MemoryBlock &destData) override;;

	void handleLatencyCheck(AudioSampleBuffer& buffer);
	void handlePostLatencyCheck(AudioSampleBuffer& buffer);

	void logMessage(const String& message, bool isCritical) override;

	void setStateInformation(const void *data,int sizeInBytes) override;

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	virtual void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);;

	void handleControllersForMacroKnobs(const MidiBuffer &midiMessages);

	AudioProcessorEditor* createEditor();
	bool hasEditor() const {return true;};

	bool acceptsMidi() const {return true;};
	bool producesMidi() const {return false;};
	
	RestServer::Response onAsyncRequest(RestServer::AsyncRequest::Ptr req);

	// RestServer::Listener callbacks
	void serverStarted(int port) override;
	void serverStopped() override;
	void requestReceived(const String& method, const String& path) override;
	void serverError(const String& message) override;

	double getTailLengthSeconds() const {return 0.0;};

	ModulatorSynthChain *getMainSynthChain() override {return synthChain; };

	const ModulatorSynthChain *getMainSynthChain() const override { return synthChain; }

	void registerItemGenerators() override;
	void registerContentProcessor(MarkdownContentProcessor* processor) override;
	File getCachedDocFolder() const override;
	File getDatabaseRootDirectory() const override;

	void toggleDynamicBufferSize() 
	{
		simulateDynamicBufferSize = !isUsingDynamicBufferSize();
	}

	void setDatabaseRootDirectory(File newDatabaseDirectory)
	{
		databaseRoot = newDatabaseDirectory;
	}

	BackendProcessor* getDocProcessor();

	BackendRootWindow* getDocWindow();

	Component* getRootComponent() override;

	static void setUseCommandLineServerMode(int port) { commandLineServerPort = port; }
	static bool isUsingCommandLineServerMode() { return commandLineServerPort != 0; }

	bool databaseDirectoryInitialised() const override
	{
		auto path = getSettingsObject().getSetting(HiseSettings::Documentation::DocRepository).toString();

		auto d = File(path);
		
		return d.isDirectory() && d.getChildFile("hise-modules").isDirectory();
	}

	/// @brief returns the number of PluginParameter objects, that are added in the constructor
    int getNumParameters() override
	{
		return 8;
	}

	AutoSaver& getAutoSaver() { return autosaver; }
	
	/** Returns the InteractionTester for UI interaction testing via REST API. */
	InteractionTester* getInteractionTester();

	/** Returns the MidiInjector for MIDI injection via REST API. */
	MidiInjector* getMidiInjector();
	
	/** Shows the Interaction Test Window, creating the tester if needed. */
	void showInteractionTestWindow();

	/// @brief returns the PluginParameter value of the indexed PluginParameter.
    float getParameter (int index) override
	{
		return synthChain->getMacroControlData(index)->getCurrentValue() / 127.0f;
	}

	/** @brief sets the PluginParameter value.
	
		This method uses the 0.0-1.0 range to ensure compatibility with hosts. 
		A more user friendly but slower function for GUI handling etc. is setParameterConverted()
	*/
    void setParameter (int index, float newValue) override
	{
		synthChain->setMacroControl(index, newValue * 127.0f, sendNotificationAsync);
	}


	/// @brief returns the name of the PluginParameter
    const String getParameterName (int index) override
	{
		return "Macro " + String(index + 1);
	}

	/// @brief returns a converted and labeled string that represents the current value
    const String getParameterText (int index) override
	{
		
		return String(synthChain->getMacroControlData(index)->getDisplayValue(), 1);
	}

	void rebuildPluginParameters() override
	{
		if(deletePendingFlag)
			return;

#if IS_STANDALONE_APP
		auto& oldParameters = getParameterTree();
		auto fl = oldParameters.getParameters(true);

		for(auto p: fl)
		{
			if(auto t = dynamic_cast<HisePluginParameterBase*>(p))
				t->cleanup();
		}

		setParameterTree({});

		juce::AudioProcessorParameterGroup list;
		createScriptedParameters(list);

		//processParameterList(list);

		setParameterTree(std::move(list));

		pluginParameterRefreshBroadcaster.sendMessage(sendNotificationAsync, true);
#endif
	}

	JavascriptProcessor* createInterface(int width, int height, bool compile=true);;

	void setEditorData(var editorState);

	juce::OnlineUnlockStatus* getLicenseUnlocker() 
	{
		return &scriptUnlocker;
	}

	/** Creates or returns the build undo manager f. */
	ControlledObject* getOrCreateRestServerBuildUndoManager();
	
	RestServer& getRestServer() { return restServer; }

	ControlledObject* getRestWizardRunner();

	simple_css::Animator& getCssParseAnimator() { return restServerAnimator; }

	BackendRootWindow* currentRootWindow = nullptr;

	LambdaBroadcaster<bool> pluginParameterRefreshBroadcaster;

	ScriptUnlocker scriptUnlocker;

#if HISE_INCLUDE_SNEX_FLOATING_TILES
	snex::ui::WorkbenchManager workbenches;
	virtual void* getWorkbenchManager() override { return reinterpret_cast<void*>(&workbenches); }
#endif

	BackendDllManager::Ptr dllManager;

	
    LambdaBroadcaster<Processor*> processorAddBroadcaster;
	
	LambdaBroadcaster<Identifier, Processor*> workspaceBroadcaster;

    ExternalClockSimulator externalClockSim;
	
	void pushToAnalyserBuffer(AnalyserInfo::Ptr info, bool post, const AudioSampleBuffer& buffer, int numSamples);

	int getCurrentAnalyserNoteNumber() const
	{
		return currentNoteNumber;
	}

	AnalyserInfo::Ptr getAnalyserInfoForProcessor(Processor* p);

	Result setAnalysedProcessor(AnalyserInfo::Ptr newInfo, bool add);

	bool isSnippetBrowser() const
	{
		return isSnippet;
	}

	void setIsSnippetBrowser()
	{
		isSnippet = true;
	}

	ExampleAssetManager::Ptr getAssetManager()
	{
		if(assetManager == nullptr)
			assetManager = new ExampleAssetManager(this);

		return assetManager;
	}

	ExampleAssetManager::Ptr assetManager;

	File customDocCacheFolder;

	PluginParameterRamp pluginParameterRamp;

	static int commandLineServerPort;

private:

#if HISE_INCLUDE_PROFILING_TOOLKIT
	int numPressedKeys = 0;
#endif

	bool isSnippet = false;

	enum class LatencyCheckState
	{
		Idle,
		WaitingForKillCounter,
		WaitingForProcessBlock,
		WaitingForImpulse,
		Done,
		numLatencyCheckStates,
	};

	LatencyCheckState latencyCheckState = LatencyCheckState::Idle;
	double reportedLatency = 0.0;
	int killCounter = 0;

	int currentNoteNumber = -1;

	mutable SimpleReadWriteLock postAnalyserLock;

	ReferenceCountedArray<AnalyserInfo> currentAnalysers;
	
	File databaseRoot;

	MemoryBlock tempLoadingData;

	friend class BackendProcessorEditor;
	friend class BackendCommandTarget;
	friend class CombinedDebugArea;

	ScopedPointer<ModulatorSynthChain> synthChain;
	
	var editorInformation;

	ScopedPointer<BackendProcessor> docProcessor;
	BackendRootWindow* docWindow;

	AutoSaver autosaver;

	ScopedPointer<ControlledObject> wizardRunner;
	ScopedPointer<ControlledObject> buildUndoManager;

	RestServer restServer;
	simple_css::Animator restServerAnimator;
	
	std::unique_ptr<InteractionTester> interactionTester;
	std::unique_ptr<MidiInjector> midiInjector;

	hise::ProcessorMetadataRegistry processorDatabase;

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(BackendProcessor);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackendProcessor)
};

struct ScopedAnalyser
{
	ScopedAnalyser(MainController* mc, Processor* p, AudioSampleBuffer& b, int numSamples_):
	  bp(static_cast<BackendProcessor*>(mc))
	{
		info = bp->getAnalyserInfoForProcessor(p);

		if(info == nullptr)
			return;

		buffer = &b;
		numSamples = numSamples_;

		bp->pushToAnalyserBuffer(info, false, *buffer, numSamples);
		milliSeconds = Time::getMillisecondCounterHiRes();
	}

	~ScopedAnalyser()
	{
		if(buffer != nullptr)
		{
			auto now = Time::getMillisecondCounterHiRes();

			info->duration = now - milliSeconds;
			bp->pushToAnalyserBuffer(info, true, *buffer, numSamples);
		}
	}

	double milliSeconds = 0.0;
	AnalyserInfo::Ptr info;
	AudioSampleBuffer* buffer = nullptr;
	int numSamples = -1;
	BackendProcessor* bp;
	
};


} // namespace hise

#endif

