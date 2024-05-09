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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#pragma once

namespace hise { using namespace juce;

class ModulatorSynthGroup;

/** A class that has a content that can be populated with script components. 
*	@ingroup processor_interfaces
*
*	This is tightly coupled with the JavascriptProcessor class (so every JavascriptProcessor must also be derived from this class).
*/
class ProcessorWithScriptingContent: public SuspendableTimer::Manager
{
public:

	ProcessorWithScriptingContent(MainController* mc_);

	enum EditorStates
	{
		contentShown = 0,
		onInitShown,
		numEditorStates
	};

	virtual ~ProcessorWithScriptingContent();;

	void setAllowObjectConstruction(bool shouldBeAllowed);

	bool objectsCanBeCreated() const;

	virtual int getCallbackEditorStateOffset() const;

#if USE_BACKEND

	void toggleSuspension()
	{
		simulatedSuspensionState = !simulatedSuspensionState;
		suspendStateChanged(simulatedSuspensionState);
	}
	
	bool simulatedSuspensionState = false;

#endif

	void suspendStateChanged(bool shouldBeSuspended) override;

	ScriptingApi::Content *getScriptingContent() const;

	Identifier getContentParameterIdentifier(int parameterIndex) const;

	int getContentParameterAmount() const
	{
		return content->getNumComponents();
	}

	int getContentParameterIdentifierIndex(const Identifier& id) const;

	void setControlValue(int index, float newValue);

	float getControlValue(int index) const;

	virtual void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue);

	virtual int getControlCallbackIndex() const = 0;

	virtual int getNumScriptParameters() const;

	var getSavedValue(Identifier name);

	void restoreContent(const ValueTree &restoredState);

	void saveContent(ValueTree &savedState) const;

	MainController* getMainController_();

	const MainController* getMainController_() const;

protected:

	/** Call this from the base class to create the content. */
	void initContent();

	friend class JavascriptProcessor;

	JavascriptProcessor* thisAsJavascriptProcessor = nullptr;

	MainController* mc;

	bool allowObjectConstructors;

	ValueTree restoredContentValues;
	
	ReferenceCountedObjectPtr<ScriptingApi::Content> content;

	//WeakReference<ScriptingApi::Content> content;

	struct ContentParameterHandler: public ScriptParameterHandler
	{
		ContentParameterHandler(ProcessorWithScriptingContent& parent);

		void setParameter(int index, float newValue) final override;

		float getParameter(int index) const final override;

		int getNumParameters() const final override;

		Identifier getParameterId(int parameterIndex) const final override;

		int getParameterIndexForIdentifier(const Identifier& id) const final override
		{
			return p.getContentParameterIdentifierIndex(id);
		}
		
		ProcessorWithScriptingContent& p;
	} contentParameterHandler;

private:

	void defaultControlCallbackIdle(ScriptingApi::Content::ScriptComponent *component, const var& controllerValue, Result& r);

	void customControlCallbackIdle(ScriptingApi::Content::ScriptComponent *component, const var& controllerValue, Result& r);

	JUCE_DECLARE_WEAK_REFERENCEABLE(ProcessorWithScriptingContent);
};


/** This class acts as base class for both ScriptProcessor and HardcodedScriptProcessor.
*
*	It contains all logic that the ScriptingApi objects need in order to work with both types.
*/
class ScriptBaseMidiProcessor: public MidiProcessor,
							   public ProcessorWithScriptingContent
{
public:

	enum Callback
	{
		onInit = 0,
		onNoteOn,
		onNoteOff,
		onController,
		onTimer,
		onControl,
		numCallbacks
	};

	ScriptBaseMidiProcessor(MainController *mc, const String &id);;

	virtual ~ScriptBaseMidiProcessor() { masterReference.clear(); }

	float getAttribute(int index) const override { return getControlValue(index); }
	void setInternalAttribute(int index, float newValue) override { setControlValue(index, newValue); }
	float getDefaultValue(int index) const override;

	ValueTree exportAsValueTree() const override { ValueTree v = MidiProcessor::exportAsValueTree(); saveContent(v); return v; }
	void restoreFromValueTree(const ValueTree &v) override { MidiProcessor::restoreFromValueTree(v); restoreContent(v); }

	const HiseEvent* getCurrentHiseEvent() const
	{
		return currentEvent;
	}


	int getControlCallbackIndex() const override { return onControl; };

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override
	{
		return getContentParameterIdentifier(parameterIndex);
	}

protected:

	WeakReference<ScriptBaseMidiProcessor>::Master masterReference;
    friend class WeakReference<ScriptBaseMidiProcessor>;
	
	HiseEvent* currentEvent;

};



class ExternalScriptFile;

class FileChangeListener
{
public:

	virtual ~FileChangeListener();

	virtual void fileChanged() = 0;

	ExternalScriptFile::Ptr addFileWatcher(const File &file);

	void setFileResult(const File &file, Result r);

	Result getWatchedResult(int index);

	CodeDocument::Position getLastPosition(CodeDocument& docToLookFor) const;

	void setWatchedFilePosition(CodeDocument::Position& newPos);

	void clearFileWatchers();

	int getNumWatchedFiles() const noexcept;

	File getWatchedFile(int index) const;

	bool isEmbeddedSnippetFile(int index) const;

	CodeDocument& getWatchedFileDocument(int index);

	void setCurrentPopup(DocumentWindow *window);

	void deleteAllPopups();

	void showPopupForFile(const File& f, int charNumberToDisplay = 0, int lineNumberToDisplay = -1);

	void showPopupForFile(int index, int charNumberToDisplay=0, int lineNumberToDisplay=-1);

	/** This includes every external script, compresses it and returns a base64 encoded string that can be shared without further dependencies. */
	static ValueTree collectAllScriptFiles(ModulatorSynthChain *synthChainToExport);

private:

	struct ExternalReloader: public Timer
	{
		ExternalReloader(FileChangeListener& p):
		  parent(p)
		{
			startTimer(3000);
		}

		void timerCallback() override;

		FileChangeListener& parent;
	};

	ScopedPointer<ExternalReloader> reloader;

	friend class ExternalScriptFile;
	friend class WeakReference < FileChangeListener > ;

	WeakReference<FileChangeListener>::Master masterReference;

	CodeDocument emptyDoc;

	ReferenceCountedArray<ExternalScriptFile> watchers;

	Array<CodeDocument::Position> lastPositions;

	Array<Component::SafePointer<DocumentWindow>> currentPopups;

	static void addFileContentToValueTree(JavascriptProcessor* jp, ValueTree externalScriptFiles, File scriptFile, ModulatorSynthChain* chainToExport);
};

using namespace snex;







/** The base class for modules that can be scripted. 
*	@ingroup processor_interfaces
*	
*/
class JavascriptProcessor :	public FileChangeListener,
							public HiseJavascriptEngine::Breakpoint::Listener,
							public Dispatchable,
							public ProcessorWithDynamicExternalData,
							public ApiProviderBase::Holder,
							public WeakCallbackHolder::CallableObjectManager,
							public scriptnode::DspNetwork::Holder
{
public:

	// ================================================================================================================

    struct ScopedPreprocessorMerger
    {
        ScopedPreprocessorMerger(MainController* mc);

        ~ScopedPreprocessorMerger();

        Array<WeakReference<JavascriptProcessor>> list;
    };
    
	using PreprocessorFunction = std::function<bool(const Identifier&, String& m)>;

    bool usePreprocessorAtMerge = false;
    
	/** A named document that contains a callback function. */
	class SnippetDocument : public CodeDocument
	{
	public:

		/** Create a snippet document. 
		*
		*	If you want to supply parameters, supply a whitespace separated list as last argument:
		*
		*	SnippetDocument doc("onControl", "component value"); // 2 parameters, 'component' and 'value'.
		*/
		SnippetDocument(const Identifier &callbackName_, const String &parameters=String());

        ~SnippetDocument();

		/** Returns the name of the SnippetDocument. */
		const Identifier &getCallbackName() const;;

		/** Checks if the document contains code. */
		void checkIfScriptActive();

		/** Returns the function text. */
		String getSnippetAsFunction() const;

		/** Checks if the snippet contains any code to execute. This is a very fast operation. */
		bool isSnippetEmpty() const;;

		/** Returns the number of arguments specified in the constructor. */
		int getNumArgs() const;

		void replaceContentAsync(String s, bool shouldBeAsync=true);

		bool isInitialised() const { return pendingNewContent.isEmpty(); }

	private:

		/** This is necessary in order to avoid sending change messages to its Listeners
		*
		*	while compiling on another thread...
		*/
		struct Notifier: public AsyncUpdater
		{
			Notifier(SnippetDocument& parent_);

			void notify();

			void handleAsyncUpdate() override;

			SnippetDocument& parent;
		};

		SpinLock pendingLock;

        Notifier notifier;
		String pendingNewContent;

		Identifier callbackName;
		
		StringArray parameters;
		int numArgs;

		String emptyText;
		bool isActive;
	};
	
	// ================================================================================================================

	/** A Result object that contains the snippet. */
	struct SnippetResult
	{
		SnippetResult(Result r_, int c_);;

		/** the result */
		Result r;

		/** the callback */
		int c;
	};

	using ResultFunction = std::function<void(const SnippetResult& result)>;

	enum ScriptContextActions
	{
		SaveScriptFile = 9000,
		LoadScriptFile,
		SaveScriptClipboard,
		LoadScriptClipboard,

		ClearAllBreakpoints,
		CreateUiFactoryMethod,
		MoveToExternalFile,
		ExportAsCompressedScript,
		ImportCompressedScript,
		JumpToDefinition,
		SearchAndReplace,
		AddCodeBookmark,
		FindAllOccurences,
		AddAutocompleteTemplate,
		ClearAutocompleteTemplates,
	};

	// ================================================================================================================

	JavascriptProcessor(MainController *mc);
	virtual ~JavascriptProcessor();

	struct EditorHelpers
	{
		static void applyChangesFromActiveEditor(JavascriptProcessor* p);

		static JavascriptCodeEditor* getActiveEditor(JavascriptProcessor* p);

		static JavascriptCodeEditor* getActiveEditor(Processor* p);

		static CodeDocument* gotoAndReturnDocumentWithDefinition(Processor* p, DebugableObjectBase* object);
	};

	ValueTree createApiTree() override;

	void addPopupMenuItems(PopupMenu &m, Component* c, const MouseEvent &e) override;

#if USE_BACKEND
	bool performPopupMenuAction(int menuId, Component* c) override;
#endif

	SET_PROCESSOR_CONNECTOR_TYPE_ID("ScriptProcessor");

	void handleBreakpoints(const Identifier& codefile, Graphics& g, Component* c);

	void handleBreakpointClick(const Identifier& codeFile, CodeEditorComponent& ed, const MouseEvent& e);

#if USE_BACKEND
	bool handleKeyPress(const KeyPress& k, Component* c) override;
#endif

	void jumpToDefinition(const String& namespaceId, const String& token) override;

	void setActiveEditor(JavascriptCodeEditor* e, CodeDocument::Position pos) override;

	int getCodeFontSize() const override;


	JavascriptCodeEditor* getActiveEditor() override;

	void breakpointWasHit(int index) override;

	void addInplaceDebugValue(const Identifier& callback, int lineNumber, const String& value);
    
    Array<mcl::LanguageManager::InplaceDebugValue> inplaceValues;
    LambdaBroadcaster<Identifier, int> inplaceBroadcaster;
    
	virtual void fileChanged() override;

	void compileScript(const ResultFunction& f = ResultFunction());

	void setupApi();

	virtual void registerApiClasses() = 0;
	void registerCallbacks();

	virtual SnippetDocument *getSnippet(int c) = 0;
	virtual const SnippetDocument *getSnippet(int c) const = 0;
	virtual int getNumSnippets() const = 0;

	SnippetDocument *getSnippet(const Identifier& id);
	const SnippetDocument *getSnippet(const Identifier& id) const;

	void saveScript(ValueTree &v) const;
	void restoreScript(const ValueTree &v);

	void restoreInterfaceData(ValueTree propertyData);

	String collectScript(bool silent) const;

	String getBase64CompressedScript(bool silent=false) const;

	

	bool restoreBase64CompressedScript(const String &base64compressedScript);

	void setConnectedFile(const String& fileReference, bool compileScriptAfterLoad=true);

	bool isConnectedToExternalFile() const;

	const String& getConnectedFileReference() const;

	void disconnectFromFile();

	void reloadFromFile();

	bool wasLastCompileOK() const;

	Result getLastErrorMessage() const;

	ApiProviderBase* getProviderBase() override;

	HiseJavascriptEngine *getScriptEngine();

	void mergeCallbacksToScript(String &x, const String& sepString=String()) const;
	bool parseSnippetsFromString(const String &x, bool clearUndoHistory = false);

	void setCompileProgress(double progress);

	void compileScriptWithCycleReferenceCheckEnabled();

	void stuffAfterCompilation(const SnippetResult& r);

	void showPopupForCallback(const Identifier& callback, int charNumber, int lineNumber);

	void toggleBreakpoint(const Identifier& snippetId, int lineNumber, int charNumber);

	HiseJavascriptEngine::Breakpoint getBreakpointForLine(const Identifier &id, int lineIndex);

	void getBreakPointsForDisplayedRange(const Identifier& snippetId, Range<int> displayedLineNumbers, Array<int> &lineNumbers);

	bool anyBreakpointsActive() const;

	void removeAllBreakpoints();

	void cleanupEngine();

	void setCallStackEnabled(bool shouldBeEnabled);

	void addBreakpointListener(HiseJavascriptEngine::Breakpoint::Listener* newListener);

	void removeBreakpointListener(HiseJavascriptEngine::Breakpoint::Listener* listenerToRemove);

	void clearPreprocessorFunctions();

	void addPreprocessorFunction(const PreprocessorFunction& pf);

	ScriptingApi::Content* getContent();

	const ScriptingApi::Content* getContent() const;


	void clearContentPropertiesDoc();

	void createUICopyFromDesktop();

	void setDeviceTypeForInterface(int newDevice);

	ValueTree getContentPropertiesForDevice(int deviceIndex=-1);

	bool hasUIDataForDeviceType(int type=-1) const;

	struct AutocompleteTemplate
	{
		String expression;
		Identifier classId;
	};

	Array<AutocompleteTemplate> autoCompleteTemplates;

	MainController* mainController;

	void setOptimisationReport(const String& report);

protected:

	String lastOptimisationReport;

	void clearExternalWindows();

	friend class ProcessorWithScriptingContent;

	/** Overwrite this when you need to do something after the script was recompiled. */
	virtual void postCompileCallback();;

	// ================================================================================================================

	class CompileThread : public ThreadWithProgressWindow
	{
	public:

		CompileThread(JavascriptProcessor *processor);

		void run();

		JavascriptProcessor::SnippetResult result;

	private:

		AlertWindowLookAndFeel alaf;
		JavascriptProcessor *sp;
	};

	// ================================================================================================================

	Result lastResult;

	virtual SnippetResult compileInternal();

	friend class CompileThread;

	String connectedFileReference;

	CompileThread *currentCompileThread;

	ScopedPointer<HiseJavascriptEngine> scriptEngine;

	

	bool lastCompileWasOK;
	bool useStoredContentData = false;

private:

	Array<PreprocessorFunction> preprocessorFunctions;
	friend class ProjectImporter;
	struct Helpers
	{
		struct ExternalScript
		{
			File f;
			String content;
		};

		

		static String resolveIncludeStatements(String& x, Array<File>& includedFiles, const JavascriptProcessor* p);
		static Array<ExternalScript> desolveIncludeStatements(String& x, const File& scriptRoot, MainController* mc);

		static String stripUnusedNamespaces(const String &code, int& counter);
		static String uglify(const String& prettyCode);

	};

	

	UpdateDispatcher repaintDispatcher;

	

	Array<HiseJavascriptEngine::Breakpoint> breakpoints;

	Array<WeakReference<HiseJavascriptEngine::Breakpoint::Listener>> breakpointListeners;

	Array<Component::SafePointer<DocumentWindow>> callbackPopups;

	bool callStackEnabled = false;

	bool cycleReferenceCheckEnabled = false;

	

	ScopedPointer<CodeDocument> contentPropertyDocument;

	ValueTree allInterfaceData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptProcessor);

public:
	
};

struct JavascriptSleepListener
{
	virtual ~JavascriptSleepListener() {};
	virtual void sleepStateChanged(const Identifier& callback, int lineNumber, bool isSleeping) = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptSleepListener);
};

class JavascriptThreadPool : public Thread,
							 public ControlledObject
{
	static constexpr uint64_t ScriptTrackId = 8999;
	static constexpr uint64_t CompilationTrackId = 9000;
	static constexpr uint64_t HighPriorityTrackId = 9001;
	static constexpr uint64_t LowPriorityTrackId = 9002;
	
public:

	using SleepListener = JavascriptSleepListener;

	JavascriptThreadPool(MainController* mc);

	~JavascriptThreadPool();

	void cancelAllJobs(bool stopThread=true);

	class Task
	{
	public:

		enum Type
		{
			Compilation,
            ReplEvaluation,
			HiPriorityCallbackExecution,
			LowPriorityCallbackExecution,
			DeferredPanelRepaintJob,
			Free,
			numTypes
		};

		using Function = std::function < Result(JavascriptProcessor*)>;

		Task() noexcept;;

		Task(Type t, JavascriptProcessor* jp_, const Function& functionToExecute) noexcept;

		JavascriptProcessor* getProcessor() const noexcept;;
		
		Type getType() const noexcept;

		Result callWithResult();

		bool isValid() const noexcept;

		bool isHiPriority() const noexcept;

	private:

		Type type;
		WeakReference<JavascriptProcessor> jp;
		Function f;
	};

	void addJob(Task::Type t, JavascriptProcessor* p, const Task::Function& f);

	void addDeferredPaintJob(ScriptingApi::Content::ScriptPanel* sp);

	void run() override;;

	const CriticalSection& getLock() const noexcept;;

	bool isBusy() const noexcept;

	Task::Type getCurrentTask() const noexcept;

	void killVoicesAndExtendTimeOut(JavascriptProcessor* jp, int milliseconds=1000);

	SimpleReadWriteLock& getLookAndFeelRenderLock();

	GlobalServer* getGlobalServer();

	void resume();

	void deactivateSleepUntilCompilation();

	struct ScopedSleeper
	{
		ScopedSleeper(JavascriptThreadPool& p_, const Identifier& id, int lineNumber_);

		void sendMessage(bool on);

		~ScopedSleeper();

		const Identifier cid;
		const int lineNumber;
		JavascriptThreadPool& p;
		const bool wasSleeping;
	};

	void addSleepListener(SleepListener* s);;

	void removeSleepListener(SleepListener* s);

	bool  isCurrentlySleeping() const;;

private:

	void clearCounter(Task::Type t)
	{
		numTasks[t] = 0;
		TRACE_COUNTER("dispatch", perfetto::CounterTrack(taskNames[t].get()), numTasks[t]);
	}

	void bumpCounter(Task::Type t)
	{
		TRACE_COUNTER("dispatch", perfetto::CounterTrack(taskNames[t].get()), ++numTasks[t]);
	}

	uint16 numTasks[(int)Task::numTypes];
	dispatch::HashedCharPtr taskNames[(int)Task::numTypes];

	Array<WeakReference<SleepListener>> sleepListeners;

	bool isSleeping = false;
	bool shouldWakeUp = false;
	bool allowSleep = true;

	ScopedPointer<GlobalServer> globalServer;

	using PendingCompilationList = Array<WeakReference<JavascriptProcessor>>;

	void pushToQueue(const Task::Type& t, JavascriptProcessor* p, const Task::Function& f);

	Result executeNow(const Task::Type& t, JavascriptProcessor* p, const Task::Function& f);

	Result executeQueue(const Task::Type& t, PendingCompilationList& pendingCompilations);

	std::atomic<bool> pending;
	
	bool busy = false;
	Task::Type currentType;

	CriticalSection scriptLock;

	SimpleReadWriteLock lookAndFeelRenderLock;

	using CompilationTask = SuspendHelpers::Suspended<Task, SuspendHelpers::ScopedTicket>;
	using CallbackTask = SuspendHelpers::Suspended<Task, SuspendHelpers::FreeTicket>;

	using Config = MultithreadedQueueHelpers::Configuration;
	constexpr static Config queueConfig = Config::AllocationsAllowedAndTokenlessUsageAllowed;

	MultithreadedLockfreeQueue<CompilationTask, queueConfig> compilationQueue;
	MultithreadedLockfreeQueue<CallbackTask, queueConfig> lowPriorityQueue;
	MultithreadedLockfreeQueue<CallbackTask, queueConfig> highPriorityQueue;
    
#if USE_BACKEND
    MultithreadedLockfreeQueue<CallbackTask, queueConfig> replQueue;
#endif

	MultithreadedLockfreeQueue<WeakReference<ScriptingApi::Content::ScriptPanel>, queueConfig> deferredPanels;
};


} // namespace hise


