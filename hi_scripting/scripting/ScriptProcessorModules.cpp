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


JavascriptMidiProcessor::JavascriptMidiProcessor(MainController *mc, const String &id) :
ScriptBaseMidiProcessor(mc, id),
JavascriptProcessor(mc),
onInitCallback(new SnippetDocument("onInit")),
onNoteOnCallback(new SnippetDocument("onNoteOn")),
onNoteOffCallback(new SnippetDocument("onNoteOff")),
onControllerCallback(new SnippetDocument("onController")),
onTimerCallback(new SnippetDocument("onTimer")),
onControlCallback(new SnippetDocument("onControl", "number value")),
currentMidiMessage(nullptr),

front(false),
deferred(false),
deferredUpdatePending(false)
{
    editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onNoteOnOpen");
	editorStateIdentifiers.add("onNoteOffOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onTimerOpen");
	editorStateIdentifiers.add("onControlOpen");
	
	editorStateIdentifiers.add("externalPopupShown");
    
    setEditorState(Identifier("contentShown"), true);
    setEditorState(Identifier("onInitOpen"), true);
}



JavascriptMidiProcessor::~JavascriptMidiProcessor()
{
#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif

}

Path JavascriptMidiProcessor::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

ValueTree JavascriptMidiProcessor::exportAsValueTree() const
{
	ValueTree v = ScriptBaseMidiProcessor::exportAsValueTree();
	saveScript(v);
	return v;
}

void JavascriptMidiProcessor::restoreFromValueTree(const ValueTree &v)
{
	restoreScript(v);
	ScriptBaseMidiProcessor::restoreFromValueTree(v);
}


JavascriptMidiProcessor::SnippetDocument * JavascriptMidiProcessor::getSnippet(int c)
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

const JavascriptMidiProcessor::SnippetDocument * JavascriptMidiProcessor::getSnippet(int c) const
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

ProcessorEditorBody *JavascriptMidiProcessor::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
};


void JavascriptMidiProcessor::processHiseEvent(HiseEvent &m)
{
	if (isDeferred())
	{
	
		ScopedWriteLock sl(defferedMessageLock);
		
		deferredEvents.addEvent(m);
		
		triggerAsyncUpdate();
	}
	else
	{
		ADD_GLITCH_DETECTOR("Processing " + getId() + " script callbacks");

		if (currentMidiMessage != nullptr)
		{
			currentEvent = &m;
			currentMidiMessage->setHiseEvent(m);

			runScriptCallbacks();

			currentEvent = nullptr;
		}
	}

}

void JavascriptMidiProcessor::registerApiClasses()
{
	content = new ScriptingApi::Content(this);
    front = false;

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, getOwnerSynth());
	samplerObject = new ScriptingApi::Sampler(this, dynamic_cast<ModulatorSampler*>(getOwnerSynth()));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::Colours());
	scriptEngine->registerApiClass(synthObject);
	scriptEngine->registerApiClass(samplerObject);
    
    scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(getMainController()));
    scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
    
}



void JavascriptMidiProcessor::runScriptCallbacks()
{
	ScopedReadLock sl(mainController->getCompileLock());

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.03);

	if (currentEvent->isNoteOn())
	{
		synthObject->increaseNoteCounter(currentEvent->getNoteNumber());
		
		if (onNoteOnCallback->isSnippetEmpty()) return;

		scriptEngine->executeCallback(onNoteOn, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onNoteOnCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));

	}
	else if (currentEvent->isNoteOff())
	{
		synthObject->decreaseNoteCounter(currentEvent->getNoteNumber());

		if (onNoteOffCallback->isSnippetEmpty()) return;

		scriptEngine->executeCallback(onNoteOff, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onNoteOffCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
	else if (currentEvent->isController() || currentEvent->isPitchWheel() || currentEvent->isAftertouch())
	{
		if (currentEvent->isControllerOfType(64))
		{
			synthObject->setSustainPedal(currentEvent->getControllerValue() > 64);
		}

		if (onControllerCallback->isSnippetEmpty()) return;

		// All notes off are controller message, so they should not be processed, or it can lead to loop.
		if (currentEvent->isAllNotesOff()) return;

		Result r = Result::ok();
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onControllerCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}

#if 0
	
	else if (currentEvent->isSongPositionPointer())
	{
		Result r = Result::ok();

		static const Identifier onClock("onClock");

		var args = currentEvent->getSongPositionPointerMidiBeat();
	
		scriptEngine->callInlineFunction(onClock, &args, 1, &r);
		
		//scriptEngine->executeWithoutAllocation(onClock, var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if (currentEvent->isMidiStart() || currentEvent->isMidiStop())
	{
		Result r = Result::ok();

		static const Identifier onTransport("onTransport");

		var args = currentEvent->isMidiStart();

		scriptEngine->callInlineFunction(onTransport, &args, 1, &r);

		//scriptEngine->executeWithoutAllocation(onClock, var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);
	}
#endif
}


void JavascriptMidiProcessor::runTimerCallback(int /*offsetInBuffer*//*=-1*/)
{
	if (isBypassed() || onTimerCallback->isSnippetEmpty()) return;

	ScopedReadLock sl(mainController->getCompileLock());

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.002);

	currentEvent = nullptr;

	scriptEngine->executeCallback(onTimer, &lastResult);

	if (isDeferred())
	{
		sendSynchronousChangeMessage();
	}

	BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onTimerCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
}

void JavascriptMidiProcessor::deferCallbacks(bool addToFront_)
{
	deferred = addToFront_;
	if (deferred)
	{
		getOwnerSynth()->stopSynthTimer();
	}
	else
	{
		stopTimer();
	}
};

StringArray JavascriptMidiProcessor::getImageFileNames() const
{
	jassert(isFront());

	StringArray fileNames;

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		const ScriptingApi::Content::ScriptImage *image = dynamic_cast<const ScriptingApi::Content::ScriptImage*>(content->getComponent(i));

		if (image != nullptr) fileNames.add(image->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::FileName));
	}

	return fileNames;
}

void JavascriptMidiProcessor::replaceReferencesWithGlobalFolder()
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

			if (fileName.isNotEmpty()) line = line.replace(fileName, getGlobalReferenceForFile(fileName));
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


void JavascriptMidiProcessor::handleAsyncUpdate()
{
	jassert(isDeferred());
	jassert(!deferredUpdatePending);

	deferredUpdatePending = true;

	if (!deferredEvents.isEmpty())
	{
		ScopedWriteLock sl(defferedMessageLock);

		copyEventBuffer.copyFrom(deferredEvents);
		
		deferredEvents.clear();

	}
	else
	{
		deferredUpdatePending = false;
		return;
	}

	HiseEventBuffer::Iterator iter(copyEventBuffer);
	
	while (HiseEvent* m = iter.getNextEventPointer(true,true))
	{
		currentEvent = m;

		currentMidiMessage->setHiseEvent(*m);

		runScriptCallbacks();

		currentEvent = nullptr;
	}

	copyEventBuffer.clear();
	deferredUpdatePending = false;

}



JavascriptMasterEffect::JavascriptMasterEffect(MainController *mc, const String &id):
JavascriptProcessor(mc),
ProcessorWithScriptingContent(mc),
MasterEffectProcessor(mc, id),
onInitCallback(new SnippetDocument("onInit")),
prepareToPlayCallback(new SnippetDocument("prepareToPlay", "sampleRate blockSize")),
processBlockCallback(new SnippetDocument("processBlock", "channels")),
onControlCallback(new SnippetDocument("onControl", "number value"))
{
	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("prepareToPlayOpen");
	editorStateIdentifiers.add("processBlockOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");
}

JavascriptMasterEffect::~JavascriptMasterEffect()
{
	bufferL = nullptr;
	bufferR = nullptr;

#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif

}

Path JavascriptMasterEffect::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

ProcessorEditorBody * JavascriptMasterEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

JavascriptProcessor::SnippetDocument * JavascriptMasterEffect::getSnippet(int c)
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case Callback::onInit:			return onInitCallback;
	case Callback::prepareToPlay:	return prepareToPlayCallback;
	case Callback::processBlock:	return processBlockCallback;
	case Callback::onControl:		return onControlCallback;
	case Callback::numCallbacks:	return nullptr;
	default:
		break;
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptMasterEffect::getSnippet(int c) const
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case Callback::onInit:			return onInitCallback;
	case Callback::prepareToPlay:	return prepareToPlayCallback;
	case Callback::processBlock:	return processBlockCallback;
	case Callback::onControl:		return onControlCallback;
	case Callback::numCallbacks:	return nullptr;
	default:
		break;
	}

	return nullptr;
}

void JavascriptMasterEffect::registerApiClasses()
{
	content = new ScriptingApi::Content(this);

	engineObject = new ScriptingApi::Engine(this);
	
	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(getMainController()));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));

}


void JavascriptMasterEffect::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getBlockSize());
}

void JavascriptMasterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ScopedReadLock sl(mainController->getCompileLock());

	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	Array<var> channelArray;

	bufferL = new VariantBuffer(0);
	bufferR = new VariantBuffer(0);

	channelArray.add(var(bufferL));
	channelArray.add(var(bufferR));

	channels = var(channelArray);

	if (!prepareToPlayCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		scriptEngine->setCallbackParameter((int)Callback::prepareToPlay, 0, sampleRate);
		scriptEngine->setCallbackParameter((int)Callback::prepareToPlay, 1, samplesPerBlock);
		scriptEngine->executeCallback((int)Callback::prepareToPlay, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, prepareToPlayCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
}

void JavascriptMasterEffect::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	ignoreUnused(startSample);

	if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		ScopedReadLock sl(getMainController()->getCompileLock());

        jassert(startSample == 0);
        
		float *l = b.getWritePointer(0, 0);
		float *r = b.getWritePointer(1, 0);
        
        
		bufferL->referToData(l, numSamples);
		bufferR->referToData(r, numSamples);

		scriptEngine->setCallbackParameter((int)Callback::processBlock, 0, channels);
		scriptEngine->executeCallback((int)Callback::processBlock, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, processBlockCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
}

JavascriptVoiceStartModulator::JavascriptVoiceStartModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
VoiceStartModulator(mc, id, voiceAmount, m),
Modulation(m),
JavascriptProcessor(mc),
ProcessorWithScriptingContent(mc)
{
	onInitCallback = new SnippetDocument("onInit");
	onVoiceStartCallback = new SnippetDocument("onVoiceStart", "voiceIndex");
	onVoiceStopCallback = new SnippetDocument("onVoiceStop", "voiceIndex");
	onControllerCallback = new SnippetDocument("onController");
	onControlCallback = new SnippetDocument("onControl", "number value");

	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onVoiceStartOpen");
	editorStateIdentifiers.add("onVoiceStopOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");
}

JavascriptVoiceStartModulator::~JavascriptVoiceStartModulator()
{
#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif
}

Path JavascriptVoiceStartModulator::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

ProcessorEditorBody * JavascriptVoiceStartModulator::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

void JavascriptVoiceStartModulator::handleHiseEvent(const HiseEvent& m)
{
	currentMidiMessage->setHiseEvent(m);

	if (m.isNoteOn())
	{
		synthObject->increaseNoteCounter(m.getNoteNumber());
	}
	else if (m.isNoteOff())
	{
		synthObject->decreaseNoteCounter(m.getNoteNumber());

		if (!onVoiceStopCallback->isSnippetEmpty())
		{
			ScopedReadLock sl(mainController->getCompileLock());
			scriptEngine->setCallbackParameter(onVoiceStop, 0, 0);
			scriptEngine->executeCallback(onVoiceStop, &lastResult);

			BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onVoiceStopCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
		}
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onControllerCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
}

void JavascriptVoiceStartModulator::startVoice(int voiceIndex)
{
	if (!onVoiceStartCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());

		synthObject->setVoiceGainValue(voiceIndex, 1.0f);
		synthObject->setVoicePitchValue(voiceIndex, 1.0f);
		scriptEngine->setCallbackParameter(onVoiceStart, 0, voiceIndex);
		unsavedValue = (float)scriptEngine->executeCallback(onVoiceStart, &lastResult);
	}

	VoiceStartModulator::startVoice(voiceIndex);
}


JavascriptProcessor::SnippetDocument * JavascriptVoiceStartModulator::getSnippet(int c)
{
	switch (c)
	{
	case Callback::onInit:		 return onInitCallback;
	case Callback::onVoiceStart: return onVoiceStartCallback;
	case Callback::onVoiceStop:	 return onVoiceStopCallback;
	case Callback::onController: return onControllerCallback;
	case Callback::onControl:	 return onControlCallback;
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptVoiceStartModulator::getSnippet(int c) const
{
	switch (c)
	{
	case Callback::onInit:		 return onInitCallback;
	case Callback::onVoiceStart: return onVoiceStartCallback;
	case Callback::onVoiceStop:	 return onVoiceStopCallback;
	case Callback::onController: return onControllerCallback;
	case Callback::onControl:	 return onControlCallback;
	}

	return nullptr;
}

void JavascriptVoiceStartModulator::registerApiClasses()
{
	content = new ScriptingApi::Content(this);

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::ModulatorApi(this));
	scriptEngine->registerApiClass(synthObject);
}



JavascriptTimeVariantModulator::JavascriptTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m) :
TimeVariantModulator(mc, id, m),
Modulation(m),
JavascriptProcessor(mc),
ProcessorWithScriptingContent(mc),
buffer(new VariantBuffer(0))
{
	onInitCallback = new SnippetDocument("onInit");
	prepareToPlayCallback = new SnippetDocument("prepareToPlay", "sampleRate samplesPerBlock");
	processBlockCallback = new SnippetDocument("processBlock", "buffer");
	onNoteOnCallback = new SnippetDocument("onNoteOn");
	onNoteOffCallback = new SnippetDocument("onNoteOff");
	onControllerCallback = new SnippetDocument("onController");
	onControlCallback = new SnippetDocument("onControl", "number value");

	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("prepareToPlayOpen");
	editorStateIdentifiers.add("processBlockOpen");
	editorStateIdentifiers.add("onNoteOnOpen");
	editorStateIdentifiers.add("onNoteOffOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");

}

JavascriptTimeVariantModulator::~JavascriptTimeVariantModulator()
{
	ScopedWriteLock sl(mainController->getCompileLock());

	onInitCallback = new SnippetDocument("onInit");
	prepareToPlayCallback = new SnippetDocument("prepareToPlay", "sampleRate samplesPerBlock");
	processBlockCallback = new SnippetDocument("processBlock", "buffer");
	onNoteOnCallback = new SnippetDocument("onNoteOn");
	onNoteOffCallback = new SnippetDocument("onNoteOff");
	onControllerCallback = new SnippetDocument("onController");
	onControlCallback = new SnippetDocument("onControl", "number value");
	
	bufferVar = var::undefined();
	buffer = nullptr;

#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif
}

Path JavascriptTimeVariantModulator::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

ProcessorEditorBody * JavascriptTimeVariantModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

void JavascriptTimeVariantModulator::handleHiseEvent(const HiseEvent &m)
{
	currentMidiMessage->setHiseEvent(m);

	if (m.isNoteOn())
	{
		synthObject->increaseNoteCounter(m.getNoteNumber());

		if (!onNoteOnCallback->isSnippetEmpty())
		{
			ScopedReadLock sl(mainController->getCompileLock());
			scriptEngine->executeCallback(onNoteOn, &lastResult);
		}

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onNoteOnCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
	else if (m.isNoteOff())
	{
		synthObject->decreaseNoteCounter(m.getNoteNumber());

		if (!onNoteOffCallback->isSnippetEmpty())
		{
			ScopedReadLock sl(mainController->getCompileLock());
			scriptEngine->executeCallback(onNoteOff, &lastResult);
		}

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onNoteOffCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onControllerCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}


}

void JavascriptTimeVariantModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);

	buffer->referToData(internalBuffer.getWritePointer(0), samplesPerBlock);
	bufferVar = var(buffer);

	if (!prepareToPlayCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());

		scriptEngine->setCallbackParameter(Callback::prepare, 0, sampleRate);
		scriptEngine->setCallbackParameter(Callback::prepare, 1, samplesPerBlock);
		scriptEngine->executeCallback(Callback::prepare, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, prepareToPlayCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
}

void JavascriptTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
	if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		buffer->referToData(internalBuffer.getWritePointer(0, startSample), numSamples);

		ScopedReadLock sl(mainController->getCompileLock());

		scriptEngine->setCallbackParameter(Callback::processBlock, 0, bufferVar);
		scriptEngine->executeCallback(Callback::processBlock, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, processBlockCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}

#if ENABLE_ALL_PEAK_METERS
	setOutputValue(internalBuffer.getSample(0, startSample));
#endif

}

JavascriptProcessor::SnippetDocument* JavascriptTimeVariantModulator::getSnippet(int c)
{
	switch (c)
	{
	case Callback::onInit: return onInitCallback;
	case Callback::prepare: return prepareToPlayCallback;
	case Callback::processBlock: return processBlockCallback;
	case Callback::onNoteOn: return onNoteOnCallback;
	case Callback::onNoteOff: return onNoteOffCallback;
	case Callback::onController: return onControllerCallback;
	case Callback::onControl: return onControlCallback;
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptTimeVariantModulator::getSnippet(int c) const
{
	switch (c)
	{
	case Callback::onInit: return onInitCallback;
	case Callback::prepare: return prepareToPlayCallback;
	case Callback::processBlock: return processBlockCallback;
	case Callback::onNoteOn: return onNoteOnCallback;
	case Callback::onNoteOff: return onNoteOffCallback;
	case Callback::onController: return onControllerCallback;
	case Callback::onControl: return onControlCallback;
	}

	return nullptr;
}

void JavascriptTimeVariantModulator::registerApiClasses()
{
	content = new ScriptingApi::Content(this);

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::ModulatorApi(this));
	scriptEngine->registerApiClass(synthObject);

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(getMainController()));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}


void JavascriptTimeVariantModulator::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getBlockSize());
}



JavascriptEnvelopeModulator::JavascriptEnvelopeModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
ProcessorWithScriptingContent(mc),
JavascriptProcessor(mc),
EnvelopeModulator(mc, id, numVoices, m),
Modulation(m),
buffer(new VariantBuffer(0))
{
	onInitCallback = new SnippetDocument("onInit");
	prepareToPlayCallback = new SnippetDocument("prepareToPlay", "sampleRate samplesPerBlock");
	renderVoiceCallback = new SnippetDocument("renderVoice", "voiceIndex uptime data");
	startVoiceCallback = new SnippetDocument("startVoice", "voiceIndex");
	stopVoiceCallback = new SnippetDocument("stopVoice", "voiceIndex");
	onNoteOnCallback = new SnippetDocument("onNoteOn");
	onNoteOffCallback = new SnippetDocument("onNoteOff");
	onControllerCallback = new SnippetDocument("onController");
	onControlCallback = new SnippetDocument("onControl", "number value");

	for (int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("prepareToPlayOpen");
	editorStateIdentifiers.add("renderVoiceOpen");
	editorStateIdentifiers.add("startVoiceVoiceOpen");
	editorStateIdentifiers.add("stopVoiceOpen");
	editorStateIdentifiers.add("onNoteOnOpen");
	editorStateIdentifiers.add("onNoteOffOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");
}



JavascriptEnvelopeModulator::~JavascriptEnvelopeModulator()
{

}

Path JavascriptEnvelopeModulator::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

ProcessorEditorBody * JavascriptEnvelopeModulator::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

void JavascriptEnvelopeModulator::handleHiseEvent(const HiseEvent &m)
{
	currentMidiMessage->setHiseEvent(m);

	if (m.isNoteOn())
	{
		synthObject->increaseNoteCounter(m.getNoteNumber());

		if (!onNoteOnCallback->isSnippetEmpty())
		{
			ScopedReadLock sl(mainController->getCompileLock());
			scriptEngine->executeCallback(onNoteOn, &lastResult);
		}

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onNoteOnCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
	else if (m.isNoteOff())
	{
		synthObject->decreaseNoteCounter(m.getNoteNumber());

		if (!onNoteOffCallback->isSnippetEmpty())
		{
			ScopedReadLock sl(mainController->getCompileLock());
			scriptEngine->executeCallback(onNoteOff, &lastResult);
		}

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onNoteOffCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, onControllerCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}

}

void JavascriptEnvelopeModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	buffer->referToData(internalBuffer.getWritePointer(0), samplesPerBlock);
	bufferVar = var(buffer);

	if (!prepareToPlayCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());

		scriptEngine->setCallbackParameter(Callback::prepare, 0, sampleRate);
		scriptEngine->setCallbackParameter(Callback::prepare, 1, samplesPerBlock);
		scriptEngine->executeCallback(Callback::prepare, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, prepareToPlayCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}
}

void JavascriptEnvelopeModulator::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = polyManager.getCurrentVoice();
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);


	if (!renderVoiceCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		buffer->referToData(internalBuffer.getWritePointer(0, startSample), numSamples);

		ScopedReadLock sl(mainController->getCompileLock());

		scriptEngine->setCallbackParameter(Callback::renderVoice, 0, voiceIndex);
		scriptEngine->setCallbackParameter(Callback::renderVoice, 1, state->uptime);
		scriptEngine->setCallbackParameter(Callback::renderVoice, 2, bufferVar);
		state->isPlaying = scriptEngine->executeCallback(Callback::renderVoice, &lastResult);

		state->uptime += (float)numSamples;

		if (!state->isPlaying) 
			reset(voiceIndex);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, renderVoiceCallback->getCallbackName().toString() + ": " + lastResult.getErrorMessage()));
	}

#if ENABLE_ALL_PEAK_METERS
	setOutputValue(internalBuffer.getSample(0, startSample));
#endif

}

void JavascriptEnvelopeModulator::startVoice(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);

	state->uptime = 0.0f;
	state->isPlaying = true;
	state->isRingingOff = false;

	if (!startVoiceCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());

		scriptEngine->setCallbackParameter(onStartVoice, 0, voiceIndex);
		scriptEngine->executeCallback(onStartVoice, &lastResult);
	}
}

void JavascriptEnvelopeModulator::stopVoice(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);
	state->isRingingOff = true;

	if (!startVoiceCallback->isSnippetEmpty())
	{
		ScopedReadLock sl(mainController->getCompileLock());

		scriptEngine->setCallbackParameter(onStopVoice, 0, voiceIndex);
		scriptEngine->executeCallback(onStopVoice, &lastResult);
	}
}

void JavascriptEnvelopeModulator::reset(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);

	state->uptime = 0.0f;
	state->isPlaying = false;
	state->isRingingOff = false;
}

bool JavascriptEnvelopeModulator::isPlaying(int voiceIndex) const
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);

	return state->isPlaying;
}

JavascriptProcessor::SnippetDocument * JavascriptEnvelopeModulator::getSnippet(int c)
{
	Callback cb = (Callback)c;

	switch (cb)
	{
	case JavascriptEnvelopeModulator::onInit: return onInitCallback;
	case JavascriptEnvelopeModulator::prepare: return prepareToPlayCallback;
	case JavascriptEnvelopeModulator::renderVoice: return renderVoiceCallback;
	case JavascriptEnvelopeModulator::onStartVoice: return startVoiceCallback;
	case JavascriptEnvelopeModulator::onStopVoice: return stopVoiceCallback;
	case JavascriptEnvelopeModulator::onNoteOn: return onNoteOnCallback;
	case JavascriptEnvelopeModulator::onNoteOff: return onNoteOffCallback;
	case JavascriptEnvelopeModulator::onController: return onControllerCallback;
	case JavascriptEnvelopeModulator::onControl: return onControlCallback;
	case JavascriptEnvelopeModulator::numCallbacks:
	default:
		break;
	}

	jassertfalse;
	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptEnvelopeModulator::getSnippet(int c) const
{
	Callback cb = (Callback)c;

	switch (cb)
	{
	case JavascriptEnvelopeModulator::onInit: return onInitCallback;
	case JavascriptEnvelopeModulator::prepare: return prepareToPlayCallback;
	case JavascriptEnvelopeModulator::renderVoice: return renderVoiceCallback;
	case JavascriptEnvelopeModulator::onStartVoice: return startVoiceCallback;
	case JavascriptEnvelopeModulator::onStopVoice: return stopVoiceCallback;
	case JavascriptEnvelopeModulator::onNoteOn: return onNoteOnCallback;
	case JavascriptEnvelopeModulator::onNoteOff: return onNoteOffCallback;
	case JavascriptEnvelopeModulator::onController: return onControllerCallback;
	case JavascriptEnvelopeModulator::onControl: return onControlCallback;
	case JavascriptEnvelopeModulator::numCallbacks:
	default:
		break;
	}

	jassertfalse;
	return nullptr;
}

void JavascriptEnvelopeModulator::registerApiClasses()
{
	content = new ScriptingApi::Content(this);

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::ModulatorApi(this));
	scriptEngine->registerApiClass(synthObject);

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(getMainController()));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptEnvelopeModulator::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getBlockSize());
}

class JavascriptModulatorSynth::Sound : public ModulatorSynthSound
{
public:
	Sound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};



class JavascriptModulatorSynth::Voice : public ModulatorSynthVoice
{
public:

	Voice(ModulatorSynth *ownerSynth) :
		ModulatorSynthVoice(ownerSynth)
	{

		leftBuffer = new VariantBuffer(0);
		rightBuffer = new VariantBuffer(0);
		pitchData = new VariantBuffer(0);

		channels.add(var(leftBuffer));
		channels.add(var(rightBuffer));
		channels.add(var(pitchData));
	};

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override
	{
		ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

		JavascriptModulatorSynth* jms = static_cast<JavascriptModulatorSynth*>(getOwnerSynth());

		ScopedReadLock sl(jms->mainController->getCompileLock());

		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::startVoice, 0, getVoiceIndex());
		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::startVoice, 1, midiNoteNumber);
		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::startVoice, 2, velocity);

		voiceUptime = 0.0;
		uptimeDelta = (double)jms->scriptEngine->executeCallback((int)JavascriptModulatorSynth::Callback::startVoice, &jms->lastResult);

		BACKEND_ONLY(if (!jms->lastResult.wasOk()) debugError(jms, jms->startVoiceCallback->getCallbackName().toString() + ": " + jms->lastResult.getErrorMessage()));
	}

	void calculateBlock(int startSample, int numSamples) override
	{
		const int startIndex = startSample;
		const int samplesToCopy = numSamples;

		const float *voicePitchValues = getVoicePitchValues();
		const float *modValues = getVoiceGainValues(startSample, numSamples);

		float *leftValues = voiceBuffer.getWritePointer(0, startSample);
		float *rightValues = voiceBuffer.getWritePointer(1, startSample);

		channels[0].getBuffer()->referToData(leftValues, numSamples);
		channels[1].getBuffer()->referToData(rightValues, numSamples);
		
		if (voicePitchValues != nullptr)
		{
			voicePitchValues += startSample;
			channels[2].getBuffer()->referToData(const_cast<float*>(voicePitchValues), numSamples);
		}
        else
        {
            channels[2].getBuffer()->referToData(rightValues, numSamples);
        }
		
		JavascriptModulatorSynth* jms = static_cast<JavascriptModulatorSynth*>(getOwnerSynth());

		ScopedReadLock sl(jms->getMainController()->getCompileLock());

		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::renderVoice, 0, getVoiceIndex());
		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::renderVoice, 1, var(channels));

		voiceUptime += (double)jms->scriptEngine->executeCallback((int)JavascriptModulatorSynth::Callback::renderVoice, &jms->lastResult);

		BACKEND_ONLY(if (!jms->lastResult.wasOk()) debugError(jms, jms->renderVoiceCallback->getCallbackName().toString() + ": " + jms->lastResult.getErrorMessage()));

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
	}

	Array<var> channels;

	VariantBuffer::Ptr leftBuffer;
	VariantBuffer::Ptr rightBuffer;
	VariantBuffer::Ptr pitchData;
};


JavascriptModulatorSynth::JavascriptModulatorSynth(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices),
JavascriptProcessor(mc),
ProcessorWithScriptingContent(mc),
scriptChain1(new ModulatorChain(mc, "Script Chain 1", numVoices, Modulation::GainMode, this)),
scriptChain2(new ModulatorChain(mc, "Script Chain 2", numVoices, Modulation::GainMode, this)),
onInitCallback(new SnippetDocument("onInit")),
prepareToPlayCallback(new SnippetDocument("prepareToPlay", "sampleRate blockSize")),
startVoiceCallback(new SnippetDocument("onStartVoice", "voiceIndex noteNumber velocity")),
renderVoiceCallback(new SnippetDocument("renderVoice", "voiceIndex channels")),
onNoteOnCallback(new SnippetDocument("onNoteOn")),
onNoteOffCallback(new SnippetDocument("onNoteOff")),
onControllerCallback(new SnippetDocument("onController")),
onControlCallback(new SnippetDocument("onControl", "number value"))
{

	scriptChain1->setColour(Colour(0xFF666666));
	scriptChain2->setColour(Colour(0xFF666666));

	editorStateIdentifiers.add("ScriptChain1Shown");
	editorStateIdentifiers.add("ScriptChain2Shown");
	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("prepareToPlayOpen");
	editorStateIdentifiers.add("startVoiceOpen");
	editorStateIdentifiers.add("renderVoiceOpen");
	editorStateIdentifiers.add("onNoteOnOpen");
	editorStateIdentifiers.add("onNoteOffOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");

	for (int i = 0; i < editorStateIdentifiers.size(); i++)
	{
		DBG(editorStateIdentifiers[i].toString());
	}

	addSound(new Sound());

	for (int i = 0; i < numVoices; i++)
	{
		addVoice(new Voice(this));
	}
}

JavascriptModulatorSynth::~JavascriptModulatorSynth()
{
#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif

}

Processor * JavascriptModulatorSynth::getChildProcessor(int processorIndex)
{
	switch (processorIndex)
	{
	
	case ModulatorSynth::InternalChains::MidiProcessor:	 return midiProcessorChain;
	case ModulatorSynth::InternalChains::GainModulation: return gainChain;
	case ModulatorSynth::InternalChains::PitchModulation:return pitchChain;
	case ModulatorSynth::InternalChains::EffectChain:	 return effectChain;
	case ScriptChain1:	return scriptChain1;
	case ScriptChain2:	return scriptChain2;
	default:			jassertfalse; return nullptr;
	}
}

const Processor * JavascriptModulatorSynth::getChildProcessor(int processorIndex) const
{
	switch (processorIndex)
	{
	case ModulatorSynth::InternalChains::MidiProcessor:	 return midiProcessorChain;
	case ModulatorSynth::InternalChains::GainModulation: return gainChain;
	case ModulatorSynth::InternalChains::PitchModulation:return pitchChain;
	case ModulatorSynth::InternalChains::EffectChain:	 return effectChain;
	case ScriptChain1:	return scriptChain1;
	case ScriptChain2:	return scriptChain2;
	default:			jassertfalse; return nullptr;
	}
}

void JavascriptModulatorSynth::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	if (newSampleRate > -1.0)
	{
		scriptChain1Buffer = AudioSampleBuffer(1, samplesPerBlock);
		scriptChain2Buffer = AudioSampleBuffer(1, samplesPerBlock);
		scriptChain1->prepareToPlay(newSampleRate, samplesPerBlock);
		scriptChain2->prepareToPlay(newSampleRate, samplesPerBlock);

		for (int i = 0; i < sounds.size(); i++)
		{
			//static_cast<WavetableSound*>(getSound(i))->calculatePitchRatio(sampleRate); // Todo
		}
	}


	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);
}

void JavascriptModulatorSynth::preHiseEventCallback(const HiseEvent& m)
{
	scriptChain1->handleHiseEvent(m);
	scriptChain2->handleHiseEvent(m);

	ModulatorSynth::preHiseEventCallback(m);
}

void JavascriptModulatorSynth::preStartVoice(int voiceIndex, int noteNumber)
{
	ModulatorSynth::preStartVoice(voiceIndex, noteNumber);

	scriptChain1->startVoice(voiceIndex);
	scriptChain2->startVoice(voiceIndex);
}

void JavascriptModulatorSynth::preVoiceRendering(int startSample, int numThisTime)
{
	scriptChain1->renderNextBlock(scriptChain1Buffer, startSample, numThisTime);
	scriptChain2->renderNextBlock(scriptChain2Buffer, startSample, numThisTime);

	ModulatorSynth::preVoiceRendering(startSample, numThisTime);

	if (!isChainDisabled(EffectChain)) effectChain->preRenderCallback(startSample, numThisTime);
}

void JavascriptModulatorSynth::calculateScriptChainValuesForVoice(int voiceIndex, int startSample, int numSamples)
{
	scriptChain1->renderVoice(voiceIndex, startSample, numSamples);
	scriptChain2->renderVoice(voiceIndex, startSample, numSamples);

	float *scriptChain1Values = scriptChain1->getVoiceValues(voiceIndex);
	float *scriptChain2Values = scriptChain2->getVoiceValues(voiceIndex);

	const float* timeVariantScriptChain1Values = scriptChain1Buffer.getReadPointer(0);
	const float* timeVariantScriptChain2Values = scriptChain2Buffer.getReadPointer(0);

	FloatVectorOperations::multiply(scriptChain1Values, timeVariantScriptChain1Values, startSample + numSamples);
	FloatVectorOperations::multiply(scriptChain2Values, timeVariantScriptChain2Values, startSample + numSamples);
}

const float * JavascriptModulatorSynth::getScriptChainValues(int chainIndex, int voiceIndex) const
{
	return chainIndex == 0 ? scriptChain1->getVoiceValues(voiceIndex) : scriptChain2->getVoiceValues(voiceIndex);
}

float JavascriptModulatorSynth::getAttribute(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

	return getControlValue(parameterIndex);
}

void JavascriptModulatorSynth::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	setControlValue(parameterIndex, newValue);
}

JavascriptProcessor::SnippetDocument * JavascriptModulatorSynth::getSnippet(int c)
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case JavascriptModulatorSynth::Callback::onInit:		return onInitCallback;
	case JavascriptModulatorSynth::Callback::prepareToPlay: return prepareToPlayCallback;
	case JavascriptModulatorSynth::Callback::startVoice:	return startVoiceCallback;
	case JavascriptModulatorSynth::Callback::renderVoice:	return renderVoiceCallback;
	case JavascriptModulatorSynth::Callback::onNoteOn:		return onNoteOnCallback;
	case JavascriptModulatorSynth::Callback::onNoteOff:		return onNoteOffCallback;
	case JavascriptModulatorSynth::Callback::onController:	return onControllerCallback;
	case JavascriptModulatorSynth::Callback::onControl:		return onControlCallback;
	case JavascriptModulatorSynth::Callback::numCallbacks:
	default:												break;
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptModulatorSynth::getSnippet(int c) const
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case JavascriptModulatorSynth::Callback::onInit:		return onInitCallback;
	case JavascriptModulatorSynth::Callback::prepareToPlay: return prepareToPlayCallback;
	case JavascriptModulatorSynth::Callback::startVoice:	return startVoiceCallback;
	case JavascriptModulatorSynth::Callback::renderVoice:	return renderVoiceCallback;
	case JavascriptModulatorSynth::Callback::onNoteOn:		return onNoteOnCallback;
	case JavascriptModulatorSynth::Callback::onNoteOff:		return onNoteOffCallback;
	case JavascriptModulatorSynth::Callback::onController:	return onControllerCallback;
	case JavascriptModulatorSynth::Callback::onControl:		return onControlCallback;
	case JavascriptModulatorSynth::Callback::numCallbacks:
	default:												break;
	}

	return nullptr;
}

void JavascriptModulatorSynth::registerApiClasses()
{
	content = new ScriptingApi::Content(this);

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, this);

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(synthObject);

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(getMainController()));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}



void JavascriptModulatorSynth::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getBlockSize());
}

ProcessorEditorBody* JavascriptModulatorSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}
