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

#ifndef MAINCONTROLLER_H_INCLUDED
#define MAINCONTROLLER_H_INCLUDED

/** A class for handling application wide tasks.
*	@ingroup core
*
*	Every Processor must be connected to a MainController instance and has access to its public methods.
*
*/
class MainController: public GlobalScriptCompileBroadcaster,
					  public OverlayMessageBroadcaster,
					  public ThreadWithQuasiModalProgressWindow::Holder
{
public:

	/** Contains all methods related to sample management. */
	class SampleManager
	{
	public:

		enum class DiskMode
		{
			SSD = 0,
			HDD,
			numDiskModes
		};

		SampleManager(MainController *mc);

		void setLoadedSampleMaps(ValueTree &v) { sampleMaps = v; };

		/** returns a pointer to the thread pool that streams the samples from disk. */
		SampleThreadPool *getGlobalSampleThreadPool() { return samplerLoaderThreadPool; }

		/** returns a pointer to the global sample pool */
		ModulatorSamplerSoundPool *getModulatorSamplerSoundPool() const { return globalSamplerSoundPool; }

		/** Copies the samples to an internal clipboard for copy & paste functionality. */
		void copySamplesToClipboard(const Array<WeakReference<ModulatorSamplerSound>> &soundsToCopy);

		const ValueTree &getSamplesFromClipboard() const;

		bool clipBoardContainsData() const { return sampleClipboard.getNumChildren() != 0;}

		/** returns the fixed streaming buffer size. */
		int getStreamingBufferSize() const { return 4096; };

		/** Returns the ValueTree that represents the samplemap with the specified file name.
		*
		*	This is used when a sample map is loaded - it checks if the name already exists in the loaded monolithic data
		*	and loads the sounds from there if there is a match. 
		*/
		const ValueTree getLoadedSampleMap(const String &fileName) const;

		bool shouldUseRelativePathToProjectFolder() const { return useRelativePathsToProjectFolder; }

		void setShouldUseRelativePathToProjectFolder(bool shouldUse) { useRelativePathsToProjectFolder = shouldUse; }

		/** Returns the impulse response pool. */
		const AudioSampleBufferPool *getAudioSampleBufferPool() const {	return globalAudioSampleBufferPool; };

		/** Returns the impulse response pool. */
		AudioSampleBufferPool *getAudioSampleBufferPool() {	return globalAudioSampleBufferPool; };

		ImagePool *getImagePool() {return globalImagePool;};
		const ImagePool *getImagePool() const {return globalImagePool;};

		ProjectHandler &getProjectHandler() { return projectHandler; }

		void setDiskMode(DiskMode mode) noexcept;

		const ProjectHandler &getProjectHandler() const { return projectHandler; }

		bool isUsingHddMode() const noexcept{ return hddMode; };

		bool shouldSkipPreloading() const { return skipPreloading; };

		void setShouldSkipPreloading(bool skip) { skipPreloading = skip; }

		void preloadEverything();

	private:

		ProjectHandler projectHandler;

		MainController* mc;

		ValueTree sampleClipboard;
		ValueTree sampleMaps;
		ScopedPointer<AudioSampleBufferPool> globalAudioSampleBufferPool;
		ScopedPointer<ImagePool> globalImagePool;
		ScopedPointer<ModulatorSamplerSoundPool> globalSamplerSoundPool;
		ScopedPointer<SampleThreadPool> samplerLoaderThreadPool;

		bool hddMode = false;
		bool useRelativePathsToProjectFolder;
		bool skipPreloading = false;
	};

	/** Contains methods for handling macros. */
	class MacroManager
	{
	public:

		MacroManager(MainController *mc_);
		~MacroManager() {};

		// ===========================================================================================================

		ModulatorSynthChain *getMacroChain() { return macroChain; };
		const ModulatorSynthChain *getMacroChain() const {return macroChain; };
		void setMacroChain(ModulatorSynthChain *chain) { macroChain = chain; }

		// ===========================================================================================================
		
		void setMidiControllerForMacro(int midiControllerNumber);
		void setMidiControllerForMacro(int macroIndex, int midiControllerNumber);;
		void setMacroControlMidiLearnMode(ModulatorSynthChain *chain, int index);
		int getMacroControlForMidiController(int midiController);
		int getMidiControllerForMacro(int macroIndex);

		bool midiMacroControlActive() const;
		bool midiControlActiveForMacro(int macroIndex) const;;
		bool macroControlMidiLearnModeActive() { return macroIndexForCurrentMidiLearnMode != -1; }

		void setMacroControlLearnMode(ModulatorSynthChain *chain, int index);
		int getMacroControlLearnMode() const;;

		void removeMidiController(int macroIndex);
		void removeMacroControlsFor(Processor *p);
		void removeMacroControlsFor(Processor *p, Identifier name);
	
		MidiControllerAutomationHandler *getMidiControlAutomationHandler();;
		const MidiControllerAutomationHandler *getMidiControlAutomationHandler() const;;

		// ===========================================================================================================

	private:

		MainController *mc;

		int macroControllerNumbers[8];

		ModulatorSynthChain *macroChain;
		int macroIndexForCurrentLearnMode;
		int macroIndexForCurrentMidiLearnMode;

		MidiControllerAutomationHandler midiControllerHandler;

		// ===========================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroManager)
	};

	class EventIdHandler
	{
	public:

		// ===========================================================================================================

		EventIdHandler(HiseEventBuffer& masterBuffer_);
		~EventIdHandler();

		// ===========================================================================================================

		/** Fills note on / note off messages with the event id and returns the current value for external storage. */
		void handleEventIds();

		/** Removes the matching noteOn event for the given noteOff event. */
		uint16 getEventIdForNoteOff(const HiseEvent &noteOffEvent);

		/** Returns the matching note on event for the given note off event (but doesn't remove it). */
		HiseEvent peekNoteOn(const HiseEvent& noteOffEvent);

		/** Adds the artificial event to the internal stack array. */
		void pushArtificialNoteOn(HiseEvent& noteOnEvent) noexcept;

		/** Searches all active note on events and returns the one with the given event id. */
		HiseEvent popNoteOnFromEventId(uint16 eventId);

		/** You can specify a global transpose value here that will be added to all note on / note off messages. */
		void setGlobalTransposeValue(int transposeValue);

		/** Adds a CC remapping configuration. If this is enabled, the CC numbers will be swapped. If you pass in the same numbers, it will be deactivated. */
		void addCCRemap(int firstCC_, int secondCC_);;

		// ===========================================================================================================

	private:

        std::atomic<int> firstCC;
        std::atomic<int> secondCC;

		const HiseEventBuffer &masterBuffer;
		HeapBlock<HiseEvent> artificialEvents;
		uint16 lastArtificialEventIds[128];
		HiseEvent realNoteOnEvents[128];
		uint16 currentEventId;

		int transposeValue = 0;

		// ===========================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventIdHandler)
	};

	class UserPresetHandler : public Timer,
							  public ThreadWithQuasiModalProgressWindow::Holder::Listener
	{
	public:

		enum RampFlags
		{
			Active = 0,
			Bypassed = -2,
			FadeIn = 1,
			FadeOut = -1
		};

		UserPresetHandler(MainController* mc_);
		~UserPresetHandler();

		void lastTaskRemoved() override;

		// ===========================================================================================================

		void timerCallback();
		void loadUserPreset(const ValueTree& presetToLoad);

		File getCurrentlyLoadedFile() const { return currentlyLoadedFile; };

		void setCurrentlyLoadedFile(const File& f) { currentlyLoadedFile = f; };

		// ===========================================================================================================

	private:

		void loadPresetInternal();

		
		MainController* mc;
		ValueTree currentPreset;

		File currentlyLoadedFile;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UserPresetHandler)

	};

	MainController();

	virtual ~MainController();

	SampleManager &getSampleManager() {return *sampleManager; };
	const SampleManager &getSampleManager() const { return *sampleManager; };

	MacroManager &getMacroManager() {return macroManager;};
	const MacroManager &getMacroManager() const {return macroManager;};

	AutoSaver &getAutoSaver() { return autoSaver; }
	const AutoSaver &getAutoSaver() const { return autoSaver; }

	DelayedRenderer& getDelayedRenderer() { return delayedRenderer; };
	const DelayedRenderer& getDelayedRenderer() const { return delayedRenderer; };

	UserPresetHandler& getUserPresetHandler() { return userPresetHandler; };
	const UserPresetHandler& getUserPresetHandler() const { return userPresetHandler; };

#if USE_BACKEND
	/** Writes to the console. */
	void writeToConsole(const String &message, int warningLevel, const Processor *p=nullptr, Colour c=Colours::transparentBlack);
#endif

	void loadPreset(const File &f, Component *mainEditor=nullptr);
	void loadPreset(ValueTree &v, Component *mainEditor=nullptr);
    void clearPreset();
    
	/** Compiles all scripts in the main synth chain */
	void compileAllScripts();

	/** Call this if you want all voices to stop. */
	void allNotesOff() { allNotesOffFlag = true; };

	/** Create a new processor and returns it. You have to supply a Chain that the Processor will be added to.
	*
	*	The function is static (it will get the MainController() instance from the FactoryType).
	*
	*	@param FactoryType this is used to create the processor and connect it to the MainController.
	*	@param typeName the identifier string of the processor that should be created.
	*	@param id the name of the processor to be created.
	*
	*	@returns a new processor. You have to manage the ownership yourself.
	*/
	static Processor *createProcessor(FactoryType *FactoryTypeToUse, 
 							   const Identifier &typeName, 
							   const String &id);

	
	
	/** same as AudioProcessor::beginParameterGesture(). */
	void beginParameterChangeGesture(int index);
	
	/** same as AudioProcessor::beginParameterGesture(). */
	void endParameterChangeGesture(int index);
	
	/** sets the plugin parameter to the new Value. */
	void setPluginParameter(int index, float newValue);
	
	/** Returns the uptime in seconds. */
	double getUptime() const noexcept { return uptime; }

	/** returns the tempo as bpm. */
    double getBpm() const noexcept
    {
        return bpm.load() > 0.0 ? bpm.load() : 120.0;
    };

	/** skins the given component (applies the global look and feel to it). */
    void skin(Component &c);
	
	/** adds a TempoListener to the main controller that will receive a callback whenever the host changes the tempo. */
	void addTempoListener(TempoListener *t);

	/** removes a TempoListener. */
	void removeTempoListener(TempoListener *t);;

	ApplicationCommandManager *getCommandManager() { return mainCommandManager; };

    const CriticalSection &getLock() const;
    
	AudioProcessor* getAsAudioProcessor() { return dynamic_cast<AudioProcessor*>(this); };
	const AudioProcessor* getAsAudioProcessor() const { return dynamic_cast<const AudioProcessor*>(this); };

	DebugLogger& getDebugLogger() { return debugLogger; }
	const DebugLogger& getDebugLogger() const { return debugLogger; }
    
	void setKeyboardCoulour(int keyNumber, Colour colour);

	CustomKeyboardState &getKeyboardState();

	void setLowestKeyToDisplay(int lowestKeyToDisplay);

	void addPlottedModulator(Modulator *m);
	void removePlottedModulator(Modulator *m);
	
#if USE_BACKEND

	void setScriptWatchTable(ScriptWatchTable *table);
	void setScriptComponentEditPanel(ScriptComponentEditPanel *panel);

	void setWatchedScriptProcessor(JavascriptProcessor *p, Component *editor);

	/** Sets the ScriptComponent object to be edited in the main ScriptComponentEditPanel.
	*
	*	The pointer is a DynamicObject because of forward declaration issues, but make sure, you only call this method with ScriptComponent objects
	*	(otherwise it will have no effect)
	*/
	void setEditedScriptComponent(ReferenceCountedObject* c, Component *listener);
	
#endif

	void setPlotter(Plotter *p);

	void setCurrentViewChanged();
	
	/** saves a variable into the global index. */
	void setGlobalVariable(int index, var newVariable);

	/** returns the variable saved at the global index. */
	var getGlobalVariable(int index) const;

	DynamicObject *getGlobalVariableObject() { return globalVariableObject.get(); };

	DynamicObject *getHostInfoObject() { return hostInfo.get(); }

	DynamicObject *getToolbarPropertiesObject() { return toolbarProperties.get(); };

	/** this must be overwritten by the derived class and return the master synth chain. */
	virtual ModulatorSynthChain *getMainSynthChain() = 0;

	virtual const ModulatorSynthChain *getMainSynthChain() const = 0;

	/** Returns the time that the plugin spends in its processBlock method. */
	float getCpuUsage() const {return usagePercent.load();};

	/** Returns the amount of playing voices. */
	int getNumActiveVoices() const;;

	void replaceReferencesToGlobalFolder();

	void showConsole(bool consoleShouldBeShown);

	/**	sets the console component to display all incoming messages. 
	*
	*	If you set 'isPopupConsole' to true, it will not overwrite the pointer to the main console.
	*/
	void setConsole(Console *c, bool isPopupConsole=false)
	{
#if USE_BACKEND
		if (isPopupConsole)
		{
			popupConsole = c;
			usePopupConsole = c != nullptr;
		}
		else
		{
			console = c;
			usePopupConsole = false;
		}

#else 
		ignoreUnused(c, isPopupConsole);

#endif
		
	};

	Console* getConsole() { return console; }

    void clearConsole();

	void setLastActiveEditor(CodeEditorComponent *editor, CodeDocument::Position position)
	{
		lastActiveEditor = editor;
		lastCharacterPositionOfSelectedEditor = position.getPosition();
	}

	void insertStringAtLastActiveEditor(const String &string, bool selectArguments);

	void loadTypeFace(const String& fileName, const void* fontData, size_t fontDataSize);

	void fillWithCustomFonts(StringArray &fontList);
	juce::Typeface* getFont(const String &fontName) const;
	ValueTree exportCustomFontsAsValueTree() const;
	void restoreCustomFontValueTree(const ValueTree &v);

    bool checkAndResetMidiInputFlag();
    bool isChanged() const { return changed; }
    void setChanged(bool shouldBeChanged=true) { changed = shouldBeChanged; }
    
    float getGlobalCodeFontSize() const {return globalCodeFontSize; };
    
    void setGlobalCodeFontSize(float newFontSize)
    {
        globalCodeFontSize = newFontSize;
        codeFontChangeNotificator.sendSynchronousChangeMessage();
    };
    
    SafeChangeBroadcaster &getFontSizeChangeBroadcaster() { return codeFontChangeNotificator; };
    
    /** This sets the global pitch factor. */
    void setGlobalPitchFactor(double pitchFactorInSemiTones)
    {
        globalPitchFactor = pow(2, pitchFactorInSemiTones / 12.0);
    }
    
    /** This returns the global pitch factor. 
    *
    *   Use this in your startVoice method and multiplicate it with your angleDelta.
    */
    double getGlobalPitchFactor() const
    {
        return globalPitchFactor;
    }
    
    /** This returns the global pitch factor as semitones. 
    *
    *   This can be used for displaying / saving purposes.
    */
    double getGlobalPitchFactorSemiTones() const
    {
        return log2(globalPitchFactor) * 12.0;
    }
    
	bool &getPluginParameterUpdateState() { return enablePluginParameterUpdate; }

	void createUserPresetData()
	{
		userPresetData = new UserPresetData(this);
	}

	const UserPresetData* getUserPresetData() const { return userPresetData; }
	
	void rebuildUserPresetDatabase() { userPresetData->refreshPresetFileList(); }

	ReadWriteLock &getCompileLock() { return compileLock; }

	EventIdHandler& getEventHandler() { return eventIdHandler; }

	bool shouldSkipCompiling() const
	{
		return skipCompilingAtPresetLoad;
	}

	void loadUserPresetAsync(const ValueTree& v)
	{
		allNotesOff();
		presetLoadRampFlag.set(-1);
		userPresetHandler.loadUserPreset(v);
	}

	UndoManager* getControlUndoManager() { return controlUndoManager; }

	struct ScopedSuspender
	{
		enum class LockType
		{
			SuspendOnly,
			SuspendWithBusyWait,
			Lock,
			TryLock,
			numLockTypes
		};

		ScopedSuspender(MainController* mc_, LockType lockType_=LockType::SuspendOnly) :
			mc(mc_),
			lockType(lockType_)
		{
			switch (lockType)
			{
			case MainController::ScopedSuspender::LockType::SuspendOnly:
				mc->suspendProcessing(true);
				break;
			case MainController::ScopedSuspender::LockType::SuspendWithBusyWait:
			{

				const CriticalSection& lock = mc->getLock();

				ScopedTryLock sl(lock);
				
				while (!lock.tryEnter())
				{

				}

				lock.exit();

				mc->suspendProcessing(true);

				break;
			}
			case MainController::ScopedSuspender::LockType::Lock:
				mc->getLock().enter();
				break;
			case MainController::ScopedSuspender::LockType::TryLock:
				hasLock = mc->getLock().tryEnter();
				break;
			case MainController::ScopedSuspender::LockType::numLockTypes:
				break;
			default:
				break;
			}

			
		}

		~ScopedSuspender()
		{
			if (mc.get() != nullptr)
			{
				switch (lockType)
				{
				case MainController::ScopedSuspender::LockType::SuspendOnly:
					mc->suspendProcessing(false);
					break;
				case MainController::ScopedSuspender::LockType::Lock:
					mc->getLock().exit();
					break;
				case MainController::ScopedSuspender::LockType::TryLock:
					if (hasLock) mc->getLock().exit();
					break;
				case MainController::ScopedSuspender::LockType::numLockTypes:
					break;
				default:
					break;
				}
			}
			else
			{
				jassertfalse;
			}
		}

	private:

		const LockType lockType;
		bool hasLock = false;

		WeakReference<MainController> mc;
	};


private: // Never call this directly, but wrap it through DelayedRenderer...

	/** This is the main processing loop that is shared among all subclasses. */
	void processBlockCommon(AudioSampleBuffer &b, MidiBuffer &mb);

	/** Sets the sample rate for the cpu meter. */
	void prepareToPlay(double sampleRate_, int samplesPerBlock);


protected:


	/** sets the new BPM and sends a message to all registered tempo listeners if the tempo changed. */
	void setBpm(double bpm_);

	/** @brief Add this at the beginning of your processBlock() method to enable CPU measurement */
	void startCpuBenchmark(int bufferSize);

	/** @brief Add this at the end of your processBlock() method to enable CPU measurement */
	void stopCpuBenchmark();

	/** Checks if a connected object called allNotesOff() and replaces the content of the supplied MidiBuffer with a allNoteOff event. */
	void checkAllNotesOff()
	{
		if(allNotesOffFlag)
		{
			masterEventBuffer.clear();
			masterEventBuffer.addEvent(HiseEvent(HiseEvent::Type::AllNotesOff, 0, 0, 1));

			allNotesOffFlag = false;
		}
	};

	double uptime;

	void setScrollY(int newY) {	scrollY = newY;	};
	int getScrollY() const {return scrollY;};

	void setComponentShown(int componentIndex, bool isShown)
	{
		shownComponents.setBit(componentIndex, isShown);
	};

	bool isComponentShown(int componentIndex) const 
	{
		return shownComponents[componentIndex];
	}
	
	
    void setMidiInputFlag() {midiInputFlag = true; };
    
	void setReplaceBufferContent(bool shouldReplaceContent)
	{
		replaceBufferContent = shouldReplaceContent;
	}

	bool suspendProcessing(bool shouldSuspend)
	{
		if (shouldSuspend)
		{
			if (suspendIndex == 0)
				getAsAudioProcessor()->suspendProcessing(true);

			++suspendIndex;
		}
		else
		{
			--suspendIndex;

			if (suspendIndex == 0)
				getAsAudioProcessor()->suspendProcessing(false);

			jassert(suspendIndex >= 0);

			
		}
			
		return suspendIndex >= 0;
	}

private:

	CriticalSection processLock;

	ScopedPointer<UndoManager> controlUndoManager;

	friend class UserPresetHandler;
    friend class PresetLoadingThread;
	friend class DelayedRenderer;

	DelayedRenderer delayedRenderer;

	bool skipCompilingAtPresetLoad = false;

	bool replaceBufferContent = true;

	HiseEventBuffer masterEventBuffer;
	EventIdHandler eventIdHandler;
	UserPresetHandler userPresetHandler;

	ScopedPointer<UserPresetData> userPresetData;

	void storePlayheadIntoDynamicObject(AudioPlayHead::CurrentPositionInfo &lastPosInfo);

	CustomKeyboardState keyboardState;

	AudioSampleBuffer multiChannelBuffer;

	friend class WeakReference<MainController>;
	WeakReference<MainController>::Master masterReference;

	ReferenceCountedArray<juce::Typeface> customTypeFaces;
	ValueTree customTypeFaceData;

	Array<var> globalVariableArray;

	DynamicObject::Ptr globalVariableObject;
	DynamicObject::Ptr hostInfo;
	DynamicObject::Ptr toolbarProperties;

	ReadWriteLock compileLock;

	ScopedPointer<SampleManager> sampleManager;
	MacroManager macroManager;

	Component::SafePointer<Plotter> plotter;

	Atomic<int> bufferSize;

	Atomic<int> presetLoadRampFlag;

	AudioPlayHead::CurrentPositionInfo lastPosInfo;
	
	ScopedPointer<ApplicationCommandManager> mainCommandManager;

	ScopedPointer<KnobLookAndFeel> mainLookAndFeel;

	Component::SafePointer<CodeEditorComponent> lastActiveEditor;
	int lastCharacterPositionOfSelectedEditor;

    SafeChangeBroadcaster codeFontChangeNotificator;
        
	WeakReference<Console> console;

    ScopedPointer<ConsoleLogger> logger;
    
	WeakReference<Console> popupConsole;
	bool usePopupConsole;

	AutoSaver autoSaver;

	DebugLogger debugLogger;

#if USE_BACKEND
    
	Component::SafePointer<ScriptWatchTable> scriptWatchTable;
	Component::SafePointer<ScriptComponentEditPanel> scriptComponentEditPanel;

#else

	const ScriptComponentEditPanel *scriptComponentEditPanel;
	const ScriptWatchTable *scriptWatchTable;

#endif

	Array<WeakReference<TempoListener>> tempoListeners;

    std::atomic<float> usagePercent;

	bool enablePluginParameterUpdate;

    float globalCodeFontSize;

    double globalPitchFactor;
    
    std::atomic<double> bpm;
	Atomic<int> voiceAmount;
	bool allNotesOffFlag;
    
    bool changed;
    
    bool midiInputFlag;
	
	double sampleRate;
    std::atomic<double> temp_usage;
	int scrollY;
	BigInteger shownComponents;

	std::atomic<int> suspendIndex;
};


#endif