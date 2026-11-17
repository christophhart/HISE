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

	virtual ~ProcessorWithScriptingContent();

	struct InterfaceQueryFunction: public ModulationDisplayValue::QueryFunction
	{
		InterfaceQueryFunction(Processor* p, ModulationDisplayValue::QueryFunction::Ptr targetFunction_):
		  safeP(p),
		  targetFunction(targetFunction_)
		{};

		bool onScaleDrag(Processor* p, bool isDown, float delta) override
		{
			jassert(targetFunction != nullptr);

			if(safeP != nullptr)
				return targetFunction->onScaleDrag(safeP.get(), isDown, delta);

			return false;
		}

		ModulationDisplayValue getDisplayValue(Processor* p, double nv, NormalisableRange<double> nr, int displayIndex) const override
		{
			jassert(targetFunction != nullptr);

			if(safeP != nullptr)
				return targetFunction->getDisplayValue(safeP, nv, nr, displayIndex);

			return {};
		}

		WeakReference<Processor> safeP;
		ModulationDisplayValue::QueryFunction::Ptr targetFunction;
	};

	void setModulationDisplayQueryFunction(int idx, Processor* p, ModulationDisplayValue::QueryFunction::Ptr mv)
	{
		if(mv != nullptr)
			assignedFunctions[idx] = new InterfaceQueryFunction(p, mv);
		else
			assignedFunctions[idx] = nullptr;
	}

	ModulationDisplayValue::QueryFunction::Ptr getAssignedModulationQueryFunction(int parameterIndex) const
	{
		auto f = assignedFunctions.find(parameterIndex);

		if(f != assignedFunctions.end())
			return f->second;

		return {};
	}

	ProcessorMetadata withDynamicScriptParameters(const ProcessorMetadata& pd) const;

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

	ProfileCollection callbackProfile;

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

	std::map<int, ModulationDisplayValue::QueryFunction::Ptr> assignedFunctions;

	void defaultControlCallbackIdle(ScriptingApi::Content::ScriptComponent *component, const var& controllerValue, UriError& r);

	void customControlCallbackIdle(ScriptingApi::Content::ScriptComponent *component, const var& controllerValue, UriError& r);

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
	
	ValueTree exportAsValueTree() const override { ValueTree v = MidiProcessor::exportAsValueTree(); saveContent(v); return v; }
	void restoreFromValueTree(const ValueTree &v) override { MidiProcessor::restoreFromValueTree(v); restoreContent(v); }

	const HiseEvent* getCurrentHiseEvent() const
	{
		return currentEvent;
	}


	int getControlCallbackIndex() const override { return onControl; };

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

	void setFileResult(const UriError& e);

	CodeDocument::Position getLastPosition(CodeDocument& docToLookFor) const;

	void setWatchedFilePosition(CodeDocument::Position& newPos);

	void clearFileWatchers();

	int getNumWatchedFiles() const noexcept;

	File getWatchedFile(int index) const;

	bool isEmbeddedSnippetFile(int index) const;

	CodeDocument& getWatchedFileDocument(int index) const;

	void setCurrentPopup(DocumentWindow *window);

	void deleteAllPopups();

	void showPopupForFile(const File& f, int charNumberToDisplay = 0, int lineNumberToDisplay = -1);

	void showPopupForFile(int index, int charNumberToDisplay=0, int lineNumberToDisplay=-1);

	/** This includes every external script, compresses it and returns a base64 encoded string that can be shared without further dependencies. */
	static ValueTree collectAllScriptFiles(ModulatorSynthChain *synthChainToExport);

	void checkOnFocusGain()
	{
		if(reloader != nullptr)
			reloader->timerCallback();
	}

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

#if USE_BACKEND
#define LOG_RUNTIME_ERROR(r, jp) if (!r.wasOk()) { jp->logRuntimeError(r); }
#else
#define LOG_RUNTIME_ERROR(r, jp)
#endif


/** The base class for modules that can be scripted. 
*	@ingroup processor_interfaces
*	
*/
class JavascriptProcessor :	public FileChangeListener,
							public Dispatchable,
							public UriBreakpoint::Owner,
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

	/** Shared callback splitting and whole-file spans for legacy loads, LSP, and the file watcher. */
	struct MergedCallbackDocument
	{
		String text;
		Array<String> snippets;
		Array<int> starts;

		/** Split without changing any snippet. On failure, the previous result remains usable. */
		Result parse(const JavascriptProcessor& processor, const String& newText);

#if USE_BACKEND
		/** Convert between whole-file and callback-relative CodeDocument character indices. */
		int getSnippetIndex(int wholeFileIndex) const;
		int toSnippetIndex(int snippetIndex, int wholeFileIndex) const;
		int toFileIndex(int snippetIndex, int snippetCharIndex) const;

		/** Convert between LSP zero-based UTF-16 positions and CodeDocument indices. */
		static int toCharacterIndex(const String& content, int line, int utf16Column);
		static std::pair<int, int> toLspPosition(const String& content, int characterIndex);
#endif
	};

#if USE_BACKEND
	/** Apply a complete merged file without compiling; must be called on the message thread. */
	Result applyMergedCallbackDocument(const String& text);
	const MergedCallbackDocument& getMergedCallbackDocument() const { return mergedCallbackDocument; }
	bool ensureMergedCallbackMapping();

	/** Resolved connected or XML-exported callback-file path. */
	File getMergedCallbackFile() const;

	/** An open LSP editor owns this file's contents until its last lease closes. */
	void setMergedCallbackFileOpen(bool isOpen);
#endif

	using ResultFunction = std::function<void(const UriError& result)>;

	enum ScriptContextActions
	{
		SaveScriptFile = 9000,
		LoadScriptFile,
		SaveScriptClipboard,
		LoadScriptClipboard,
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
	};

	template <typename T> static ProcessorMetadata withScriptnodeMetadata(const ProcessorMetadata& md)
	{
		return md.asDynamic()
			.withId(T::getClassType())
			.withPrettyName(T::getClassName())
			.template withType<T>()
			.template withInterface<T>()
			.template withInterface<HotswappableProcessor>()
			.withComplexDataInterface(ExternalData::DataType::Table)
			.withComplexDataInterface(ExternalData::DataType::SliderPack)
			.withComplexDataInterface(ExternalData::DataType::AudioFile)
			.withComplexDataInterface(ExternalData::DataType::DisplayBuffer);
	}

	ValueTree createApiTree() override;

	void addPopupMenuItems(PopupMenu &m, Component* c, const MouseEvent &e) override;

#if USE_BACKEND
	bool performPopupMenuAction(int menuId, Component* c) override;
#endif

	SET_PROCESSOR_CONNECTOR_TYPE_ID("ScriptProcessor");


#if USE_BACKEND
	bool handleKeyPress(const KeyPress& k, Component* c) override;
#endif

	bool jumpToDefinition(const UriLocation::WithExpression& locationAndToken) override;

	void setActiveEditor(JavascriptCodeEditor* e, CodeDocument::Position pos) override;

	

	int getCodeFontSize() const override;

	String getUriProcessorId() const override { return dynamic_cast<const Processor*>(this)->getId(); }

	std::pair<bool, String> getUriContent(const String& uri) const override;

	JavascriptCodeEditor* getActiveEditor() override;

	void addInplaceDebugValue(const UriLocation& loc, const String& value, DebugInformationBase::Ptr info);

	Array<std::pair<String, mcl::LanguageManager::InplaceDebugValue::Ptr>> deferredValues;
    mcl::LanguageManager::InplaceDebugValue::List inplaceValues;
    LambdaBroadcaster<Identifier, int> inplaceBroadcaster;
	
	virtual void fileChanged() override;

	void compileScript(const ResultFunction& f = ResultFunction());

	struct ErrorProvider : public ApiProviderBase,
						   public UriErrorLogger,
						   public ApiProviderBase::Holder
	{
		ErrorProvider(JavascriptProcessor& jp) :
			parent(jp)
		{};

		void handleAsyncUpdate() override
		{
			rebuild();
		}

		String getUriProcessorId() const override
		{
			return parent.getUriProcessorId();
		}

		std::pair<bool, String> getUriContent(const String& uri) const override
		{
			return parent.getUriContent(uri);
		}

		ApiProviderBase* getProviderBase() override
		{
			return this;
		}

		void getColourAndLetterForType(int type, Colour& colour, char& letter);

		int getNumDebugObjects() const override;

		DebugInformationBase::Ptr getDebugInformation(int index) override;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ErrorProvider);

		JavascriptProcessor& parent;

		std::pair<UriError, int> currentBreakpoint;
		std::vector<std::pair<UriError, int>> currentBlinkEvents;
		UriLocationTree::Ptr currentIncludeTree;
	} errorProvider;

#if USE_BACKEND
	using DiagnosticList = Array<UriError>;
	using DiagnosticCallback = std::function<void(const DiagnosticList&)>;

	bool isShadowParseReady() const { return engineInitialised && scriptEngine != nullptr; }

	DiagnosticBase::ShadowParseData createFullShadowParseData();

	/** Shadow-parse a file in diagnostic mode.
	    sendNotificationAsync (default): defers to the scripting thread via killVoicesAndCall,
	    callback receives diagnostics on the message thread. Used by IDE (F7).
	    sendNotificationSync: executes directly on the calling thread with a read lock
	    on lookAndFeelRenderLock, callback invoked inline. Used by REST API. */
	void shadowParse(const DiagnosticBase::ShadowParseData& sd, const DiagnosticCallback& callback,
						 NotificationType notificationType = sendNotificationAsync);

	void onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, AttachState newAttachState) override;
	
	void onUriBreakpointChange(const UriBreakpoint& bp, UriBreakpointListener::ChangeType changeType) override 
	{
		getScriptEngine()->setRuntimeBreakpoint(bp, changeType);
	}

	std::pair<AttachState, UriBreakpoint> attachBreakpoint(const UriBreakpoint& bp) override;

	/** Override this method and implement moving the instruction pointer to the given location.

		If the instruction marker needs to be moved to another location (eg. clipped to the current
		function, return the actual location of the new pointer.
	*/
	UriLocation moveInstructionPointer(const UriBreakpoint& newLocation) override
	{
		return getScriptEngine()->moveInstructionPointer(newLocation);
	}

	/** Override this method and implement the logic that handles the breakpoint suspension. */
	void stopOnBreakpoint(const UriError& e) override;

	/** Override this method and implement the logic that handles the resuming of the execution. */
	void resumeExecution(NextBreakpointAction n) override;

	void onUriBreakpointEvent(const UriBreakpoint& resolvedBreakpoint, const UriError& breakpointEvent) override;

#endif

	void setupApi();

	virtual void registerApiClasses() = 0;
	void registerCallbacks();

	virtual SnippetDocument *getSnippet(int c) = 0;
	virtual const SnippetDocument *getSnippet(int c) const = 0;
	virtual int getNumSnippets() const = 0;

	std::pair<UriLocationTree::Ptr, UriLocationTree::Ptr> getIncludeAndScopeTree();

	const CodeDocument* getSnippetOrExternalFile(const UriLocation& location) const
	{
		if (location.getUriType() == UriLocation::UriType::ExternalFile)
		{
			for (int i = 0; i < getNumWatchedFiles(); i++)
			{
				if(location.matchesFile(getWatchedFile(i)))
					return &getWatchedFileDocument(i);
			}
		}

		if(auto sn = getSnippet(Identifier(location.toLocationString(UriLocation::UriFormat::CallbackId))))
			return sn;

		return nullptr;
	}

	SnippetDocument *getSnippet(const Identifier& id);
	const SnippetDocument *getSnippet(const Identifier& id) const;

	/** Returns the code document for a debug location (handles callbacks and external files). */
	CodeDocument* getSnippet(const UriLocation& loc);

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

	const UriError& getLastErrorMessage() const;

	ApiProviderBase* getProviderBase() override;

	DebugSession* getDebugSession() override;

	HiseJavascriptEngine *getScriptEngine();

	void logRuntimeError(const UriError& r);

	void mergeCallbacksToScript(String &x, const String& sepString=String()) const;
	bool parseSnippetsFromString(const String &x, bool clearUndoHistory = false);

	void compileScriptWithCycleReferenceCheckEnabled();

	void stuffAfterCompilation(const UriError& r);

	void showPopupForCallback(const Identifier& callback, int charNumber, int lineNumber);

	void cleanupEngine();

	void setCallStackEnabled(bool shouldBeEnabled);

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

#if USE_BACKEND
	/** Per-script override set by #strict/#warn/#unsafe preprocessor directive.
	    Unset = use global setting. */
	WeakCallbackHolder::CallableObject::StrictnessLevel callScopeOverride =
		WeakCallbackHolder::CallableObject::StrictnessLevel::Unset;

	/** Returns the effective strictness for this processor.
	    Per-script override first, falls back to global HISE setting. */
	WeakCallbackHolder::CallableObject::StrictnessLevel getStrictnessLevel() const;
#endif

	
	

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

		UriError result;

	private:

		AlertWindowLookAndFeel alaf;
		JavascriptProcessor *sp;
	};

	// ================================================================================================================

	UriError lastResult;

	virtual UriError compileInternal();

	String connectedFileReference;

	ScopedPointer<HiseJavascriptEngine> scriptEngine;

	bool lastCompileWasOK;
	bool useStoredContentData = false;
	ProfileCollection compileProfile;
	ProfileCollection::ID pCompileScript, pCreateDebugInfo, pControlCallback;

	

private:

	Array<PreprocessorFunction> preprocessorFunctions;
	friend class ProjectImporter;
	friend class ImporterBase;
	struct Helpers
	{
		struct ExternalScript
		{
			File f;
			String content;
		};

		

		static String resolveIncludeStatements(String& x, Array<File>& includedFiles, const JavascriptProcessor* p);
		static Array<ExternalScript> desolveIncludeStatements(String& x, const File& scriptRoot, MainController* mc);

		
		

	};

	UriLocationTree::Ptr lastIncludeTree;
	UriLocationTree::Ptr lastScopeTree;

#if USE_BACKEND
	struct MergedCallbackReloader : public Timer
	{
		explicit MergedCallbackReloader(JavascriptProcessor& owner);
		void timerCallback() override;
		JavascriptProcessor& owner;
		File lastFile;
		String lastDiskText;
		bool hasBaseline = false;
		bool sawMissingFile = false;
		bool lspOpen = false;
	};

	MergedCallbackDocument mergedCallbackDocument;
	MergedCallbackReloader mergedCallbackReloader { *this };
#endif

	void initialiseEngine();

	bool engineInitialised = false;

	UpdateDispatcher repaintDispatcher;

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
	virtual bool sleepStateChanged(const UriError& location, bool isSleeping) = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptSleepListener);
};

class JavascriptThreadPool : public Thread,
							 public ProfiledRecordingSession,
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

		using Function = std::function<UriError(JavascriptProcessor*)>;

		Task() noexcept;;

		Task(Type t, JavascriptProcessor* jp_, const Function& functionToExecute) noexcept;

		JavascriptProcessor* getProcessor() const noexcept;;
		
		Type getType() const noexcept;

		UriError callWithResult();

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

	void resume(UriBreakpoint::Owner::NextBreakpointAction stepToNext);

	void deactivateSleepUntilCompilation();

	struct ScopedCustomOnBreakFunction
	{
		ScopedCustomOnBreakFunction(JavascriptThreadPool& pool, const std::function<void()>& f):
		  p(pool),
		  prevFunction(p.customOnBreakFunction)
		{
			p.customOnBreakFunction = f;
		}

		~ScopedCustomOnBreakFunction()
		{
			p.customOnBreakFunction = prevFunction;
		}

		JavascriptThreadPool& p;
		std::function<void()> prevFunction;
	};

	struct ScopedSleeper
	{
		ScopedSleeper(JavascriptThreadPool& p_, JavascriptProcessor* jp, const UriError& loc);

		void sendMessage(bool on);

		~ScopedSleeper();

		const UriError location;
		JavascriptThreadPool& p;
		JavascriptProcessor* prevJp;
		const bool wasSleeping;
		bool messageSent = false;
	};

	void notifyIfNotInBreakpoint()
	{
		if (!isSleeping)
			notify();
	}

	void addSleepListener(SleepListener* s);;

	void removeSleepListener(SleepListener* s);

	bool  isCurrentlySleeping() const;;

	struct ScopedAsyncCallstack
	{
		ScopedAsyncCallstack(JavascriptThreadPool& t, const WeakCallbackHolder& wh):
		  p(t)
		{
			if (!wh.asyncCallstack.empty())
			{
				pushedSomething = true;
				p.asyncCallstack.push_back(wh.asyncCallstack);
			}
		}

		~ScopedAsyncCallstack()
		{
			if (pushedSomething)
				p.asyncCallstack.pop_back();
		}

		JavascriptThreadPool& p;
		bool pushedSomething = false;
	};

	void attachAsyncCallstack(UriError& event)
	{
		for (auto& csItem : asyncCallstack)
		{
			for(auto& as: csItem)
				event.callstack.push_back({ "async: " + as.first, as.second });
		}
	}

private:

	std::vector<std::vector<UriLocation::WithExpression>> asyncCallstack;
	std::function<void()> customOnBreakFunction;

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
	UriBreakpoint::Owner::NextBreakpointAction stepToNext = UriBreakpoint::Owner::NextBreakpointAction::Resume;
	bool allowSleep = true;

	WeakReference<JavascriptProcessor> currentSleeper;

	ScopedPointer<GlobalServer> globalServer;

	using PendingCompilationList = Array<WeakReference<JavascriptProcessor>>;

	void pushToQueue(const Task::Type& t, JavascriptProcessor* p, const Task::Function& f);

	UriError executeNow(const Task::Type& t, JavascriptProcessor* p, const Task::Function& f);

	UriError executeQueue(const Task::Type& t, PendingCompilationList& pendingCompilations);

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

	ProfileCollection scriptThreadData;
	ProfileCollection::ID pLow, pHigh, pRepaint, pCompile;

	MultithreadedLockfreeQueue<WeakReference<ScriptingApi::Content::ScriptPanel>, queueConfig> deferredPanels;
};


} // namespace hise


