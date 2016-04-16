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

#ifndef SCRIPTPROCESSOR_H_INCLUDED
#define SCRIPTPROCESSOR_H_INCLUDED



class ModulatorSynthGroup;

/** This class acts as base class for both ScriptProcessor and HardcodedScriptProcessor.
*
*	It contains all logic that the ScriptingApi objects need in order to work with both types.
*/
class ScriptBaseProcessor: public MidiProcessor
{
public:

	ScriptBaseProcessor(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		content(nullptr),
		restoredContentValues(ValueTree("Content"))
	{
	};

	virtual ~ScriptBaseProcessor();

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

	virtual void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue) = 0;
	

	bool objectsCanBeCreated() const
	{
		return allowObjectConstructors;
	}

	ScriptingApi::Content *getScriptingContent() const
	{
		return content.get();
	}

	float getAttribute(int index) const override;;

	void setInternalAttribute(int index, float newValue) override;;

	virtual ValueTree exportAsValueTree() const override;

	virtual void restoreFromValueTree(const ValueTree &v) override;

	var getSavedValue(Identifier name)
	{
		return restoredContentValues.getChildWithName(name).getProperty("value", var::undefined());
	}

	

	bool isControlVisible(Identifier name)
	{
		return restoredContentValues.getChildWithName(name).getProperty("visible", true);
	}

	void setScriptProcessorDeactivatedRoundRobin(bool deactivated)
	{
		scriptProcessorDeactivatedRoundRobin = deactivated;
	}

	const MidiMessage &getCurrentMidiMessage() const { return currentMessage; };

protected:

	WeakReference<ScriptBaseProcessor>::Master masterReference;
    friend class WeakReference<ScriptBaseProcessor>;
	

	MidiMessage currentMessage;

	ValueTree restoredContentValues;
	WeakReference<ScriptingApi::Content> content;
	
	bool allowObjectConstructors;

	bool scriptProcessorDeactivatedRoundRobin;

};

class FileWatcher;

class FileChangeListener
{
public:

	virtual ~FileChangeListener();

	virtual void fileChanged() = 0;

	void addFileWatcher(const File &file);

	void setFileResult(const File &file, Result r);

	Result getWatchedResult(int index);

	void clearFileWatchers()
	{
		watchers.clear();
	}

	int getNumWatchedFiles() const noexcept
	{
		return watchers.size();
	}

	File getWatchedFile(int index) const;

	void setCurrentPopup(DocumentWindow *window)
	{
		currentPopups.add(window);
	}

	void showPopupForFile(int index);

	static ValueTree collectAllScriptFiles(ModulatorSynthChain *synthChainToExport);

private:

	friend class FileWatcher;
	friend class WeakReference < FileChangeListener > ;

	WeakReference<FileChangeListener>::Master masterReference;

	OwnedArray<FileWatcher> watchers;

	Array<Component::SafePointer<DocumentWindow>> currentPopups;

};

class FileWatcher : public Timer					
{
public:
	FileWatcher(const File&file, FileChangeListener *listener_):
		fileToWatch(file),
		lastModifiedTime(file.getLastModificationTime()),
		listener(listener_),
		currentResult(Result::ok())
	{
		//startTimer(1000);
	}

	~FileWatcher()
	{
		stopTimer();
		listener = nullptr;
	}

	void timerCallback() override
	{
		if (fileToWatch.getLastModificationTime() != lastModifiedTime)
		{
			if (listener.get() != nullptr)
			{
				listener->fileChanged();
			}
			else stopTimer();
		}
	}

	void setResult(Result r)
	{
		currentResult = r;
	}

	Result getResult() const { return currentResult; }

	File getFile() const { return fileToWatch; }

private:

	WeakReference<FileChangeListener> listener;

	Result currentResult;

	File fileToWatch;
	const Time lastModifiedTime;

};



/** This scripting processor uses the JavaScript Engine to execute small scripts that can change the midi message. 
*	@ingroup midiTypes
*
*	A script should have this function:
*	
*		function onNoteOn()
*		{
*			// do your stuff here
*		}
*
*	You can use the methods from ScriptingApi to change the midi message.
*
*/
class ScriptProcessor: public ScriptBaseProcessor,
					   public Timer,
					   public ExternalFileProcessor,
					   public AsyncUpdater,
					   public FileChangeListener
					
{
	class SnippetDocument;

public:

	SET_PROCESSOR_NAME("ScriptProcessor", "Script Processor")

	struct SnippetResult;

	enum SnippetsOpen
	{
		onInitOpen = Processor::numEditorStates,
		onNoteOnOpen,
		onNoteOffOpen,
		onControllerOpen,
		onTimerOpen,
		onControlOpen,
		contentShown,
		externalPopupShown,
		numScriptEditorStates
	};

	ScriptProcessor(MainController *mc, const String &id);;

	~ScriptProcessor();;

	Path getSpecialSymbol() const override;

	ValueTree exportAsValueTree() const override;;

	virtual void restoreFromValueTree(const ValueTree &v) override;

	virtual void fileChanged() override;

	SnippetDocument *getSnippet(int c);;

	void replaceReferencesWithGlobalFolder() override;

	const SnippetDocument *getSnippet(int c) const;;

	CodeDocument *getDocument() { return doc; };
    
	void setupApi();

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void addToFront(bool addToFront_) noexcept { front = addToFront_;};

	bool isFront() const {return front;};

	StringArray getImageFileNames() const;

	/** This defers the callbacks to the message thread.
	*
	*	It stops all timers and clears any message queues.
	*/
	void deferCallbacks(bool addToFront_);

	bool isDeferred() const {return deferred;};

	void setCompileProgress(double progress)
	{ 
		if (currentCompileThread != nullptr && getMainController()->isUsingBackgroundThreadForCompiling())
		{
			currentCompileThread->setProgress(progress);
		}
	}

	void handleAsyncUpdate();

	void timerCallback() override
	{
		jassert(isDeferred());
		runTimerCallback();
	}

	bool wasLastCompileOK() const { return lastCompileWasOK; }

	void synthTimerCallback(int offsetInBuffer) override
	{
		jassert(!isDeferred());
		runTimerCallback(offsetInBuffer);
	};

	void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue) override;

	/** Checks if any of the properties was changed. */
	ScriptingApi::Content::ScriptComponent *checkContentChangedInPropertyPanel();

	/** Call this when you compile the script. */
	SnippetResult compileScript();

	JavascriptEngine *getScriptEngine()
	{
		return scriptEngine;
	}

	/** This calls the processMidiMessage() function in the compiled Javascript code. @see ScriptingApi */
	void processMidiMessage(MidiMessage &m);
	
	/** Runs the script callbacks.
	*
	*	These are the possible callbacks:
	*
	*		function onNoteOn() {};
	*		function onNoteOff() {};
	*		function onController() {};
	*		function onPitchWheel() {};
	*/
	void runScriptCallbacks();

	double getLastExecutionTime() const noexcept {return lastExecutionTime;};

	struct SnippetResult
	{
		SnippetResult(Result r_, Callback c_): r(r_), c(c_) {};

		/** the result */
		Result r;

		/** the callback */
		Callback c;
	};

	void includeFile(const String &name);

	void mergeCallbacksToScript(String &x) const;;

	void parseSnippetsFromString(const String &x);

	ReferenceCountedObject *textInputisArrayElementObject(const String &t);


	DynamicObject *textInputMatchesScriptingObject(const String &textToShow);

	XmlElement *textInputMatchesApiClass(const String &s) const;

	
	
private:

	class CompileThread : public ThreadWithProgressWindow
	{
	public:

		CompileThread(ScriptProcessor *processor):
			ThreadWithProgressWindow("Compiling", true, false),
			sp(processor),
			result(SnippetResult(Result::ok(), ScriptBaseProcessor::onInit))
		{
			getAlertWindow()->setLookAndFeel(&alaf);
		}

		void run()
		{
			ScopedLock sl(sp->lock);

			result = sp->compileInternal();
		}

		ScriptProcessor::SnippetResult result;

	private:

		AlertWindowLookAndFeel alaf;

		ScriptProcessor *sp;

		
	};

	ScriptProcessor::SnippetResult compileInternal();

	friend class CompileThread;

	CriticalSection lock;

	CriticalSection compileLock;

	void runTimerCallback(int offsetInBuffer=-1);

	ValueTree apiData;

	class SnippetDocument: public CodeDocument
	{
	public:
		SnippetDocument(const Identifier &callbackName_);;

		const Identifier &getCallbackName() const
		{
			return callbackName;
		};

		void checkIfScriptActive()
		{
			isActive = true;

			if( !getAllContent().containsNonWhitespaceChars() ) isActive = false;

			String trimmedText = getAllContent().removeCharacters(" \t\n\r");

			String trimmedEmptyText = emptyText.removeCharacters(" \t\n\r");

			if (trimmedEmptyText == trimmedText)	isActive = false;
		};

		String getSnippetAsFunction() const
		{
			if( isSnippetEmpty()) return emptyText;
			else				  return getAllContent();
		}

		bool isSnippetEmpty() const
		{
			return !isActive;
		};

		void setRange(int start, int end)
		{
			lines = Range<int>(start, end);
		};

		void setEmptyText(const String &t)
		{
			emptyText = t;
			replaceAllContent(t);
		}

		static int addLineToString(String &t, const String &lineToAdd);;

		/** Adds the content of the snippet to the code and returns the number of added lines.
		*
		*	@param t the reference to the string
		*	@param startLine the current line index where the snippet is inserted.
		*/
		int addToString(String &t, int startLine);;

	private:

		Identifier callbackName;

		String emptyText;
		bool isActive;
		Range<int> lines;
	};

	double lastExecutionTime;

	CompileThread *currentCompileThread;
	
	ScopedPointer<CodeDocument> doc;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onNoteOnCallback;
	ScopedPointer<SnippetDocument> onNoteOffCallback;
	ScopedPointer<SnippetDocument> onControllerCallback;
	ScopedPointer<SnippetDocument> onControlCallback;
	ScopedPointer<SnippetDocument> onTimerCallback;

	StringArray includedFileNames;

	MidiBuffer deferredMidiMessages;
	MidiBuffer copyBuffer;

	ScopedPointer<JavascriptEngine> scriptEngine;

	// Only Pointers, since they will be owned by the Javascript Engine

	ScriptingApi::Message *currentMidiMessage;

	ScriptingApi::Engine *engineObject;

	ScriptingApi::Sampler *samplerObject;

	bool front, deferred, deferredUpdatePending;

	ScriptingApi::Synth *synthObject;

	bool lastCompileWasOK;
};




#endif  // SCRIPTPROCESSOR_H_INCLUDED
