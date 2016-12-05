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



void ProcessorWithScriptingContent::setControlValue(int index, float newValue)
{

	jassert(content.get() != nullptr);

	if (content != nullptr && index < content->getNumComponents())
	{
		ScriptingApi::Content::ScriptComponent *c = content->getComponent(index);

		if (c != nullptr)
		{
			c->setValue(newValue);

#if USE_FRONTEND
			if (c->isAutomatable() &&
				c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::isPluginParameter) &&
				getMainController_()->getPluginParameterUpdateState())
			{
				dynamic_cast<FrontendProcessor*>(getMainController_())->setScriptedPluginParameter(c->getName(), newValue);
			}
#endif

			controlCallback(c, newValue);
		}
	}
}

float ProcessorWithScriptingContent::getControlValue(int index) const
{
	if (content != nullptr && index < content->getNumComponents())
	{
		return content->getComponent(index)->getValue();

	}

	else return 1.0f;
}

void ProcessorWithScriptingContent::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	JavascriptProcessor* jsp = dynamic_cast<JavascriptProcessor*>(this);
	int callbackIndex = getControlCallbackIndex();

	JavascriptProcessor::SnippetDocument* onControlCallback = jsp->getSnippet(callbackIndex);

	if (onControlCallback->isSnippetEmpty())
	{
		return;
	}

	Processor* thisAsProcessor = dynamic_cast<Processor*>(this);
	
	
	HiseJavascriptEngine* scriptEngine = jsp->getScriptEngine();

	scriptEngine->maximumExecutionTime = RelativeTime(3.0);

	

	ScopedReadLock sl(getMainController_()->getCompileLock());

	scriptEngine->setCallbackParameter(callbackIndex, 0, component);
	scriptEngine->setCallbackParameter(callbackIndex, 1, controllerValue);
	scriptEngine->executeCallback(callbackIndex, &jsp->lastResult);

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		thisAsProcessor->sendSynchronousChangeMessage();
	}
	else
	{
		thisAsProcessor->sendChangeMessage();
	}

	BACKEND_ONLY(if (!jsp->lastResult.wasOk()) debugError(thisAsProcessor, onControlCallback->getCallbackName().toString() + ": " + jsp->lastResult.getErrorMessage()));
}

void ProcessorWithScriptingContent::restoreContent(const ValueTree &restoredState)
{
	restoredContentValues = restoredState.getChildWithName("Content");

	if (content.get() != nullptr)
	{
		content->restoreFromValueTree(restoredContentValues);
	}
}

void ProcessorWithScriptingContent::saveContent(ValueTree &savedState) const
{
	if (content.get() != nullptr)
		savedState.addChild(content->exportAsValueTree(), -1, nullptr);
}


ScriptingApi::Content::ScriptComponent * ProcessorWithScriptingContent::checkContentChangedInPropertyPanel()
{
	if (content.get() == nullptr) return nullptr;

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		if (content->getComponent(i)->isChanged()) return content->getComponent(i);
	}

	return nullptr;
}

FileChangeListener::~FileChangeListener()
{
	watchers.clear();
	masterReference.clear();

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

void FileChangeListener::addFileWatcher(const File &file)
{
	watchers.add(new FileWatcher(file, this));
}

void FileChangeListener::setFileResult(const File &file, Result r)
{
	for (int i = 0; i < watchers.size(); i++)
	{
		if (file == watchers[i]->getFile())
		{
			watchers[i]->setResult(r);
		}
	}
}

Result FileChangeListener::getWatchedResult(int index)
{
	return watchers[index]->getResult();
}

File FileChangeListener::getWatchedFile(int index) const
{
	if (index < watchers.size())
	{
		return watchers[index]->getFile();

	}
	else return File::nonexistent;
}

void FileChangeListener::showPopupForFile(int index, int charNumberToDisplay/*=0*/, int lineNumberToDisplay/*=-1*/)
{
#if USE_BACKEND
	const File watchedFile = getWatchedFile(index);


	for (int i = 0; i < currentPopups.size(); i++)
	{
		if (currentPopups[i] == nullptr)
		{
			currentPopups.remove(i--);
			continue;
		}

		if (dynamic_cast<PopupIncludeEditorWindow*>(currentPopups[i].getComponent())->getFile() == watchedFile)
		{
			currentPopups[i]->toFront(true);

			if (charNumberToDisplay != 0)
			{
				dynamic_cast<PopupIncludeEditorWindow*>(currentPopups[i].getComponent())->gotoChar(charNumberToDisplay, lineNumberToDisplay);
			}

			return;
		}
	}

	PopupIncludeEditorWindow *popup = new PopupIncludeEditorWindow(getWatchedFile(index), dynamic_cast<JavascriptProcessor*>(this));

	currentPopups.add(popup);

	popup->addToDesktop();

	if (charNumberToDisplay != 0)
	{
		popup->gotoChar(charNumberToDisplay, lineNumberToDisplay);
	}

#else
	ignoreUnused(index, charNumberToDisplay);
#endif
}

void JavascriptProcessor::showPopupForCallback(const Identifier& callback, int charNumberToDisplay, int lineNumberToDisplay)
{
#if USE_BACKEND
	for (int i = 0; i < callbackPopups.size(); i++)
	{
		if (callbackPopups[i] == nullptr)
		{
			callbackPopups.remove(i--);
			continue;
		}

		if (dynamic_cast<PopupIncludeEditorWindow*>(callbackPopups[i].getComponent())->getCallback() == callback)
		{
			callbackPopups[i]->toFront(true);

			if (charNumberToDisplay != 0)
			{
				dynamic_cast<PopupIncludeEditorWindow*>(callbackPopups[i].getComponent())->gotoChar(charNumberToDisplay, lineNumberToDisplay);
			}

			return;
		}
	}

	PopupIncludeEditorWindow *popup = new PopupIncludeEditorWindow(callback, dynamic_cast<JavascriptProcessor*>(this));

	callbackPopups.add(popup);

	popup->addToDesktop();

	if (charNumberToDisplay != 0)
	{
		popup->gotoChar(charNumberToDisplay, lineNumberToDisplay);
	}
#endif
}

ValueTree FileChangeListener::collectAllScriptFiles(ModulatorSynthChain *chainToExport)
{
	Processor::Iterator<JavascriptProcessor> iter(chainToExport);

	ValueTree externalScriptFiles = ValueTree("ExternalScripts");

	while (JavascriptProcessor *sp = iter.getNextProcessor())
	{
		for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			File scriptFile = sp->getWatchedFile(i);
			String fileName = scriptFile.getRelativePathFrom(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Scripts));

			// Wow, much cross-platform, very OSX, totally Windows
			fileName = fileName.replace("\\", "/");

			bool exists = false;

			for (int j = 0; j < externalScriptFiles.getNumChildren(); j++)
			{
				if (externalScriptFiles.getChild(j).getProperty("FileName").toString() == fileName)
				{
					exists = true;
					break;
				}
			}

			if (exists) continue;

			String fileContent = scriptFile.loadFileAsString();
			ValueTree script("Script");

			script.setProperty("FileName", fileName, nullptr);
			script.setProperty("Content", fileContent, nullptr);

			externalScriptFiles.addChild(script, -1, nullptr);
		}
	}

	return externalScriptFiles;
}


JavascriptProcessor::JavascriptProcessor(MainController *mc) :
mainController(mc),
scriptEngine(new HiseJavascriptEngine(this)),
lastCompileWasOK(false),
currentCompileThread(nullptr),
lastResult(Result::ok())
{

}



JavascriptProcessor::~JavascriptProcessor()
{
	
	scriptEngine = nullptr;
}


void JavascriptProcessor::fileChanged()
{
	compileScript();
}




void JavascriptProcessor::clearExternalWindows()
{
	if (callbackPopups.size() != 0)
	{
		for (int i = 0; i < callbackPopups.size(); i++)
		{
			if (callbackPopups[i].getComponent() != nullptr)
			{
				callbackPopups[i]->closeButtonPressed();
			}

		}

		callbackPopups.clear();
	}
}

JavascriptProcessor::SnippetResult JavascriptProcessor::compileInternal()
{
	ProcessorWithScriptingContent* thisAsScriptBaseProcessor = dynamic_cast<ProcessorWithScriptingContent*>(this);

	ScriptingApi::Content* content = thisAsScriptBaseProcessor->getScriptingContent();

	const bool saveThisContent = lastCompileWasOK && content != nullptr && !useStoredContentData;

	if (saveThisContent) thisAsScriptBaseProcessor->restoredContentValues = content->exportAsValueTree();

	ScopedLock callbackLock(mainController->getLock());
	ScopedWriteLock sl(mainController->getCompileLock());
    

	scriptEngine->clearDebugInformation();

	setupApi();

	content = thisAsScriptBaseProcessor->getScriptingContent();

	thisAsScriptBaseProcessor->allowObjectConstructors = true;

	const static Identifier onInit("onInit");

	if (compileScriptAsWhole)
	{
		// Check the rest of the snippets or they will be deleted on failed compile...
		for (int i = 0; i < getNumSnippets(); i++)
		{
			getSnippet(i)->checkIfScriptActive();
		}

		String wholeScript;
		mergeCallbacksToScript(wholeScript);

		lastResult = scriptEngine->execute(wholeScript, true);

		if (!lastResult.wasOk())
		{
			content->endInitialization();
			thisAsScriptBaseProcessor->allowObjectConstructors = false;

			lastCompileWasOK = false;

			scriptEngine->rebuildDebugInformation();
			return SnippetResult(lastResult, 0);
		}
	}
	else
	{
		for (int i = 0; i < getNumSnippets(); i++)
		{
			getSnippet(i)->checkIfScriptActive();

			if (!getSnippet(i)->isSnippetEmpty())
			{
				lastResult = scriptEngine->execute(getSnippet(i)->getSnippetAsFunction(), getSnippet(i)->getCallbackName() == onInit);

				if (!lastResult.wasOk())
				{
					content->endInitialization();
					thisAsScriptBaseProcessor->allowObjectConstructors = false;

					// Check the rest of the snippets or they will be deleted on failed compile...
					for (int j = i; j < getNumSnippets(); j++)
					{
						getSnippet(j)->checkIfScriptActive();
					}

					lastCompileWasOK = false;

					scriptEngine->rebuildDebugInformation();
					return SnippetResult(lastResult, i);

				}
			}
		}
	}

	scriptEngine->rebuildDebugInformation();

	content->restoreAllControlsFromPreset(thisAsScriptBaseProcessor->restoredContentValues);

	useStoredContentData = false; // From now on it's normal;

	content->endInitialization();

	thisAsScriptBaseProcessor->allowObjectConstructors = false;

	lastCompileWasOK = true;

	postCompileCallback();

	return SnippetResult(Result::ok(), getNumSnippets());
}



JavascriptProcessor::SnippetResult JavascriptProcessor::compileScript()
{
	const bool useBackgroundThread = mainController->isUsingBackgroundThreadForCompiling();

	SnippetResult result = SnippetResult(Result::ok(), 0);

	if (useBackgroundThread)
	{
		CompileThread ct(this);

		currentCompileThread = &ct;

		ct.runThread();

		currentCompileThread = nullptr;

		result = ct.result;
	}
	else
	{
		result = compileInternal();
	}

	if (lastCompileWasOK)
	{
		String x;
		mergeCallbacksToScript(x);
	}
	clearFileWatchers();

	const int numFiles = scriptEngine->getNumIncludedFiles();

	for (int i = 0; i < numFiles; i++)
	{
		addFileWatcher(scriptEngine->getIncludedFile(i));
		setFileResult(scriptEngine->getIncludedFile(i), scriptEngine->getIncludedFileResult(i));
	}

	const String fileName = ApiHelpers::getFileNameFromErrorMessage(result.r.getErrorMessage());

	if (fileName.isNotEmpty())
	{
		for (int i = 0; i < getNumWatchedFiles(); i++)
		{
			if (getWatchedFile(i).getFileName() == fileName)
			{
				setFileResult(getWatchedFile(i), result.r);
			}
		}
	}

	mainController->sendScriptCompileMessage(this);

	return result;
}


void JavascriptProcessor::setupApi()
{
	clearFileWatchers();

	scriptEngine = new HiseJavascriptEngine(this);
	scriptEngine->maximumExecutionTime = RelativeTime(mainController->getCompileTimeOut());

	registerApiClasses();
	
	scriptEngine->registerNativeObject("Globals", mainController->getGlobalVariableObject());
	scriptEngine->registerGlobalStorge(mainController->getGlobalVariableObject());

	registerCallbacks();
}


void JavascriptProcessor::registerCallbacks()
{
	const Processor* p = dynamic_cast<Processor*>(this);

	const double bufferTime = (double)p->getBlockSize() / p->getSampleRate() * 1000.0;

	for (int i = 0; i < getNumSnippets(); i++)
	{
		scriptEngine->registerCallbackName(getSnippet(i)->getCallbackName(), getSnippet(i)->getNumArgs(), bufferTime);
	}
}


JavascriptProcessor::SnippetDocument * JavascriptProcessor::getSnippet(const Identifier& id)
{
	for (int i = 0; i < getNumSnippets(); i++)
	{
		if (getSnippet(i)->getCallbackName() == id) return getSnippet(i);
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptProcessor::getSnippet(const Identifier& id) const
{
	for (int i = 0; i < getNumSnippets(); i++)
	{
		if (getSnippet(i)->getCallbackName() == id) return getSnippet(i);
	}

	return nullptr;
}

void JavascriptProcessor::saveScript(ValueTree &v) const
{
	String x;
	mergeCallbacksToScript(x);
	v.setProperty("Script", x, nullptr);
}

void JavascriptProcessor::restoreScript(const ValueTree &v)
{
	String x = v.getProperty("Script", String::empty);
	parseSnippetsFromString(x, true);

	if (Processor* parent = ProcessorHelpers::findParentProcessor(dynamic_cast<Processor*>(this), true))
	{
		if (parent->getMainController()->shouldSkipCompiling())
		{
			dynamic_cast<ProcessorWithScriptingContent*>(this)->restoredContentValues = v.getChildWithName("Content");
			useStoredContentData = true;
		}
		else
		{
			compileScript();
		}
	}
	else jassertfalse;
}

void JavascriptProcessor::mergeCallbacksToScript(String &x) const
{
	for (int i = 0; i < getNumSnippets(); i++)
	{
		const SnippetDocument *s = getSnippet(i);

		x << s->getSnippetAsFunction();
	}
}

bool JavascriptProcessor::parseSnippetsFromString(const String &x, bool clearUndoHistory /*= false*/)
{
	String codeToCut = String(x);

	for (int i = getNumSnippets(); i > 1; i--)
	{
		SnippetDocument *s = getSnippet(i-1);
		
		const String filter = "function " + s->getCallbackName().toString() + "(";

		if (!x.contains(filter))
		{
			PresetHandler::showMessageWindow("Invalid script", "The script you are trying to load is not a valid HISE script file.\nThe callback " + s->getCallbackName().toString() + " is not defined.", PresetHandler::IconType::Error);
			debugError(dynamic_cast<Processor*>(this), s->getCallbackName().toString() + " could not be parsed!");
			return false;
		}

		String code = codeToCut.fromLastOccurrenceOf(filter, true, false);

		

		s->replaceAllContent(code);
        
		codeToCut = codeToCut.upToLastOccurrenceOf(filter, false, false);
        
        if(clearUndoHistory)
        {
            s->getUndoManager().clearUndoHistory();
        }
	}

	getSnippet(0)->replaceAllContent(codeToCut);

	debugToConsole(dynamic_cast<Processor*>(this), "All callbacks sucessfuly parsed");

	return true;
}



void JavascriptProcessor::setCompileProgress(double progress)
{
	if (currentCompileThread != nullptr && mainController->isUsingBackgroundThreadForCompiling())
	{
		currentCompileThread->setProgress(progress);
	}
}



JavascriptProcessor::SnippetDocument::SnippetDocument(const Identifier &callbackName_, const String &parameters_) :
CodeDocument(),
isActive(false),
callbackName(callbackName_)
{
	parameters = StringArray::fromTokens(parameters_, " ", "");
	numArgs = parameters.size();
	
	if (callbackName != Identifier("onInit"))
	{
		emptyText << "function " << callbackName_.toString() << "(";
		
		for (int i = 0; i < numArgs; i++)
		{
			emptyText << parameters[i];
			if (i != numArgs - 1) emptyText << ", ";
		}
		
		emptyText << ")\n";
		emptyText << "{\n";
		emptyText << "\t\n";
		emptyText << "}\n";
	};
	
	replaceAllContent(emptyText);
    
    getUndoManager().clearUndoHistory();
}

void JavascriptProcessor::SnippetDocument::checkIfScriptActive()
{
	isActive = true;

	if (!getAllContent().containsNonWhitespaceChars()) isActive = false;

	String trimmedText = getAllContent().removeCharacters(" \t\n\r");

	String trimmedEmptyText = emptyText.removeCharacters(" \t\n\r");

	if (trimmedEmptyText == trimmedText)	isActive = false;
}

String JavascriptProcessor::SnippetDocument::getSnippetAsFunction() const
{
	if (isSnippetEmpty()) return emptyText;
	else				  return getAllContent();
}

JavascriptProcessor::CompileThread::CompileThread(JavascriptProcessor *processor) :
ThreadWithProgressWindow("Compiling", true, false),
sp(processor),
result(SnippetResult(Result::ok(), 0))
{
	getAlertWindow()->setLookAndFeel(&alaf);
}

void JavascriptProcessor::CompileThread::run()
{
	result = sp->compileInternal();
}
