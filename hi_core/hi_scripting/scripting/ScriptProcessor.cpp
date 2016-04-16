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


float ScriptBaseProcessor::getAttribute(int index) const
{

	if (content != nullptr && index < content->getNumComponents())
	{
		return content->getComponent(index)->getValue();

	}

	else return 1.0f;
}


ScriptBaseProcessor::~ScriptBaseProcessor()
{
	if (scriptProcessorDeactivatedRoundRobin && dynamic_cast<ModulatorSampler*>(getOwnerSynth()) != nullptr)
	{
		dynamic_cast<ModulatorSampler*>(getOwnerSynth())->setUseRoundRobinLogic(true); // Reactivate Round Robin Logic if ScriptProcessor is deleted.
	}

	masterReference.clear();
}



void ScriptBaseProcessor::setInternalAttribute(int index, float newValue)
{

    jassert(content.get() != nullptr);
    
	if (content != nullptr && index < content->getNumComponents())
	{
		ScriptingApi::Content::ScriptComponent *c = content->getComponent(index);

		c->setValue(newValue);
		controlCallback(c, newValue);
	}
}

ValueTree ScriptBaseProcessor::exportAsValueTree() const
{
	ValueTree v = MidiProcessor::exportAsValueTree();

	if (content.get() != nullptr)
	{

#if USE_OLD_FILE_FORMAT

		MemoryBlock b;

		MemoryOutputStream mos(b, false);

		content->exportAsValueTree().writeToStream(mos);

		v.setProperty("Content", var(b.getData(), b.getSize()), nullptr);

#else

		v.addChild(content->exportAsValueTree(), -1, nullptr);

#endif

	}

	return v;
}

void ScriptBaseProcessor::restoreFromValueTree(const ValueTree &v)
{
	MidiProcessor::restoreFromValueTree(v);

	if (getOwnerSynth() != nullptr)
	{
#if USE_OLD_FILE_FORMAT

		MemoryBlock b = *v.getProperty("Content", MemoryBlock()).getBinaryData();

		restoredContentValues = ValueTree::readFromData(b.getData(), b.getSize());
#else
		restoredContentValues = v.getChildWithName("Content");

#endif
        
		if (content.get() != nullptr)
		{
			content->restoreFromValueTree(restoredContentValues);
		}
	}
	else
	{
		jassertfalse;
	}
}

void ScriptProcessor::replaceReferencesWithGlobalFolder()
{
	String script;
	
	mergeCallbacksToScript(script);

	const StringArray allLines = StringArray::fromLines(script);

	StringArray newLines;

	for (int i = 0; i < allLines.size(); i++)
	{
		String line = allLines[i];

		if (line.contains("\"fileName\""))
		{
			String fileName = line.fromFirstOccurrenceOf("\"fileName\"", false, false);
			fileName = fileName.fromFirstOccurrenceOf("\"", false, false);
			fileName = fileName.upToFirstOccurrenceOf("\"", false, false);

			if (fileName.isNotEmpty()) line = line.replace(fileName, getGlobalReferenceForFile(fileName));
		}

		else if (line.contains("\"filmstripImage\"") && !line.contains("Use default skin"))
		{
			String fileName = line.fromFirstOccurrenceOf("\"filmstripImage\"", false, false);
			fileName = fileName.fromFirstOccurrenceOf("\"", false, false);
			fileName = fileName.upToFirstOccurrenceOf("\"", false, false);
			
			if(fileName.isNotEmpty()) line = line.replace(fileName, getGlobalReferenceForFile(fileName));
		}
		else if (line.contains(".setImageFile("))
		{
			String fileName = line.fromFirstOccurrenceOf(".setImageFile(", false, false);
			fileName = fileName.fromFirstOccurrenceOf("\"", false, false);
			fileName = fileName.upToFirstOccurrenceOf("\"", false, false);

			line = line.replace(fileName, getGlobalReferenceForFile(fileName));
		}

		newLines.add(line);
	}

	String newCode = newLines.joinIntoString("\n");

	parseSnippetsFromString(newCode);

	compileScript();
}

ScriptProcessor::ScriptProcessor(MainController *mc, const String &id) :
ScriptBaseProcessor(mc, id),
doc(new CodeDocument()),
onInitCallback(new SnippetDocument("onInit")),
onNoteOnCallback(new SnippetDocument("onNoteOn")),
onNoteOffCallback(new SnippetDocument("onNoteOff")),
onControllerCallback(new SnippetDocument("onController")),
onTimerCallback(new SnippetDocument("onTimer")),
onControlCallback(new SnippetDocument("onControl")),
scriptEngine(new JavascriptEngine()),
lastExecutionTime(0.0),
currentMidiMessage(nullptr),
apiData(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize)),
front(false),
deferred(false),
deferredUpdatePending(false),
lastCompileWasOK(false),
currentCompileThread(nullptr)
{
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onNoteOnOpen");
	editorStateIdentifiers.add("onNoteOffOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onTimerOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("externalPopupShown");
}



ScriptProcessor::~ScriptProcessor()
{
#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif

	scriptEngine = nullptr;
	doc = nullptr;
}

Path ScriptProcessor::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor));

	return path;
}

ValueTree ScriptProcessor::exportAsValueTree() const
{
	ValueTree v = ScriptBaseProcessor::exportAsValueTree();

	String x;

	mergeCallbacksToScript(x);

	v.setProperty("Script", x, nullptr);



	return v;
}

void ScriptProcessor::restoreFromValueTree(const ValueTree &v)
{
	String x = v.getProperty("Script", String::empty);

	parseSnippetsFromString(x);
    
	ScriptBaseProcessor::restoreFromValueTree(v);
}

void ScriptProcessor::fileChanged()
{
	compileScript();
}

ScriptProcessor::SnippetDocument * ScriptProcessor::getSnippet(int c)
{
	switch (c)
	{
	case onInit:		return onInitCallback;
	case onNoteOn:		return onNoteOnCallback;
	case onNoteOff:		return onNoteOffCallback;
	case onController:	return onControllerCallback;
	case onTimer:		return onTimerCallback;
	case onControl:		return onControlCallback;
	default:			jassertfalse; return nullptr;
	}
}

const ScriptProcessor::SnippetDocument * ScriptProcessor::getSnippet(int c) const
{
	switch (c)
	{
	case onInit:		return onInitCallback;
	case onNoteOn:		return onNoteOnCallback;
	case onNoteOff:		return onNoteOffCallback;
	case onController:	return onControllerCallback;
	case onTimer:		return onTimerCallback;
	case onControl:		return onControlCallback;
	default:			jassertfalse; return nullptr;
	}
}

ProcessorEditorBody *ScriptProcessor::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ScriptingEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};

void ScriptProcessor::handleAsyncUpdate()
{
	jassert(isDeferred());
	jassert(!deferredUpdatePending);

	deferredUpdatePending = true;

	if(deferredMidiMessages.getNumEvents() != 0)
	{
		ScopedLock sl(lock);

		copyBuffer.swapWith(deferredMidiMessages);
	}
	else
	{
		deferredUpdatePending = false;
		return;
	}

	MidiBuffer::Iterator iter(copyBuffer);

	MidiMessage m;
	int samplePos;

	while(iter.getNextEvent(m, samplePos))
	{
		currentMessage = m;
		currentMidiMessage->setMidiMessage(&m);


		runScriptCallbacks();
	}

	copyBuffer.clear();
	deferredUpdatePending = false;

}

void ScriptProcessor::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	if (onControlCallback->isSnippetEmpty())
	{
		return;
	}

	Result r = Result::ok();

	const var args[2] = { component, controllerValue };

	scriptEngine->maximumExecutionTime = RelativeTime(0.5);

	allowObjectConstructors = true;

	scriptEngine->callFunction(getSnippet(onControl)->getCallbackName(), var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 2), &r);

	allowObjectConstructors = false;

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		sendSynchronousChangeMessage();
	}
	else
	{
		sendChangeMessage();
	}
	

#if USE_BACKEND
	if (!r.wasOk()) getMainController()->writeToConsole(r.getErrorMessage(), 1, this, content->getColour());
#endif
}

ScriptingApi::Content::ScriptComponent * ScriptProcessor::checkContentChangedInPropertyPanel()
{
	if (content.get() == nullptr) return nullptr;

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		if (content->getComponent(i)->isChanged()) return content->getComponent(i);
	}

	return nullptr;
}

ScriptProcessor::SnippetResult ScriptProcessor::compileInternal()
{
	if (lastCompileWasOK && content != nullptr) restoredContentValues = content->exportAsValueTree();

	ScopedLock sl(compileLock);

	scriptEngine = new JavascriptEngine();
	scriptEngine->maximumExecutionTime = RelativeTime(getMainController()->getCompileTimeOut());

	setupApi();

	allowObjectConstructors = true;

	content->restoreFromValueTree(restoredContentValues);

	for (int i = 0; i < numCallbacks; i++)
	{
		getSnippet(i)->checkIfScriptActive();

		if (!getSnippet(i)->isSnippetEmpty())
		{
			Result r = scriptEngine->execute(getSnippet(i)->getSnippetAsFunction());

			if (!r.wasOk())
			{
				//content->restoreFromValueTree(restoredContentValues);
				content->endInitialization();
				allowObjectConstructors = false;

				// Check the rest of the snippets or they will be deleted on failed compile...
				for (int j = i; j < numCallbacks; j++)
				{
					getSnippet(j)->checkIfScriptActive();
				}

				lastCompileWasOK = false;

				return SnippetResult(r, (Callback)i);

			}
		}
	}

	content->restoreFromValueTree(restoredContentValues);

	content->endInitialization();

	allowObjectConstructors = false;

	lastCompileWasOK = true;

	return SnippetResult(Result::ok(), numCallbacks);
}



ScriptProcessor::SnippetResult ScriptProcessor::compileScript()
{
	const bool useBackgroundThread = getMainController()->isUsingBackgroundThreadForCompiling();

	SnippetResult result = SnippetResult(Result::ok(), ScriptBaseProcessor::Callback::onInit);

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
		parseSnippetsFromString(x);
	}

	getMainController()->sendScriptCompileMessage(this);

	return result;
}

void ScriptProcessor::processMidiMessage(MidiMessage &m)
{	
	if(isDeferred())
	{
		processThisMessage = true;

		if (processThisMessage)
		{
			ScopedLock sl(lock);
			deferredMidiMessages.addEvent(m, (int)m.getTimeStamp());
		}
		
		//currentMessage = m;
		//currentMidiMessage->setMidiMessage(&m);
		currentMidiMessage->ignoreEvent(false);

		triggerAsyncUpdate();
	}
	else
	{
		if(currentMidiMessage != nullptr)
		{
			currentMessage = m;
			currentMidiMessage->setMidiMessage(&m);
			currentMidiMessage->ignoreEvent(false);

			runScriptCallbacks();

			processThisMessage = !currentMidiMessage->ignored;
		}
	}

	
};

void ScriptProcessor::setupApi()
{
	includedFileNames.clear();

	clearFileWatchers();

	scriptEngine = new JavascriptEngine();
	scriptEngine->maximumExecutionTime = RelativeTime(getMainController()->getCompileTimeOut());

	content = new ScriptingApi::Content(this);

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, getOwnerSynth());
	samplerObject = new ScriptingApi::Sampler(this, dynamic_cast<ModulatorSampler*>(getOwnerSynth()));

	DynamicObject *synthParameters = new DynamicObject();

	for(int i = 0; i < getOwnerSynth()->getNumParameters(); i++)
	{
		synthParameters->setProperty(getOwnerSynth()->getIdentifierForParameterIndex(i), var(i));
	}


	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerNativeObject("SynthParameters", synthParameters);
	scriptEngine->registerNativeObject("Console", new ScriptingApi::Console(this));
	scriptEngine->registerNativeObject("Engine", engineObject);
	scriptEngine->registerNativeObject("Message", currentMidiMessage);
	scriptEngine->registerNativeObject("Synth", synthObject);
	scriptEngine->registerNativeObject("Sampler", samplerObject);

	scriptEngine->registerNativeObject("Globals", getMainController()->getGlobalVariableObject());
	scriptEngine->execute("function include(string){Engine.include(string);};");

}

#pragma warning( push )
#pragma warning( disable : 4390)

void ScriptProcessor::runScriptCallbacks()
{
	ScopedLock sl(compileLock);

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.003);

	if(currentMessage.isNoteOn())
	{
		synthObject->increaseNoteCounter();

		if(onNoteOnCallback->isSnippetEmpty()) return;

		Result r = Result::ok();
		const double startTime = consoleEnabled ? Time::getMillisecondCounterHiRes() : 0.0;

		scriptEngine->callFunction(getSnippet(onNoteOn)->getCallbackName(), var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), nullptr, 0), &r);

		lastExecutionTime = consoleEnabled ? Time::getMillisecondCounterHiRes() - startTime : 0.0 ;
		sendChangeMessage();

		if (!r.wasOk()) debugError(this, r.getErrorMessage());

	}
	else if(currentMessage.isNoteOff())
	{
		synthObject->decreaseNoteCounter();

		if(onNoteOffCallback->isSnippetEmpty()) return;
		Result r = Result::ok();
		const double startTime = consoleEnabled ? Time::getMillisecondCounterHiRes() : 0.0;

		scriptEngine->callFunction(getSnippet(onNoteOff)->getCallbackName(), var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), nullptr, 0), &r);

		lastExecutionTime = consoleEnabled ? Time::getMillisecondCounterHiRes() - startTime : 0.0 ;
		sendChangeMessage();

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if(currentMessage.isController() || currentMessage.isPitchWheel() || currentMessage.isAftertouch())
	{
		if(currentMessage.isControllerOfType(64))
		{
			synthObject->setSustainPedal(currentMessage.getControllerValue() > 64);
		}

		if(onControllerCallback->isSnippetEmpty()) return;

		// All notes off are controller message, so they should not be processed, or it can lead to loop.
		if(currentMessage.isAllNotesOff()) return;

		Result r = Result::ok();
		const double startTime = consoleEnabled ? Time::getMillisecondCounterHiRes() : 0.0;

		scriptEngine->callFunction(getSnippet(onController)->getCallbackName(), var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), nullptr, 0), &r);

		lastExecutionTime = consoleEnabled ? Time::getMillisecondCounterHiRes() - startTime : 0.0 ;
		sendChangeMessage();

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if (currentMessage.isSongPositionPointer())
	{

		//if (onNoteOffCallback->isSnippetEmpty()) return;
		Result r = Result::ok();
		const double startTime = consoleEnabled ? Time::getMillisecondCounterHiRes() : 0.0;

		static Identifier onClock("onClock");

		var args[1] = { currentMessage.getSongPositionPointerMidiBeat() };

		scriptEngine->callFunction(onClock, var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);

		lastExecutionTime = consoleEnabled ? Time::getMillisecondCounterHiRes() - startTime : 0.0;
		sendChangeMessage();

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if (currentMessage.isMidiStart() || currentMessage.isMidiStop())
	{
		Result r = Result::ok();
		const double startTime = consoleEnabled ? Time::getMillisecondCounterHiRes() : 0.0;

		static Identifier onClock("onTransport");

		var args[1] = { currentMessage.isMidiStart() };

		scriptEngine->callFunction(onClock, var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);

		lastExecutionTime = consoleEnabled ? Time::getMillisecondCounterHiRes() - startTime : 0.0;
		sendChangeMessage();
	}
}



void ScriptProcessor::includeFile(const String &name)
{
#if USE_FRONTEND

	if (includedFileNames.contains(name, true))
	{
		debugError(this, "Warning: File " + name + " is already included.");
	}
	else
	{
		String content = getMainController()->getExternalScriptFromCollection(name);
		debugToConsole(this, "Including " + name);

		includedFileNames.add(name);

		scriptEngine->execute(content);

		return;
	}

#else

	const File file = GET_PROJECT_HANDLER(this).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile(name);

	if (file.existsAsFile())
	{
		if (includedFileNames.contains(name, true))
		{
			debugError(this, "Warning: File " + name + " is already included.");
		}
		else
		{
			String content = file.loadFileAsString();
			debugToConsole(this, "Including " + name);

			includedFileNames.add(name);
			
			addFileWatcher(file);

			Result r = scriptEngine->execute(content);

			setFileResult(file, r);

			return;
		}
	}
	else
	{
		debugError(this, "File " + file.getFullPathName() + " was not found.");
	}
#endif
}

void ScriptProcessor::mergeCallbacksToScript(String &x) const
{
	for (int i = 0; i < numCallbacks; i++)
	{
		const SnippetDocument *s = getSnippet(i);

		x << s->getSnippetAsFunction();
	}
}

void ScriptProcessor::parseSnippetsFromString(const String &x)
{
	String codeToCut = String(x);

	for (int i = numCallbacks; i > 1; i--)
	{
		SnippetDocument *s = getSnippet(i-1);
		
		String filter = "function " + s->getCallbackName().toString() + "(";

		String code = codeToCut.fromLastOccurrenceOf(filter, true, false);

		if (!code.containsNonWhitespaceChars())
		{
			debugError(this, s->getCallbackName().toString() + " could not be parsed!");
		}

		s->replaceAllContent(code);

		codeToCut = codeToCut.upToLastOccurrenceOf(filter, false, false);
	}

	getSnippet(0)->replaceAllContent(codeToCut);

	debugToConsole(this, "All callbacks sucessfuly parsed");
}

ReferenceCountedObject * ScriptProcessor::textInputisArrayElementObject(const String &t)
{
	if (t.contains("[") && t.contains("]"))
	{
		const String possibleArrayName = t.upToFirstOccurrenceOf("[", false, false);

		if (!Identifier::isValidIdentifier(possibleArrayName))
		{
			return nullptr;
		}

		Identifier variableName = Identifier(possibleArrayName);

		var *v = scriptEngine->getRootObjectProperties().getVarPointer(variableName);

		if (v == nullptr || v->isUndefined())
		{
			return nullptr;
		}

		if (!v->isArray())
		{
			return nullptr;
		}


		const int number = t.fromFirstOccurrenceOf("[", false, false).upToFirstOccurrenceOf("]", false, false).getIntValue();

		if (number < 0 || number >= v->getArray()->size())
		{
			return nullptr;
		}

		var obj = v->getArray()->getUnchecked(number);

		if (obj.isObject())
		{
			return obj.getObject();
		}

		return nullptr;
	}


	return nullptr;
}

DynamicObject * ScriptProcessor::textInputMatchesScriptingObject(const String &textToShow)
{

	if (!Identifier::isValidIdentifier(textToShow)) return nullptr;

	String stripped = textToShow.removeCharacters(" \r\n\t");

	if (scriptEngine->getRootObjectProperties().contains(stripped))
	{
		var *v = scriptEngine->getRootObjectProperties().getVarPointer(stripped);

		DynamicObject *obj = v->getDynamicObject();

		return obj;
	}

	return nullptr;
}

XmlElement * ScriptProcessor::textInputMatchesApiClass(const String &s) const
{
	for (int i = 0; i < apiData.getNumChildren(); i++)
	{
		const String apiClassName = apiData.getChild(i).getType().toString();

		if (s == apiClassName || s == (apiClassName + "."))
		{
			return apiData.getChild(i).createXml();
		}
	}

	return nullptr;
}

void ScriptProcessor::runTimerCallback(int offsetInBuffer/*=-1*/)
{
	if (isBypassed() || onTimerCallback->isSnippetEmpty()) return;

	ScopedLock sl(compileLock);

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.002);

	Result r = Result::ok();
	const double startTime = consoleEnabled ? Time::getMillisecondCounterHiRes() : 0.0;

	const var args[1] = { offsetInBuffer };

	scriptEngine->callFunction(getSnippet(onTimer)->getCallbackName(), var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);

	lastExecutionTime = consoleEnabled ? Time::getMillisecondCounterHiRes() - startTime : 0.0;

	if (isDeferred())
	{
		sendSynchronousChangeMessage();
	}
	else
	{
		sendChangeMessage();
	}

#if USE_BACKEND
	if (!r.wasOk()) getMainController()->writeToConsole(r.getErrorMessage(), 1, this, content->getColour());
#endif
}

#pragma warning( pop ) 

void ScriptProcessor::deferCallbacks(bool addToFront_)
{ 
	deferred = addToFront_;
	if(deferred)
	{
		getOwnerSynth()->stopSynthTimer();
	}
	else
	{
		stopTimer();
	}
};

StringArray ScriptProcessor::getImageFileNames() const
{
	jassert(isFront());

	StringArray fileNames;

	for(int i = 0; i < content->getNumComponents(); i++)
	{
		const ScriptingApi::Content::ScriptImage *image = dynamic_cast<const ScriptingApi::Content::ScriptImage*>(content->getComponent(i));

		if(image != nullptr) fileNames.add(image->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::FileName));
	}

	return fileNames;
}

ScriptProcessor::SnippetDocument::SnippetDocument(const Identifier &callbackName_) :
CodeDocument(),
lines(Range<int>()),
isActive(false),
callbackName(callbackName_)
{
	String header;
	//String x = callbackName_.toString() + " Callback";

	//String line;

	//for (int i = 0; i < x.length(); i++) line << "=";

	//header << "// " << line << "\n";
	//header << "// " << x << "\n";
	//header << "\n";

	if (callbackName == Identifier("onInit"))
	{

	}
	else if (callbackName == Identifier("onControl"))
	{
		header << "function onControl(number, value)\n";
		header << "{\n";
		header << "\t\n";
		header << "}\n";
	}
	else {
		header << "function " << callbackName_.toString() << "()\n";
		header << "{\n";
		header << "\t\n";
		header << "}\n";
	};

	setEmptyText(header);
}

int ScriptProcessor::SnippetDocument::addLineToString(String &t, const String &lineToAdd)
{
	// Every line must be added seperately!
	//jassert(lineToAdd.findFirstOccurenceOf('\n') == -1);

	t << lineToAdd << "\n";
	return 1;
}

int ScriptProcessor::SnippetDocument::addToString(String &t, int startLine)
{
	t << getAllContent();

	isActive = true;

	if (!getAllContent().containsNonWhitespaceChars()) isActive = false;
	if (getAllContent() == emptyText)					isActive = false;

	lines = Range<int>(startLine, startLine + getNumLines());
	return getNumLines();
}

FileChangeListener::~FileChangeListener()
{
	watchers.clear();
	masterReference.clear();

    if(currentPopups.size() != 0)
    {
        for(int i = 0; i < currentPopups.size(); i++)
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
    if(index < watchers.size())
    {
        return watchers[index]->getFile();

    }
    else return File::nonexistent;
}

void FileChangeListener::showPopupForFile(int index)
{
    const File watchedFile = getWatchedFile(index);
    
    
    for(int i = 0; i < currentPopups.size(); i++)
    {
        if(currentPopups[i] == nullptr)
        {
            currentPopups.remove(i--);
            continue;
        }
        
        if(dynamic_cast<PopupIncludeEditorWindow*>(currentPopups[i].getComponent())->getFile() == watchedFile)
        {
            currentPopups[i]->toFront(true);
            return;
        }
    }
    
	PopupIncludeEditorWindow *popup = new PopupIncludeEditorWindow(getWatchedFile(index), dynamic_cast<ScriptProcessor*>(this));

    currentPopups.add(popup);

    popup->addToDesktop();
	
}

ValueTree FileChangeListener::collectAllScriptFiles(ModulatorSynthChain *chainToExport)
{
	Processor::Iterator<ScriptProcessor> iter(chainToExport);

	ValueTree externalScriptFiles = ValueTree("ExternalScripts");

	while (ScriptProcessor *sp = iter.getNextProcessor())
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
