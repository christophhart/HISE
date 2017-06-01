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


/** A class that has a content that can be populated with script components. 
*	@ingroup processor_interfaces
*
*	This is tightly coupled with the JavascriptProcessor class (so every JavascriptProcessor must also be derived from this class).
*/
class ProcessorWithScriptingContent
{
public:

	ProcessorWithScriptingContent(MainController* mc_) :
		content(nullptr),
		restoredContentValues(ValueTree("Content")),
		mc(mc_)
	{}

	enum EditorStates
	{
		contentShown = 0,
		onInitShown,
		numEditorStates
	};

	virtual ~ProcessorWithScriptingContent() {};

	bool objectsCanBeCreated() const
	{
		return allowObjectConstructors;
	}

	virtual int getCallbackEditorStateOffset() const { return Processor::EditorState::numEditorStates; }

	ScriptingApi::Content *getScriptingContent() const
	{
		return content.get();
	}

	void setControlValue(int index, float newValue);

	float getControlValue(int index) const;

	virtual void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue);

	virtual int getControlCallbackIndex() const = 0;

	var getSavedValue(Identifier name)
	{
		return restoredContentValues.getChildWithName(name).getProperty("value", var::undefined());
	}

	/** Checks if any of the properties was changed. */
	ScriptingApi::Content::ScriptComponent *checkContentChangedInPropertyPanel();

	void restoreContent(const ValueTree &restoredState);

	void saveContent(ValueTree &savedState) const;

	MainController* getMainController_() { return mc; }

	const MainController* getMainController_() const { return mc; }

protected:

	friend class JavascriptProcessor;

	JavascriptProcessor* thisAsJavascriptProcessor = nullptr;

	MainController* mc;

	bool allowObjectConstructors;

	ValueTree restoredContentValues;
	WeakReference<ScriptingApi::Content> content;
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

	ScriptBaseMidiProcessor(MainController *mc, const String &id): 
		MidiProcessor(mc, id), 
		ProcessorWithScriptingContent(mc),
		currentEvent(nullptr)
	{};

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

protected:

	WeakReference<ScriptBaseMidiProcessor>::Master masterReference;
    friend class WeakReference<ScriptBaseMidiProcessor>;
	
	HiseEvent* currentEvent;

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

	void deleteAllPopups()
	{
		if (currentPopups.size() != 0)
		{
			for (int i = 0; i < currentPopups.size(); i++)
			{
				if (currentPopups[i].getComponent() != nullptr)
				{
					currentPopups[i]->closeButtonPressed();
				}

			}

			currentPopups.clear();
		}
	}

	void showPopupForFile(int index, int charNumberToDisplay=0, int lineNumberToDisplay=-1);

	static ValueTree collectAllScriptFiles(ModulatorSynthChain *synthChainToExport);

	/** This includes every external script, compresses it and returns a base64 encoded string that can be shared without further dependencies. */

	

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



/** The base class for modules that can be scripted. 
*	@ingroup processor_interfaces
*	
*/
class JavascriptProcessor :	public FileChangeListener,
							public HiseJavascriptEngine::Breakpoint::Listener
{
public:

	// ================================================================================================================

	/** A named document that contains a callback function. */
	class SnippetDocument : public CodeDocument
	{
	public:

		

		/** Create a snippet document. 
		*
		*	If you want to supply parameters, supply a whitespace separated list as last argument:
		*
		*	SnippetDocument doc("onControl", "widget value"); // 2 parameters, 'widget' and 'value'.
		*/
		SnippetDocument(const Identifier &callbackName_, const String &parameters=String());

		/** Returns the name of the SnippetDocument. */
		const Identifier &getCallbackName() const { return callbackName; };

		/** Checks if the document contains code. */
		void checkIfScriptActive();

		/** Returns the function text. */
		String getSnippetAsFunction() const;

		/** Checks if the snippet contains any code to execute. This is a very fast operation. */
		bool isSnippetEmpty() const { return !isActive; };

		/** Returns the number of arguments specified in the constructor. */
		int getNumArgs() const { return numArgs; }

	private:

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
		SnippetResult(Result r_, int c_) : r(r_), c(c_) {};

		/** the result */
		Result r;

		/** the callback */
		int c;
	};

	// ================================================================================================================

	JavascriptProcessor(MainController *mc);
	virtual ~JavascriptProcessor();

	SET_PROCESSOR_CONNECTOR_TYPE_ID("JavascriptProcessor");

	void breakpointWasHit(int index) override
	{
		for (int i = 0; i < breakpoints.size(); i++)
		{
			breakpoints.getReference(i).hit = (i == index);
		}

		repaintUpdater.triggerAsyncUpdate();
	}

	void addEditor(Component* editor)
	{
		repaintUpdater.editors.add(editor);
	}

	void removeEditor(Component* editor)
	{
		repaintUpdater.editors.removeAllInstancesOf(editor);
	}

	virtual void fileChanged() override;

	SnippetResult compileScript();

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

	String getBase64CompressedScript() const;

	bool restoreBase64CompressedScript(const String &base64compressedScript);

	void setConnectedFile(const String& fileReference, bool compileScriptAfterLoad=true);

	bool isConnectedToExternalFile() const { return connectedFileReference.isNotEmpty(); }

	const String& getConnectedFileReference() const { return connectedFileReference; }

	void disconnectFromFile();

	void reloadFromFile();

	bool wasLastCompileOK() const { return lastCompileWasOK; }

	Result getLastErrorMessage() const { return lastResult; }

	HiseJavascriptEngine *getScriptEngine() { return scriptEngine; }

	void mergeCallbacksToScript(String &x, const String& sepString=String()) const;
	bool parseSnippetsFromString(const String &x, bool clearUndoHistory = false);

	void setCompileProgress(double progress);

	void showPopupForCallback(const Identifier& callback, int charNumber, int lineNumber);

	void setCompileScriptAsWhole(bool shouldCompileWholeScript) { compileScriptAsWhole = shouldCompileWholeScript; }

	void toggleBreakpoint(const Identifier& snippetId, int lineNumber, int charNumber)
	{
		HiseJavascriptEngine::Breakpoint bp(snippetId, lineNumber, charNumber, breakpoints.size());

		int index = breakpoints.indexOf(bp);

		if (index != -1)
		{
			breakpoints.remove(index);
		}
		else
		{
			breakpoints.add(bp);
		}

		compileScript();
	}

	HiseJavascriptEngine::Breakpoint getBreakpointForLine(const Identifier &id, int lineIndex)
	{
		for (int i = 0; i < breakpoints.size(); i++)
		{
			if (breakpoints[i].snippetId == id && breakpoints[i].lineNumber == lineIndex)
				return breakpoints[i];
		}

		return HiseJavascriptEngine::Breakpoint();
	}

	void getBreakPointsForDisplayedRange(const Identifier& snippetId, Range<int> displayedLineNumbers, Array<int> &lineNumbers)
	{
		for (int i = 0; i < breakpoints.size(); i++)
		{
			if (breakpoints[i].snippetId != snippetId) continue;

			if (displayedLineNumbers.contains(breakpoints[i].lineNumber))
			{
				lineNumbers.add(breakpoints[i].lineNumber);
			}
		}
	}

	bool anyBreakpointsActive() const { return breakpoints.size() != 0; }

	void removeAllBreakpoints()
	{
		breakpoints.clear();

		compileScript();
	}

protected:

	void clearExternalWindows();

	friend class ProcessorWithScriptingContent;

	/** Overwrite this when you need to do something after the script was recompiled. */
	virtual void postCompileCallback() {};

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

	MainController* mainController;

	bool lastCompileWasOK;
	bool useStoredContentData = false;

	bool compileScriptAsWhole = false;

private:

	struct Helpers
	{
		static String resolveIncludeStatements(String& x, Array<File>& includedFiles, const JavascriptProcessor* p);
		static String stripUnusedNamespaces(const String &code, int& counter);
		static String uglify(const String& prettyCode);

	};

	struct RepaintUpdater : public AsyncUpdater
	{
		void handleAsyncUpdate()
		{
			for (int i = 0; i < editors.size(); i++)
			{
				editors[i]->repaint();
			}
		}

		Array<Component::SafePointer<Component>> editors;
	};

	RepaintUpdater repaintUpdater;

	Array<HiseJavascriptEngine::Breakpoint> breakpoints;

	Array<Component::SafePointer<DocumentWindow>> callbackPopups;

	

	
};



#endif  // SCRIPTPROCESSOR_H_INCLUDED
