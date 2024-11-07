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
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef MAINCONTROLLER_H_INCLUDED
#define MAINCONTROLLER_H_INCLUDED


namespace hise { using namespace juce;

class ProjectDocDatabaseHolder;

/** The grand central station of HISE.
*	@ingroup core
*
*	The MainController class represents the instance of a HISE project and can be used
*	to access quasi-global data / methods.
*
*	It is divided into multiple sub-classes which encapsulate different logic in order
*	to bring some order into the enormous task of handling everything:
*
*	- the SampleManager subclass handles pooling / preloading of samples
*	- the MacroManager (which itself has multiple subclasses) handle all automation /
*     MIDI controller tasks
*	- the UserPresetHandler takes care of the loading / browsing of user presets.
*
*	Implementations of this class are also derived by the juce::AudioProcessor and some 
*	other helper classes. Check out the hise::FrontendProcessor class for actual usage
*	in a C++ HISE project.
*
*	Most classes just want a reference to the MainController instance. If you want to
*	use it in your C++ classes, I recommend subclassing it from ControlledObject, which
*	exists for this sole purpose.
*/
class MainController: public GlobalScriptCompileBroadcaster,
					  public OverlayMessageBroadcaster,
					  public ThreadWithQuasiModalProgressWindow::Holder,
					  public Timer
{
public:

#if HI_RUN_UNIT_TESTS
	// You can set this bool globally and it will skip some annoying things like restoring the pool or spawning threads
	// when a BackendProcessor is created for testing purposes...
	static bool unitTestMode;

	static bool inUnitTestMode() { return unitTestMode; }

#else

	static bool inUnitTestMode() { return false; }

#endif

	/** This class will set a flag in the MainController to embed all
	    external resources (that are usually saved in dedicated files) into the preset.
		This is being used when exporting a HiseSnippet to ensure that it contains all
		relevant external data (DspNetwork, SNEX code files, external scripts. 

		You can query this flag with MainController::shouldEmbedAllResources.
	*/
	struct ScopedEmbedAllResources: public ControlledObject
	{
		ScopedEmbedAllResources(MainController* mc):
			ControlledObject(mc)
		{
			prevState = mc->embedAllResources;
			mc->embedAllResources = true;
		}

		~ScopedEmbedAllResources()
		{
			getMainController()->embedAllResources = prevState;
		}

		bool prevState = false;
	};

	/** Contains all methods related to sample management. */
	class SampleManager
	{
	public:

		

		/** A class that will be notified about sample preloading changes.
		*
		*	This can be used to implement loading bars / progress labels when
		*	the samples are preloaded. In order to use this, just create one of
		*	these, supply the SampleManager instance of your MainController and
		*	it will automatically register and deregister as callbacks and will
		*	call the preloadStateChanged(bool isPreloading).
		*/
		class PreloadListener
		{
		public:

			PreloadListener(SampleManager& sampleManager):
				manager(&sampleManager)
			{
				manager->addPreloadListener(this);
			}

			virtual ~PreloadListener()
			{
				if(manager != nullptr)
					manager->removePreloadListener(this);
			}

			/** This gets called whenever the preload state changes.
			*
			*	the isPreloading flag indicates whether the preloading
			*	was initiated or completed. Normally, you would start
			*	or stop a timer which regularly checks the sample progress
			*	(with SampleManager::getPreloadProgress()).
			*/
			virtual void preloadStateChanged(bool isPreloading) = 0;

		protected:

			/** Returns the preload message. */
			String getCurrentErrorMessage() const
			{
				return manager != nullptr ? manager->currentPreloadMessage : "";
			}

		private:

			WeakReference<SampleManager> manager;

			JUCE_DECLARE_WEAK_REFERENCEABLE(PreloadListener);
		};
		
		/** A POD structure that contains information about a Preload function. */
		struct PreloadThreadData
		{
			Thread* thread = nullptr;
			double* progress = nullptr;
			int samplesLoaded = 0;
			int totalSamplesToLoad = 0;
		};

		/** A PreloadFunction is a lambda that will be called by the preload thread.
		*
		*
		*	Regularly check the PreloadThreadData thread if it should exit and return false on a failure to abort.
		*/
		using PreloadFunction = std::function<bool(Processor*, PreloadThreadData&)>;

		enum class DiskMode
		{
			SSD = 0,
			HDD,
			numDiskModes
		};

		SampleManager(MainController *mc);

		~SampleManager();

		void setLoadedSampleMaps(ValueTree &v) { sampleMaps = v; };

		/** returns a pointer to the thread pool that streams the samples from disk. */
		SampleThreadPool *getGlobalSampleThreadPool() { return samplerLoaderThreadPool; }

		/** returns a pointer to the global sample pool */
		ModulatorSamplerSoundPool *getModulatorSamplerSoundPool2() const;

		void copySamplesToClipboard(const void* soundsToCopy);

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

		

		NativeFileHandler &getProjectHandler();

		void setDiskMode(DiskMode mode) noexcept;

		const NativeFileHandler &getProjectHandler() const;

		bool isUsingHddMode() const noexcept{ return hddMode; };

		bool isPreloading() const noexcept { return preloadFlag; };

		bool shouldSkipPreloading() const { return skipPreloading; };

		ModulatorSamplerSoundPool* getModulatorSamplerSoundPool();;

		/** If you load multiple samplemaps at once (eg. at startup), call this and it will coallescate the preloading. */
		void setShouldSkipPreloading(bool skip);

		/** Preload everything since the last call to setShouldSkipPreloading. */
		void preloadEverything();

		void clearPreloadFlag();
		void setPreloadFlag();

		void triggerSamplePreloading();

		void addDeferredFunction(Processor* p, const ProcessorFunction& f);

		void setCurrentPreloadMessage(String newMessage)
		{
			currentPreloadMessage.swapWith(newMessage);
		};


		void addPreloadListener(PreloadListener* p);
		void removePreloadListener(PreloadListener* p);

		double& getPreloadProgress();

		double getPreloadProgressConst() const;

		const CriticalSection& getSampleLock() const noexcept { return sampleLock; }

		void cancelAllJobs();

		void initialiseQueue();

		bool hasPendingFunction(Processor* p) const;

		bool isNonRealtime() const
		{
			return nonRealtime;
		}

		void handleNonRealtimeState();

		void setNonRealtime(bool shouldBeNonRealtime)
		{
			nonRealtime = shouldBeNonRealtime;
		}

		String getPreloadMessage() const
		{
			return currentPreloadMessage;
		}

	private:

		bool nonRealtime = false;
		bool internalsSetToNonRealtime = false;

		String currentPreloadMessage;

		struct PreloadListenerUpdater : private Timer
		{
		public:

			PreloadListenerUpdater(SampleManager* manager_) :
				manager(manager_)
			{
#if !HISE_HEADLESS
				startTimer(30);
#endif
			};

			~PreloadListenerUpdater()
			{
				stopTimer();
			}

			void triggerAsyncUpdate()
			{
#if HISE_HEADLESS
				handleAsyncUpdate();
#else
				dirty = true;
#endif
			}

		private:

			void timerCallback() override
			{
				bool value = true;

				if (dirty.compare_exchange_strong(value, false))
				{
					handleAsyncUpdate();
				}
			}

			std::atomic<bool> dirty;

			void handleAsyncUpdate();

			SampleManager* manager;
		};

		PreloadListenerUpdater preloadListenerUpdater;

		struct PreloadJob : public SampleThreadPoolJob
		{
		public:


			PreloadJob(MainController* mc);
			JobStatus runJob() override;


			double progress = 0.0;
		private:

			MainController* mc = nullptr;
			
		};


		NativeFileHandler projectHandler;

		CriticalSection sampleLock;

		MainController* mc;

		ValueTree sampleClipboard;
		ValueTree sampleMaps;

		ScopedPointer<SampleThreadPool> samplerLoaderThreadPool;

		bool hddMode = false;
		bool skipPreloading = false;

		PreloadJob internalPreloadJob;

		// Just used for the listeners
		std::atomic<bool> preloadFlag;

		bool initialised = false;

		Array<WeakReference<PreloadListener>, CriticalSection> preloadListeners;

		using SampleFunction = SuspendHelpers::Suspended<SafeFunctionCall, SuspendHelpers::ScopedTicket>;
		static constexpr auto config = MultithreadedQueueHelpers::Configuration::AllocationsAllowedAndTokenlessUsageAllowed;

		MultithreadedLockfreeQueue<SampleFunction, config> pendingFunctions;

		Array<WeakReference<Processor>> pendingProcessors;

		std::atomic<int> pendingTasksWithSuspension;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SampleManager);
	};

	/** Contains methods for handling macros, MIDI automation and MPE gestures. */
	class MacroManager
	{
	public:

		MacroManager(MainController *mc_);
		~MacroManager();;

		// ===========================================================================================================

		ModulatorSynthChain *getMacroChain();;
		const ModulatorSynthChain *getMacroChain() const;;
		void setMacroChain(ModulatorSynthChain *chain);

		// ===========================================================================================================
		
		void setMidiControllerForMacro(int midiControllerNumber);
		void setMidiControllerForMacro(int macroIndex, int midiControllerNumber);;
		void setMacroControlMidiLearnMode(ModulatorSynthChain *chain, int index);
		int getMacroControlForMidiController(int midiController);
		int getMidiControllerForMacro(int macroIndex);

		bool midiMacroControlActive() const;
		bool midiControlActiveForMacro(int macroIndex) const;;
		bool macroControlMidiLearnModeActive();

		void setMacroControlLearnMode(ModulatorSynthChain *chain, int index);
		int getMacroControlLearnMode() const;;

		void removeMidiController(int macroIndex);
		void removeMacroControlsFor(Processor *p);
		void removeMacroControlsFor(Processor *p, Identifier name);
	
		bool isMacroEnabledOnFrontend() const;;
		void setEnableMacroOnFrontend(bool shouldBeEnabled);

		MidiControllerAutomationHandler *getMidiControlAutomationHandler();;
		const MidiControllerAutomationHandler *getMidiControlAutomationHandler() const;;

		// ===========================================================================================================
		
		/** If true, then a macro can only be connected to a single target (and connecting it to another target will remove the old connection). */
		void setExclusiveMode(bool shouldBeExclusive)
		{
			exclusiveMode = shouldBeExclusive;
		}

		bool isExclusive() const { return exclusiveMode; }


	private:

		bool exclusiveMode = false;
		
		MainController *mc;
		
		int macroControllerNumbers[HISE_NUM_MACROS];

		ModulatorSynthChain *macroChain;
		int macroIndexForCurrentLearnMode;
		int macroIndexForCurrentMidiLearnMode;
		bool enableMacroOnFrontend = false;

		MidiControllerAutomationHandler midiControllerHandler;

		// ===========================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroManager)
	};

	

	/** This class is a dispatcher for methods that are being called by either the message thread or the sample loading thread.
	*
	*	The general philosophy is that the loading thread functions always have the priority. In order to enforce this method,
	*	you need to check regularly in your message thread's function if it should abort and return false so it will get called again.
	*
	*	If you can' acquire the lock in your sample loading function, call signalAbort and return false, so it will try to repeat calling
	*	the method after a short time (usually 50 ms)
	*
	*/
	class LockFreeDispatcher: private Timer
	{
	public:

		LockFreeDispatcher(MainController* mc_);;

		~LockFreeDispatcher();

		struct AbortSignal
		{
			String errorMessage;
		};

		void clearQueueWithoutCalling();
		
		bool isIdle() const;

		bool isInDispatchLoop() const noexcept { return inDispatchLoop; }

		/** This method executes the function with the given DispatchableBaseObject.
		
			It will call it synchronously if its on the message thread and there's no abort signal from a 
			pending global lock.

			If it needs to be called asynchronously, it will be added to the internal queue.
			In your method you should need to regularly check if the operation should abort and
			return DispatchableObject::Status::needsToRunAgain
		*/
		bool callOnMessageThreadAfterSuspension(Dispatchable* object, const Dispatchable::Function& f);

		struct PresetLoadListener
		{
			virtual ~PresetLoadListener() {};

			virtual void newHisePresetLoaded() = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(PresetLoadListener);
		};

		void addPresetLoadListener(PresetLoadListener* l)
		{
			presetLoadListeners.addIfNotAlreadyThere(l);

			l->newHisePresetLoaded();
		}

		void removePresetLoadListener(PresetLoadListener* l)
		{
			presetLoadListeners.removeAllInstancesOf(l);
		}

		void sendPresetReloadMessage()
		{
			for (auto l : presetLoadListeners)
			{
				if (l.get() != nullptr)
					l->newHisePresetLoaded();
			}
		}

		void clearRoutingManagerAsync()
		{
			auto rm = mc->getGlobalRoutingManager();

			if(rm != nullptr)
			{
				routingManagerToDelete = var(rm);
				mc->setGlobalRoutingManager(nullptr);
			}
		}

	private:

		var routingManagerToDelete;
		Array<WeakReference<PresetLoadListener>> presetLoadListeners;

		struct Job
		{
			Job()
			{};

			Job(Dispatchable* object, const Dispatchable::Function &f) noexcept :
				obj(object),
				func(f)
			{};

			~Job();

			Dispatchable::Status run();

			void cancel();

			bool isDone() const noexcept;

		private:
			
			Dispatchable::Status status = Dispatchable::Status::notExecuted;
			WeakReference<Dispatchable> obj;
			Dispatchable::Function func;
		};

		MultithreadedLockfreeQueue<Job, MultithreadedQueueHelpers::Configuration::AllocationsAllowedAndTokenlessUsageAllowed> pendingTasks;

		void timerCallback() override;
		bool isMessageThread() const noexcept;
		bool isLoadingThread() const noexcept;

		bool inDispatchLoop = false;

		MainController* mc;

		JUCE_DECLARE_WEAK_REFERENCEABLE(LockFreeDispatcher);
	};

	/** The handler class for user presets in HISE.
	*
	*	A user preset is a certain state of your plugin which is used
	*	for factory banks, storing the plugin state in the DAW as well as
	*	files created by the user that restore a certain sound. 
	*
	*	If you use a scripted interface, the data that is stored will be a
	*	XML element with the value of every ScriptComponent that has the
	*	`saveInPreset` flag enabled. 
	*
	*	However if you are bypassing the scripted interface logic and write your
	*	UI and signal path completely in C++, you can still use the logic / file
	*	handling of this class. In this case, you will be given a ValueTree that
	*	you have to fill / restore with the callbacks in the FrontendProcessor. 
	*
	*	This class is deeply connected to the MultiColumnPresetBrowser class, which
	*	is the general purpose preset management UI component of HISE and offers
	*	functionality like a folder-based hierarchy, search field, favorite system and
	*	tags.
	*
	*	It also makes sure that the loading of presets are as smooth as possible by
	*	killing all voices, then load the preset on the background thread and call
	*	all registered UserPresetHandler::Listener objects when the loading has finished
	*	so you can update your UI (or whatever).
	*/
	class UserPresetHandler: public Dispatchable
	{
	public:

        struct ScopedInternalPresetLoadSetter: public ControlledObject
        {
            ScopedInternalPresetLoadSetter(MainController* mc):
              ControlledObject(mc)
            {
                auto& flag = getMainController()->getUserPresetHandler().isInternalPresetLoadFlag;
                prevValue = flag;
                flag = true;
            }
            
            ~ScopedInternalPresetLoadSetter()
            {
                getMainController()->getUserPresetHandler().isInternalPresetLoadFlag = prevValue;
            }
            
            
            bool prevValue;
        };
        
#define USE_OLD_AUTOMATION_DISPATCH 0
#define USE_NEW_AUTOMATION_DISPATCH 1

#if USE_OLD_AUTOMATION_DISPATCH
#define IF_OLD_AUTOMATION_DISPATCH(x) x
#define OLD_AUTOMATION_WITH_COMMA(x) x,
#else
#define IF_OLD_AUTOMATION_DISPATCH(x)
#define OLD_AUTOMATION_WITH_COMMA(x)
#endif

#if USE_NEW_AUTOMATION_DISPATCH
#define IF_NEW_AUTOMATION_DISPATCH(x) x
#define NEW_AUTOMATION_WITH_COMMA(x) x,
#else
#define IF_NEW_AUTOMATION_DISPATCH(x)
#define NEW_AUTOMATION_WITH_COMMA(x)
#endif

		struct CustomAutomationData : public ReferenceCountedObject,
			NEW_AUTOMATION_WITH_COMMA(public dispatch::SourceOwner)
									  public ControlledObject
		{
			using WeakPtr = WeakReference<CustomAutomationData>;
			using Ptr = ReferenceCountedObjectPtr<CustomAutomationData>;
			using List = ReferenceCountedArray<CustomAutomationData>;

			CustomAutomationData(CustomAutomationData::List newList, MainController* mc, int index_, const var& d);

			void updateFromConnectionValue(int preferredIndex);

			bool isConnectedToMidi() const;
			bool isConnectedToComponent() const;

			const int index;
			String id;
			float lastValue = 0.0f;
			bool allowMidi = true;
			bool allowHost = true;
			NormalisableRange<float> range;
			Result r;
			var args[2];

#if USE_OLD_AUTOMATION_DISPATCH
			LambdaBroadcaster<var*> syncListeners;
			LambdaBroadcaster<int, float> asyncListeners;
#endif
			IF_NEW_AUTOMATION_DISPATCH(dispatch::library::CustomAutomationSource dispatcher);

			struct ConnectionBase: public ReferenceCountedObject
			{
				using Ptr = ReferenceCountedObjectPtr<ConnectionBase>;
				using List = ReferenceCountedArray<ConnectionBase>;
				
				virtual ~ConnectionBase() {};
				
				virtual bool isValid() const = 0;
				virtual void call(float v, dispatch::DispatchType n) const = 0;

				virtual String getDisplayString() const = 0;

				virtual float getLastValue() const = 0;

				operator bool() const { return isValid(); }
			};

			void call(float newValue, dispatch::DispatchType n, const std::function<bool(ConnectionBase*)>& connectionFilter = {});

			ConnectionBase::Ptr parse(CustomAutomationData::List newList, MainController* mc, const var& jsonData);

			struct MetaConnection : public ConnectionBase
			{
				void call(float v, dispatch::DispatchType n) const final override
				{
					target->call(v, n);
				}

				bool isValid() const final override
				{
					return target != nullptr;
				}

				String getDisplayString() const override
				{
					return "Automation: " + target->id;
				}

				float getLastValue() const final override
				{
					if (target != nullptr)
						return target->lastValue;
                    
                    return 0.0;
				}

				CustomAutomationData::Ptr target;
			};

			struct CableConnection;

			struct ProcessorConnection : public ConnectionBase
			{
				WeakReference<Processor> connectedProcessor;
				int connectedParameterIndex = -1;

				bool isValid() const final override
				{
					return connectedProcessor != nullptr && connectedParameterIndex != -1;
				}

				String getDisplayString() const final override;

				float getLastValue() const final override;

				void call(float v, dispatch::DispatchType n) const final override;
			};

			ConnectionBase::List connectionList;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomAutomationData);
			JUCE_DECLARE_WEAK_REFERENCEABLE(CustomAutomationData);
		};

		struct UndoableUserPresetLoad : public ControlledObject,
										public UndoableAction
		{
			UndoableUserPresetLoad(MainController* mc, const File& oldFile_, const File& newFile_, ValueTree newPreset_, ValueTree forcedOld=ValueTree()):
				ControlledObject(mc),
				newPreset(newPreset_),
			    oldFile(oldFile_),
			    newFile(newFile_)
			{
				if (forcedOld.isValid())
					oldPreset = forcedOld;
				else
					oldPreset = UserPresetHelpers::createUserPreset(mc->getMainSynthChain());
			}

			bool perform() override
			{
				getMainController()->getUserPresetHandler().loadUserPresetFromValueTree(newPreset, oldFile, newFile, false);
				return true;
			}

			bool undo() override
			{
				getMainController()->getUserPresetHandler().loadUserPresetFromValueTree(oldPreset, newFile, oldFile, false);
				return true;
			}

			UndoableAction* createCoalescedAction(UndoableAction* nextAction) override
			{
				if (auto upAction = dynamic_cast<UndoableUserPresetLoad*>(nextAction))
					return new UndoableUserPresetLoad(getMainController(), oldFile, upAction->newFile, upAction->newPreset, oldPreset);

				return nullptr;
			}

			ValueTree oldPreset;
			ValueTree newPreset;
			File oldFile;
			File newFile;
		};

		struct TagDataBase
		{
			struct CachedTag
			{
				int64 hashCode;
				Array<Identifier> tags;
				bool shown = false;
			};

			void setRootDirectory(const File& newRoot);;

			void buildDataBase(bool force = false);

			/** If you want to use the tag system, supply a list of Strings and it will
			create the tags automatically.
			*/
			void setTagList(const StringArray& newTagList)
			{
				tagList = newTagList;
			}

			/** @internal */
			const StringArray& getTagList() const { return tagList; }

			const Array<CachedTag>& getCachedTags() const { return cachedTags; }
			Array<CachedTag>& getCachedTags() { return cachedTags; }

		private:

			StringArray tagList;

			File root;

			Array<CachedTag> cachedTags;

			void buildInternal();

			bool dirty = true;
		};

		/** A class that will be notified about user preset changes. */
		class Listener
		{
		public:

			virtual ~Listener() {};

			/** Called on the message thread whenever the new preset was loaded. */
			virtual void presetChanged(const File& newPreset) = 0;

			virtual void presetSaved(const File& newPreset) {};

			/** Called whenever the number of presets changed. */
			virtual void presetListUpdated() = 0;

			/** This will be called synchronously just before the new preset is about to be loaded. 

				You can use this method to actually modify the value tree that is being loaded
				so you can implement eg. backward-compatibility migration routines. If you do so,
				make sure to create a deep copy of the valuetree, then return the modified one.
			*/
			virtual ValueTree prePresetLoad(const ValueTree& dataToLoad, const File& fileToLoad) { return dataToLoad; };

			virtual void loadCustomUserPreset(const var& dataObject) {};

			virtual var saveCustomUserPreset(const String& presetName) { return {}; }

		private:

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		UserPresetHandler(MainController* mc_);;

		/** Just loads the next user preset (or the previous one). Use this for "browse buttons". 
		*
		*	@param stayInSameDirectory if true, it will cycle through the current directory
		*                              (otherwise the entire preset list will be used).
		*/
		void incPreset(bool next, bool stayInSameDirectory);

		void loadUserPresetFromValueTree(const ValueTree& v, const File& oldFile, const File& newFile, bool useUndoManagerIfEnabled=true);
		void loadUserPreset(const File& f, bool useUndoManagerIfEnabled=true);

		

		struct DefaultPresetManager: public ControlledObject
		{
			DefaultPresetManager(UserPresetHandler& parent);

			void init(const ValueTree& v);

			void resetToDefault();

			var getDefaultValue(const String& componentId) const;
			var getDefaultValue(int componentIndex) const;

			ValueTree getDefaultPreset() { return defaultPreset; }

		private:

			File defaultFile;
			WeakReference<Processor> interfaceProcessor;
			ValueTree defaultPreset;
		};

		/** Returns the currently loaded file. Can be used to display the user preset name. */
		File getCurrentlyLoadedFile() const;;

		/** @internal */
		//void setCurrentlyLoadedFile(const File& f);

		/** @internal */
		void sendRebuildMessage();

        bool isInternalPresetLoad() const { return isInternalPresetLoadFlag; }
        
		bool isCurrentlyInsidePresetLoad() const { return LockHelpers::getCurrentThreadHandleOrMessageManager() == currentThreadThatIsLoadingPreset; };

		bool isUsingCustomDataModel() const { return customStateManager != nullptr; };
		
		bool isUsingPersistentObject() const { return usePersistentObject; }

		/** Checks if the current preset file (or directory) is marked as read-only.
		
			If READ_ONLY_FACTORY_PRESETS is true, this function will check the file path
			against the ones embedded as factory presets and return true for matches. */
		bool isReadOnly(const File& f);

		/** Saves a preset. 
		*
		*	If you use the MultiColumnPresetBrowser, you won't need to bother about this method,
		*	but for custom implementations, this will save the current UI state to the given file.
		*/
		void savePreset(String presetName = String());

		StringArray getCustomAutomationIds() const;

		int getNumCustomAutomationData() const { return customAutomationData.size(); }

		CustomAutomationData::Ptr getCustomAutomationData(const Identifier& id) const;

		CustomAutomationData::Ptr getCustomAutomationData(int index) const;

		int getCustomAutomationIndex(const Identifier& id) const;

		/** Registers a listener that will be notified about preset changes. */
		void addListener(Listener* listener);

		/** Deregisters a listener. */
		void removeListener(Listener* listener);

		TagDataBase& getTagDataBase() const	{ return tagDataBase.get();}

		void setAllowUndoAtUserPresetLoad(bool shouldAllowUndo)
		{
			useUndoForPresetLoads = shouldAllowUndo;
		}

		void preprocess(ValueTree& presetToLoad);

		void postPresetLoad();

		void postPresetSave();

		bool setCustomAutomationData(CustomAutomationData::List newList);

		void setUseCustomDataModel(bool shouldUseCustomModel, bool usePersistentObject);

		double getSecondsSinceLastPresetLoad() const;

		LambdaBroadcaster<bool> deferredAutomationListener;

		void addStateManager(UserPresetStateManager* newManager);

		void removeStateManager(UserPresetStateManager* managerToRemove);

		bool restoreStateManager(const ValueTree& presetRoot, const Identifier& stateId);
		bool saveStateManager(ValueTree& preset, const Identifier& stateId);

		

		UserPresetStateManager::List stateManagers;

#if READ_ONLY_FACTORY_PRESETS
	private:


		struct FactoryPaths
		{
			bool contains(MainController* mc, const File& f);
			void addRecursive(const ValueTree& v, const String& path);

		private:

			void initialise(MainController* mc);
			String getPath(MainController* mc, const File& f);

			StringArray factoryPaths;
			bool initialised = false;
		};

		SharedResourcePointer<FactoryPaths> factoryPaths;

		public:

		FactoryPaths& getFactoryPaths() { return *factoryPaths; }
#endif

		void initDefaultPresetManager(const ValueTree& defaultState);

		bool getDefaultValueFromPreset(int componentIndex, float& value)
		{
			if (defaultPresetManager->getDefaultPreset().isValid())
			{
				value = defaultPresetManager->getDefaultValue(componentIndex);
				return true;
			}

			return false;
		}

		ScopedPointer<DefaultPresetManager> defaultPresetManager;

		private:

		SharedResourcePointer<TagDataBase> tagDataBase;

		void loadUserPresetInternal();
		void saveUserPresetInternal(const String& name=String());

		Array<WeakReference<Listener>, CriticalSection> listeners;

		File currentlyLoadedFile;
		ValueTree pendingPreset;
		StringArray storedModuleIds;

		MainController* mc;
		bool useUndoForPresetLoads = false;

		

		struct CustomStateManager : public UserPresetStateManager
		{
			CustomStateManager(UserPresetHandler& parent_);

			void resetUserPresetState() override
			{

			};

			Identifier getUserPresetStateId() const override { return UserPresetIds::CustomJSON; };
			
			void restoreFromValueTree(const ValueTree &previouslyExportedState) override;

			ValueTree exportAsValueTree() const override;

			UserPresetHandler& parent;
		};

		ScopedPointer<CustomStateManager> customStateManager;

		bool usePersistentObject = false;
        bool isInternalPresetLoadFlag = false;

		void* currentThreadThatIsLoadingPreset = nullptr;

		CustomAutomationData::List customAutomationData;

		

    private:

		friend class UserPresetHelpers;
		friend class FrontendProcessor;

		uint32 timeOfLastPresetLoad = 0;

        bool processStateManager(bool shouldSave, ValueTree& presetRoot, const Identifier& stateId);
        
		JUCE_DECLARE_WEAK_REFERENCEABLE(UserPresetHandler);
	};


	/** A class that handles the asynchronous adding / removal of Processor objects to the signal path. */
	struct GlobalAsyncModuleHandler
	{
	public:

		GlobalAsyncModuleHandler(MainController* mc_) :
			mc(mc_)
		{};

		/** Asynchronously removes the module. 
		*
		*	It will be deactivated and removed from it's parent on the Sample loading thread
		*	Then it will call all listeners and be finally removed on the message thread.
		*
		*	The removeFunction you pass in must not delete the processor!
		*
		*/
		void removeAsync(Processor* p, const SafeFunctionCall::Function& removeFunction);

		/** Asynchronously adds a module.
		*
		*	It will be activated, initialised and added to the parent on the sample loading thread
		*	like defined in the addFunction
		*
		*	Then in the message thread, it will call all listeners.
		*
		*	The ownership must be transferred in the addFunction
		*/
		void addAsync(Processor* p, const SafeFunctionCall::Function& addFunction);
		
	private:

		enum What
		{
			Delete,
			Add,
			numWhat
		};

		void addPendingUIJob(Processor* p, What what); // what

		MainController * mc;
	};

	// TODO before remove: implement add / remove listeners for dispatch::library::ProcessorHandler
	class ProcessorChangeHandler : public AsyncUpdater
	{
	public:

		ProcessorChangeHandler(MainController* mc_);

		enum class EventType
		{
			ProcessorAdded = 0,
			ProcessorRemoved,
			RebuildModuleList,
			ClearBeforeRebuild,
#if HISE_OLD_PROCESSOR_DISPATCH
			ProcessorRenamed,
			ProcessorColourChange,
			ProcessorBypassed,
#endif
			numEventTypes
		};

		~ProcessorChangeHandler();

		class Listener
			: public dispatch::ListenerOwner
		{
			
		public:
			virtual void moduleListChanged(Processor* processorThatWasChanged, EventType type) = 0;

			virtual ~Listener();

		private:

			friend class WeakReference<Listener>;
			WeakReference<Listener>::Master masterReference;
		};

		void sendProcessorChangeMessage(Processor* changedProcessor, EventType type, bool synchronous = true);

		void handleAsyncUpdate();

		void addProcessorChangeListener(Listener* newListener);

		void removeProcessorChangeListener(Listener* listenerToRemove);

	private:

		MainController* mc;

		Processor* tempProcessor = nullptr;
		EventType tempType = EventType::numEventTypes;

		Array<WeakReference<Listener>, CriticalSection> listeners;
	};

	class CodeHandler: public Dispatchable,
					   private LockfreeAsyncUpdater
	{
	public:

		enum WarningLevel
		{
			Message = 0,
			Error = 1
		};

		CodeHandler(MainController* mc);

		void writeToConsole(const String &t, int warningLevel, const Processor *p, Colour c);

		void clearConsole();

		CodeDocument* getConsoleData() { return &consoleData; }

		Component* getMainConsole() { return mainConsole.getComponent(); }

		void setMainConsole(Console* console);

		void initialise();

	private:

		struct ConsoleMessage
		{
			WarningLevel warningLevel;
			WeakReference<Processor> p;
			String message;
		};

		void handleAsyncUpdate();

		void printPendingMessagesFromQueue();

		MultithreadedLockfreeQueue<ConsoleMessage, MultithreadedQueueHelpers::Configuration::AllocationsAllowedAndTokenlessUsageAllowed> pendingMessages;

		bool initialised = false;

		bool overflowProtection = false;

		bool clearFlag = false;

		CodeDocument consoleData;

		Component::SafePointer<Component> mainConsole;

		MainController* mc;

		JUCE_DECLARE_WEAK_REFERENCEABLE(CodeHandler);

	};

	/** Handles the voice killing when a longer task is about to start. */
	class KillStateHandler :  public AudioThreadGuard::Handler
	{
	public:

		enum IllegalOps
		{
			ProcessorInsertion = IllegalAudioThreadOps::numIllegalOperationTypes,
			ProcessorDestructor,
			ValueTreeOperation,
			SampleCreation,
			SampleDestructor,
			IteratorCreation,
			Compilation,
			GlobalLocking,
			numIllegalOps
		};

		enum class TargetThread
		{
			MessageThread = 0,
			SampleLoadingThread,
			AudioThread,
			AudioExportThread,
			ScriptingThread,
			numTargetThreads,
			UnknownThread,
			Free // This is just to indicate there's no thread in use
		};

		enum QueueProducerFlags
		{
			AudioThreadIsProducer = 0x0001,
			LoadingThreadIsProducer = 0x0010,
			MessageThreadIsProducer = 0x0100,
			ScriptThreadIsProducer = 0x1000,
			AllProducers = 0x1111,
			numConsumerFlags
		};


		KillStateHandler(MainController* mc);

		/** Returns false if there is a pending action somewhere that prevents clickless voice rendering. */
		bool voiceStartIsDisabled() const;

        bool isCurrentlyExporting() const { return threadIds[(int)TargetThread::AudioExportThread] != nullptr; }
        
		/** Call this in the processBlock method and it will check whether voice starts are allowed.
		*
		*	It checks if anything is pending and if yes, voiceStartIsDisabled() will return true for the callback.
		*/
		bool handleKillState();

		/** Replacement for allVoicesKilled. Checks if the audio is running. */
		bool isAudioRunning() const noexcept;
		

		/** Give this method a lambda and a processor and it will call it as soon as all voices are killed.
		*
		*	If the voices are already killed, it will synchronously call the function if this function is called from the given targetThread.
		*	If not, it will kill all functions and execute the function on the specified thread.
		*
		*	It will check whether the processor was deleted before calling the function.
		*/
		bool killVoicesAndCall(Processor* p, const ProcessorFunction& functionToExecuteWhenKilled, TargetThread targetThread);

		bool killVoicesAndWait(int* timeOutMilliSeconds=nullptr);

		/** This can be set by the Internal Preloader. */
		void setSampleLoadingThreadId(void* newId);

		void setScriptingThreadId(void* newId)
		{
			jassert(newId != nullptr);
			jassert(threadIds[(int)TargetThread::ScriptingThread] == nullptr);
			threadIds[(int)TargetThread::ScriptingThread].store(newId);
		}

		TargetThread getCurrentThread() const;

		void addThreadIdToAudioThreadList();

		void removeThreadIdFromAudioThreadList();

		bool test() const noexcept override;

		void warn(int operationType) override;

		void requestQuit();

		bool hasRequestedQuit() const;

		String getOperationName(int operationType) override;

		void enableAudioThreadGuard(bool shouldBeEnabled)
		{
			guardEnabled = shouldBeEnabled;
		}

		Array<MultithreadedQueueHelpers::PublicToken> createPublicTokenList(int producerFlags = AllProducers);

		TargetThread getThreadThatLocks(LockHelpers::Type t)
		{
			return lockStates.threadsForLock[(int)t];
		}

		void setLockForCurrentThread(LockHelpers::Type t, bool lock) const;

		bool currentThreadHoldsLock(LockHelpers::Type t) const noexcept;

		bool initialised() const noexcept;

        bool& getStateLoadFlag() { return stateIsLoading; };
        
		void deinitialise();

		/** Returns true if the current thread can be safely suspended by a call to Thread::sleep().
		*
		*	This will return true only for the sample loading thread and the scripting thread. */
		bool isSuspendableThread() const noexcept;


		bool handleBufferDuringSuspension(AudioSampleBuffer& b);

        void setCurrentExportThread(void* exportThread);

		static LockHelpers::Type getLockTypeForThread(TargetThread t);

		static TargetThread getThreadForLockType(LockHelpers::Type t);

		void initFromProcessCallback()
		{
			addThreadIdToAudioThreadList();

			if (!init)
			{
				if (currentState == WaitingForInitialisation)
					currentState = State::InitialisedButNotActivated;
				
				init = true;
			}
		}

	private:

		friend class SuspendHelpers::ScopedTicket;

		uint16 requestNewTicket();

		bool invalidateTicket(uint16 ticket);

		bool checkForClearance() const noexcept;

		struct LockStates
		{
			LockStates()
			{
				threadsForLock[(int)LockHelpers::Type::MessageLock].store(TargetThread::Free);
				threadsForLock[(int)LockHelpers::Type::AudioLock].store(TargetThread::Free);
				threadsForLock[(int)LockHelpers::Type::SampleLock].store(TargetThread::Free);
				threadsForLock[(int)LockHelpers::Type::IteratorLock].store(TargetThread::Free);
				threadsForLock[(int)LockHelpers::Type::ScriptLock].store(TargetThread::Free);
			}

			std::atomic<TargetThread> threadsForLock[(int)LockHelpers::Type::numLockTypes];
		};

		mutable LockStates lockStates;

		/** Checks if all voices are killed. */

		bool voicesAreKilled() const;

		bool guardEnabled = true;

		void initAudioThreadId();

		bool allowGracefulExit() const noexcept;

		void deferToThread(Processor* p, const ProcessorFunction& f, TargetThread t);

		void quit();

		enum State
		{
			WaitingForInitialisation = 0,
			InitialisedButNotActivated,
			Clear,
			VoiceKill,
			WaitingForClearance,
			Suspended,
			ShutdownSignalReceived,
			PendingShutdown,
			ShutdownComplete,
			numPendingStates
		};

		bool init = false;
        bool stateIsLoading = false;

		UnorderedStack<uint16, 4096> pendingTickets;
		uint16 ticketCounter = 0;

		mutable hise::SimpleReadWriteLock ticketLock;

		std::atomic<State> currentState;

		UnorderedStack<StackTrace<3, 6>, 32> stackTraces;

		MainController* mc;
		std::atomic<void*> threadIds[(int)TargetThread::numTargetThreads];
        hise::SimpleReadWriteLock audioListLock;
		UnorderedStack<void*, 32> audioThreads;
	};

	struct PluginBypassHandler: public PooledUIUpdater::SimpleTimer,
								public ControlledObject
	{
		PluginBypassHandler(MainController* mc):
		  ControlledObject(mc),
		  SimpleTimer(mc->getGlobalUIUpdater())
		{};

		void timerCallback() override;

		void bumpWatchDog();

		uint32 lastProcessBlockTime = 0;
		bool lastBypassFlag = false;
		bool currentBypassState = false;
		bool reactivateOnNextCall = false;
		LambdaBroadcaster<bool> listeners;
	};

	MainController();

	virtual ~MainController();

	void notifyShutdownToRegisteredObjects();

	SampleManager &getSampleManager() noexcept {return *sampleManager; };
	const SampleManager &getSampleManager() const noexcept { return *sampleManager; };

	MacroManager &getMacroManager() noexcept {return macroManager;};
	const MacroManager &getMacroManager() const noexcept {return macroManager;};

	AutoSaver &getAutoSaver() noexcept { return autoSaver; }
	const AutoSaver &getAutoSaver() const noexcept { return autoSaver; }

	PluginBypassHandler& getPluginBypassHandler() noexcept { return bypassHandler; }
	const PluginBypassHandler& getPluginBypassHandler() const noexcept { return bypassHandler; }

	DelayedRenderer& getDelayedRenderer() noexcept { return delayedRenderer; };
	const DelayedRenderer& getDelayedRenderer() const noexcept { return delayedRenderer; };

	UserPresetHandler& getUserPresetHandler() noexcept { return userPresetHandler; };
	const UserPresetHandler& getUserPresetHandler() const noexcept { return userPresetHandler; };

	ModuleStateManager& getModuleStateManager() noexcept { return moduleStateManager; };
	const ModuleStateManager& getModuleStateManager() const noexcept { return moduleStateManager; };

	CodeHandler& getConsoleHandler() noexcept { return codeHandler; };
	const CodeHandler& getConsoleHandler() const noexcept { return codeHandler; };

	ProcessorChangeHandler& getProcessorChangeHandler() noexcept { return processorChangeHandler; }
	const ProcessorChangeHandler& getProcessorChangeHandler() const noexcept { return processorChangeHandler; }

	GlobalAsyncModuleHandler& getGlobalAsyncModuleHandler() { return globalAsyncModuleHandler; }
	const GlobalAsyncModuleHandler& getGlobalAsyncModuleHandler() const { return globalAsyncModuleHandler; }

	ExpansionHandler& getExpansionHandler() noexcept { return expansionHandler; }
	const ExpansionHandler& getExpansionHandler() const noexcept { return expansionHandler; }

	LockFreeDispatcher& getLockFreeDispatcher() noexcept { return lockfreeDispatcher; }
	const LockFreeDispatcher& getLockFreeDispatcher() const noexcept { return lockfreeDispatcher; }

	JavascriptThreadPool& getJavascriptThreadPool() noexcept { return *javascriptThreadPool.get(); }
	const JavascriptThreadPool& getJavascriptThreadPool() const noexcept { return *javascriptThreadPool.get(); }

	PooledUIUpdater* getGlobalUIUpdater() { return &globalUIUpdater; }
	const PooledUIUpdater* getGlobalUIUpdater() const { return &globalUIUpdater; }

	dispatch::RootObject& getRootDispatcher() { return rootDispatcher; }
	const dispatch::RootObject& getRootDispatcher() const { return rootDispatcher; }

	dispatch::library::CustomAutomationSourceManager& getCustomAutomationSourceManager() { return customAutomationSourceManager; }
	const dispatch::library::CustomAutomationSourceManager& getCustomAutomationSourceManager() const { return customAutomationSourceManager; }

	dispatch::library::ProcessorHandler& getProcessorDispatchHandler() { return processorHandler; }
	const dispatch::library::ProcessorHandler& getProcessorDispatchHandler() const { return processorHandler; }

	ProjectDocDatabaseHolder* getProjectDocHolder();
	
	void initProjectDocsWithURL(const String& projectDocURL);

	GlobalHiseLookAndFeel& getGlobalLookAndFeel() const { return *mainLookAndFeel; }

	FileHandlerBase* getActiveFileHandler()
	{
		if (auto exp = getExpansionHandler().getCurrentExpansion())
		{
			return exp;
		}

		return &getCurrentFileHandler();
	}

	const FileHandlerBase* getActiveFileHandler() const
	{
		if (auto exp = getExpansionHandler().getCurrentExpansion())
		{
			return exp;
		}

		return &getCurrentFileHandler();
	}

	AudioPlayHead::CurrentPositionInfo& getPositionInfo() { return lastPosInfo; }

	const FileHandlerBase& getCurrentFileHandler() const
	{
		return getSampleManager().getProjectHandler();
	}

	FileHandlerBase& getCurrentFileHandler()
	{
		return getSampleManager().getProjectHandler();
	}
	
	const AudioSampleBufferPool *getCurrentAudioSampleBufferPool() const 
	{ 
		return &getSampleManager().getProjectHandler().pool->getAudioSampleBufferPool();
	};

	AudioSampleBufferPool *getCurrentAudioSampleBufferPool()
	{
		return &getSampleManager().getProjectHandler().pool->getAudioSampleBufferPool();
	};

	const ImagePool *getCurrentImagePool() const
	{
		return &getSampleManager().getProjectHandler().pool->getImagePool();
	};

	ImagePool *getCurrentImagePool()
	{
		return &getSampleManager().getProjectHandler().pool->getImagePool();
	};

	SampleMapPool* getCurrentSampleMapPool();

	const SampleMapPool* getCurrentSampleMapPool() const;

	MidiFilePool* getCurrentMidiFilePool()
	{
		return &getSampleManager().getProjectHandler().pool->getMidiFilePool();
	}

	const MidiFilePool* getCurrentMidiFilePool() const
	{
		return &getSampleManager().getProjectHandler().pool->getMidiFilePool();
	}

	virtual void* getWorkbenchManager() { return nullptr; }

	KillStateHandler& getKillStateHandler() { return killStateHandler; };
	const KillStateHandler& getKillStateHandler() const { return killStateHandler; };
#if USE_BACKEND
	/** Writes to the console. */
	void writeToConsole(const String &message, int warningLevel, const Processor *p=nullptr, Colour c=Colours::transparentBlack);
#endif

	void loadPresetFromFile(const File &f, Component *mainEditor=nullptr);
	void loadPresetFromValueTree(const ValueTree &v, Component *mainEditor=nullptr);
    void clearPreset(NotificationType n);
    

	/** Compiles all scripts in the main synth chain */
	void compileAllScripts();

	void sendToMidiOut(const HiseEvent& e);

	/** Call this if you want all voices to stop. */
	void allNotesOff(bool resetSoftBypassState=false);;

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

	bool isSyncedToHost() const;

	void handleTransportCallbacks(const AudioPlayHead::CurrentPositionInfo& newInfo, const MasterClock::GridInfo& gi);

	void sendArtificialTransportMessage(bool shouldBeOn);

	/** skins the given component (applies the global look and feel to it). */
    void skin(Component &c);
	
	/** adds a TempoListener to the main controller that will receive a callback whenever the host changes the tempo. */
	void addTempoListener(TempoListener *t);

	/** removes a TempoListener. */
	void removeTempoListener(TempoListener *t);;

	void addMusicalUpdateListener(TempoListener* t);

	void removeMusicalUpdateListener(TempoListener* t);

	MasterClock& getMasterClock() { return masterClock; }
	const MasterClock& getMasterClock() const { return masterClock; }

	ApplicationCommandManager *getCommandManager() { return mainCommandManager; };

	LambdaBroadcaster<double, int>& getSpecBroadcaster() { return specBroadcaster; }
	LambdaBroadcaster<bool>& getNonRealtimeBroadcaster() { return realtimeBroadcaster; }

    const CriticalSection& getLock() const;
    
	const CriticalSection& getLockNew() const { return processLock; };

	AudioProcessor* getAsAudioProcessor() { return dynamic_cast<AudioProcessor*>(this); };
	const AudioProcessor* getAsAudioProcessor() const { return dynamic_cast<const AudioProcessor*>(this); };

	DebugLogger& getDebugLogger() { return debugLogger; }
	const DebugLogger& getDebugLogger() const { return debugLogger; }
    
	void addPreviewListener(BufferPreviewListener* l);

	void removePreviewListener(BufferPreviewListener* l);

	void stopBufferToPlay();

	void setBufferToPlay(const AudioSampleBuffer& buffer, double previewSampleRate, const std::function<void(int)>& previewFunction = {});

	int getPreviewBufferPosition() const;

	int getPreviewBufferSize() const;

    void connectToRuntimeTargets(scriptnode::OpaqueNode& opaqueNode, bool shouldAdd);
    
	void setKeyboardCoulour(int keyNumber, Colour colour);

	CustomKeyboardState &getKeyboardState();

	bool shouldEmbedAllResources() const { return embedAllResources; }

	void setLowestKeyToDisplay(int lowestKeyToDisplay);

	float getVoiceAmountMultiplier() const;

	void rebuildVoiceLimits();

	void timerCallback() override;

#if USE_BACKEND

	void setScriptWatchTable(ScriptWatchTable *table);
	
	void setWatchedScriptProcessor(JavascriptProcessor *p, Component *editor);

	/** Use this and the main controller will ignore all threading issues and just does what it wants until the
		bad babysitter leaves the scope.
	
		This is mostly used for creating objects during documentation generation or other non-critical tasks
		which couldn't care less about race conditions...
	*/
	struct ScopedBadBabysitter
	{
		ScopedBadBabysitter(MainController* mc_):
			mc(mc_),
			prevValue(mc->flakyThreadingAllowed)
		{
			mc->flakyThreadingAllowed = true;
		}
		
		~ScopedBadBabysitter()
		{
			mc->flakyThreadingAllowed = prevValue;
		}

		MainController* mc;
		bool prevValue;
	};

	bool isFlakyThreadingAllowed() const noexcept 
	{ 
		return flakyThreadingAllowed; 
	}

#else

	/** There is no use for a bad babysitter in exported projects so this is just a dummy class. */
	struct ScopedBadBabysitter
	{
		ScopedBadBabysitter(MainController*) {};
	};

	bool isFlakyThreadingAllowed() const noexcept
	{
		return false;
	}

#endif


	void setPlotter(Plotter *p);

	DynamicObject *getGlobalVariableObject() { return globalVariableObject.get(); };

	DynamicObject *getHostInfoObject() { return hostInfo.get(); }

	/** this must be overwritten by the derived class and return the master synth chain. */
	virtual ModulatorSynthChain *getMainSynthChain() = 0;

	virtual const ModulatorSynthChain *getMainSynthChain() const = 0;

	void resetLookAndFeelToDefault(Component* c);

	void setCurrentScriptLookAndFeel(ReferenceCountedObject* newLaf) override;

	bool setMinimumSamplerate(double newMinimumSampleRate)
	{
		minimumSamplerate = jlimit<double>(1.0, 96000.0 * 4.0, newMinimumSampleRate);
		return refreshOversampling();
	}

	void setMaximumBlockSize(int newBlockSize);

	/** Returns the maximum block size that HISE will use for its process callback. 
	
		It defaults to HISE_MAX_PROCESSING_BLOCKSIZE (which is 512) but it can be set with Engine.setMaximumBlockSize()
	*/
	int getMaximumBlockSize() const { return maximumBlockSize; }

	/** Returns the time that the plugin spends in its processBlock method. */
	float getCpuUsage() const {return usagePercent.load();};

	/** Returns the amount of playing voices. */
	int getNumActiveVoices() const;;

	void setLastActiveEditor(Component *editor, CodeDocument::Position position)
	{
		auto old = lastActiveEditor;

		lastActiveEditor = editor;
		lastCharacterPositionOfSelectedEditor = position.getPosition();

		if (old != nullptr)
			old->repaint();

		if (lastActiveEditor != nullptr)
			lastActiveEditor->repaint();
	}

    UndoManager* getLocationUndoManager() { return &locationUndoManager; }
    
	Component* getLastActiveEditor()
	{
		return lastActiveEditor.getComponent();
	}

#if HISE_INCLUDE_RT_NEURAL
	NeuralNetwork::Holder& getNeuralNetworks() { return neuralNetworks; }
#endif

	/** This returns always true after the processor was initialised. */
	bool isInitialised() const noexcept;;

	void insertStringAtLastActiveEditor(const String &string, bool selectArguments);

	void loadTypeFace(const String& fileName, const void* fontData, size_t fontDataSize, const String& fontId=String());

	int getBufferSizeForCurrentBlock() const noexcept;
	
	void fillWithCustomFonts(StringArray &fontList);
	juce::Typeface* getFont(const String &fontName) const;

	float getStringWidthFloat(const Font& f, const String& name)
	{
		return getStringWidthFromEmbeddedFont(name, f.getTypefaceName(), f.getHeight(), f.getExtraKerningFactor());
	}

	float getStringWidthFromEmbeddedFont(const String& text, const String& fontName, float fontSize,
	                                     float kerningFactor);
	

	ValueTree exportCustomFontsAsValueTree() const;
	ValueTree exportAllMarkdownDocsAsValueTree() const;
	void restoreCustomFontValueTree(const ValueTree &v);
	void restoreEmbeddedMarkdownDocs(const ValueTree& v);

	String getEmbeddedMarkdownContent(const String& url) const;

	Font getFontFromString(const String& fontName, float fontSize) const;

	void setGlobalFont(const String& fontName);

	
	void checkAndAbortMessageThreadOperation();

    bool checkAndResetMidiInputFlag();
    bool isChanged() const { return changed; }
    void setChanged(bool shouldBeChanged=true) { changed = shouldBeChanged; }
    
    float getGlobalCodeFontSize() const;;
    
	

	bool isUsingDynamicBufferSize() const
	{
#if USE_BACKEND
		return simulateDynamicBufferSize;
#else
		return false;
#endif
	}

    ReferenceCountedObject* getGlobalPreprocessor();

	bool isInsideAudioRendering() const;

	bool shouldAbortMessageThreadOperation() const noexcept
	{
		return false;
	}

	bool& getDeferNotifyHostFlag() { return deferNotifyHostFlag; }

	LambdaBroadcaster<float> &getFontSizeChangeBroadcaster() { return codeFontChangeNotificator; };
    
	

    /** This sets the global pitch factor. */
    void setGlobalPitchFactor(double pitchFactorInSemiTones)
    {
        globalPitchFactor = pow(2, pitchFactorInSemiTones / 12.0);
    }

	void setGlobalMidiPlaybackSpeed(double newGlobalPlaybackSpeed)
	{
		globalPlaybackSpeed = newGlobalPlaybackSpeed;
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
    
	double getGlobalPlaybackSpeed() const
	{
		return globalPlaybackSpeed;
	}

	bool &getPluginParameterUpdateState() { return enablePluginParameterUpdate; }

	const CriticalSection& getIteratorLock() const { return iteratorLock; }


	EventIdHandler& getEventHandler() { return eventIdHandler; }

	void setSkipCompileAtPresetLoad(bool shouldSkip)
	{
		skipCompilingAtPresetLoad = shouldSkip;
	}

	bool shouldSkipCompiling() const noexcept
	{
		return skipCompilingAtPresetLoad;
	}

	bool isBeingDeleted() const noexcept
	{
		return deletePendingFlag;
	}


	void loadUserPresetAsync(const File& f);

	UndoManager* getControlUndoManager() { return controlUndoManager; }

	void registerControlledObject(ControlledObject* obj) { registeredObjects.add(obj); }

	void removeControlledObject(ControlledObject* obj) { registeredObjects.removeAllInstancesOf(obj); }

	void setAllowSoftBypassRamps(bool shouldBeAllowed)
	{
		allowSoftBypassRamps = shouldBeAllowed;
	}

	bool shouldUseSoftBypassRamps() const noexcept;

	void setCurrentMarkdownPreview(MarkdownContentProcessor* p)
	{
		currentPreview = p;
	}

	void setDefaultPresetHandler(ControlledObject* ownedHandler)
	{
		defaultPresetHandler = ownedHandler;
	}

	ONNXLoader::Ptr getONNXLoader();

	MarkdownContentProcessor* getCurrentMarkdownPreview();

	MultiChannelAudioBuffer::XYZPool* getXYZPool()
	{
		return xyzPool.get();
	}

#if USE_COPY_PROTECTION || USE_BACKEND
	virtual juce::OnlineUnlockStatus* getLicenseUnlocker() = 0;
#endif

#if HISE_INCLUDE_RLOTTIE
	RLottieManager::Ptr getRLottieManager();
#endif

#if HISE_INCLUDE_LORIS
    LorisManager* getLorisManager() { return lorisManager.get(); }
#endif
    
    LambdaBroadcaster<int>& getBlocksizeBroadcaster() { return blocksizeBroadcaster; }
    
    /** sets the new BPM and sends a message to all registered tempo listeners if the tempo changed. */
    void setBpm(double bpm_);
    
private: // Never call this directly, but wrap it through DelayedRenderer...

	// except for the audio rendererbase as this does not need to use the outer interface
	// (in order to avoid messing with the leftover sample logic from misbehFL!avinStudio!!1!g hosts...)
	friend class AudioRendererBase;

	/** This is the main processing loop that is shared among all subclasses. */
	void processBlockCommon(AudioSampleBuffer &b, MidiBuffer &mb);

    

protected:

    /** Sets the sample rate for the cpu meter. */
    void prepareToPlay(double sampleRate_, int samplesPerBlock);
    
	bool deletePendingFlag = false;

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

			keyboardState.allNotesOff(0);
			keyboardState.reset();

			allNotesOffFlag = false;
		}
	};

#if HISE_INCLUDE_LORIS
    LorisManager::Ptr lorisManager;
#endif
    
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
    
	void killAndCallOnAudioThread(const ProcessorFunction& f);

	void killAndCallOnLoadingThread(const ProcessorFunction& f);


    void setSampleOffsetWithinProcessBuffer(int offset)
    {
        offsetWithinProcessBuffer = offset;
    }

	void setMaxEventTimestamp(int newMaxTimestamp)
	{
		maxEventTimestamp = newMaxTimestamp;
	}

#if USE_BACKEND
	bool simulateDynamicBufferSize = false;
#endif

protected:

	const AudioSampleBuffer& getMultiChannelBuffer() const;

private:

	void sendHisePresetLoadMessage(NotificationType n);

	MasterClock masterClock;

	ReferenceCountedObjectPtr<MultiChannelAudioBuffer::XYZPool> xyzPool;

	bool refreshOversampling();

	double getOriginalSamplerate() const { return originalSampleRate; }

	int getOriginalBufferSize() const { return originalBufferSize; }

	int getOversampleFactor() const { return currentOversampleFactor; }
	
	void processMidiOutBuffer(MidiBuffer& mb, int numSamples);


#if HISE_INCLUDE_RLOTTIE
	ScopedPointer<RLottieManager> rLottieManager;
#endif

	LambdaBroadcaster<double, int> specBroadcaster;

    LambdaBroadcaster<int> blocksizeBroadcaster;

	LambdaBroadcaster<bool> realtimeBroadcaster;
    
	Array<WeakReference<ControlledObject>> registeredObjects;

	int maxEventTimestamp = 0;
	bool embedAllResources = false;

	PooledUIUpdater globalUIUpdater;
	dispatch::RootObject rootDispatcher;
	dispatch::library::ProcessorHandler processorHandler;
	dispatch::library::CustomAutomationSourceManager customAutomationSourceManager;

	PluginBypassHandler bypassHandler;

	AudioSampleBuffer previewBuffer;
	double previewBufferIndex = -1.0;
	double previewBufferDelta = 1.0;
	float fadeOutPreviewBufferGain = 1.0f;
	bool fadeOutPreviewBuffer = false;

	bool flakyThreadingAllowed = false;

	bool allowSoftBypassRamps = true;

	// Apparrently calling ScriptComponent::changed() in order to update plugin parameters does not
	// work if the notifyParameter() function is not deferred properly...
	bool deferNotifyHostFlag = false;

	void loadPresetInternal(const ValueTree& v);

	CriticalSection processLock;

	// This lock should be acquired when you add a new processor to the processing chain
	// or when an iterator is created.
	// You must not acquire this on the audio thread
	CriticalSection iteratorLock;

	ScopedPointer<UndoManager> controlUndoManager;

	ScopedPointer<JavascriptThreadPool> javascriptThreadPool;

	friend class UserPresetHandler;
    friend class PresetLoadingThread;
	friend class DelayedRenderer;
	friend class CodeHandler;

	DelayedRenderer delayedRenderer;
	CodeHandler codeHandler;

	bool skipCompilingAtPresetLoad = false;

	UnorderedStack<HiseEvent> suspendedNoteOns;

	HiseEventBuffer masterEventBuffer;
	SimpleReadWriteLock midiOutputLock;
	HiseEventBuffer outputMidiBuffer;
	EventIdHandler eventIdHandler;
	LockFreeDispatcher lockfreeDispatcher;
	ModuleStateManager moduleStateManager;
	UserPresetHandler userPresetHandler;
	ProcessorChangeHandler processorChangeHandler;
	GlobalAsyncModuleHandler globalAsyncModuleHandler;
	
	void storePlayheadIntoDynamicObject(AudioPlayHead::CurrentPositionInfo &lastPosInfo);

	CustomKeyboardState keyboardState;

	AudioSampleBuffer multiChannelBuffer;

	friend class WeakReference<MainController>;
	WeakReference<MainController>::Master masterReference;

	struct CustomTypeFace
	{
		CustomTypeFace(ReferenceCountedObjectPtr<juce::Typeface> tf, Identifier id_);;

		float getStringWidthFloat(const String& text, float fontSize, float kerning) const
		{
			auto pos = text.begin();
			auto end = text.end();

			float normalisedLength = 0.0f;

			while(pos != end)
			{
				auto c = *pos++;
				normalisedLength += characterWidths[jlimit<uint8>(31, 128, c)];
				normalisedLength += kerning;
			}

			return normalisedLength * fontSize;
		}

		ReferenceCountedObjectPtr<juce::Typeface> typeface;
		Identifier id;

		float characterWidths[128];
	};

	ScopedPointer<juce::dsp::Oversampling<float>> oversampler;
	double minimumSamplerate = 0.0;
	int maximumBlockSize = HISE_MAX_PROCESSING_BLOCKSIZE;
	int currentOversampleFactor = 1;

	int originalBufferSize = 0;
	double originalSampleRate = 0.0;
	
	Array<CustomTypeFace> customTypeFaces;

	CustomTypeFace defaultFont;
	ValueTree customTypeFaceData;
	ValueTree embeddedMarkdownDocs;

	DynamicObject::Ptr globalVariableObject;
	DynamicObject::Ptr hostInfo;
	
	ReadWriteLock compileLock;

	ScopedPointer<ProjectDocDatabaseHolder> projectDocHolder;
	WeakReference<MarkdownContentProcessor> currentPreview;

    ReferenceCountedObjectPtr<ReferenceCountedObject> preprocessor;
    
	ScopedPointer<SampleManager> sampleManager;
	ExpansionHandler expansionHandler;
	
	std::function<void(int)> previewFunction;
	MacroManager macroManager;

	KillStateHandler killStateHandler;

	Component::SafePointer<Plotter> plotter;

	Atomic<int> processingBufferSize;

	Atomic<int> cpuBufferSize;

	int numSamplesThisBlock = 0;

	Atomic<int> presetLoadRampFlag;

	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	double globalPlaybackSpeed = 1.0;

	double fallbackBpm = -1.0;
	double* internalBpmPointer = &fallbackBpm;

    int offsetWithinProcessBuffer = 0;
    
	ScopedPointer<ApplicationCommandManager> mainCommandManager;

	ScopedPointer<GlobalHiseLookAndFeel> mainLookAndFeel;

	Font globalFont;

    UndoManager locationUndoManager;
	Component::SafePointer<Component> lastActiveEditor;
	int lastCharacterPositionOfSelectedEditor;

    LambdaBroadcaster<float> codeFontChangeNotificator;
        
	WeakReference<Console> console;

    ScopedPointer<ConsoleLogger> logger;
    
	WeakReference<Console> popupConsole;
	bool usePopupConsole;

	AutoSaver autoSaver;

	DebugLogger debugLogger;

	hise::ONNXLoader::Ptr onnxLoader;

#if USE_BACKEND
	Component::SafePointer<ScriptWatchTable> scriptWatchTable;
	Array<Component::SafePointer<ScriptComponentEditPanel>> scriptComponentEditPanels;
#else
	const ScriptComponentEditPanel *scriptComponentEditPanel;
	const ScriptWatchTable *scriptWatchTable;
#endif

    AudioProcessor* thisAsProcessor;
    
	Array<WeakReference<BufferPreviewListener>> previewListeners;
	Array<WeakReference<TempoListener>> tempoListeners;

	Array<WeakReference<TempoListener>> pulseListener;

    std::atomic<float> usagePercent;

	bool enablePluginParameterUpdate = true;

    double globalPitchFactor;

	std::pair<bool, Thread::ThreadID> currentlyRenderingThread;

    std::atomic<double> bpm;

	std::atomic<bool> hostIsPlaying;

	Atomic<int> voiceAmount;
	bool allNotesOffFlag;
    bool changed;
    bool midiInputFlag;

#if HISE_INCLUDE_RT_NEURAL
	NeuralNetwork::Holder neuralNetworks;
#endif

	double processingSampleRate = 0.0;
    std::atomic<double> temp_usage;
	int scrollY;
	BigInteger shownComponents;

#if PERFETTO
    std::unique_ptr<perfetto::TracingSession> tracingSession;
#endif

    // Make sure that this is alive all the time...
    snex::cppgen::CustomNodeProperties data;
    
	ScopedPointer<ControlledObject> defaultPresetHandler;

	void handleSuspendedNoteOffs();
public:
	void updateMultiChannelBuffer(int numNewChannels);
};


} // namespace hise

#endif
