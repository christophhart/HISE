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
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onNoteOnOpen");
	editorStateIdentifiers.add("onNoteOffOpen");
	editorStateIdentifiers.add("onControllerOpen");
	editorStateIdentifiers.add("onTimerOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("externalPopupShown");
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


void JavascriptMidiProcessor::processMidiMessage(MidiMessage &m)
{
	if (isDeferred())
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
		ADD_GLITCH_DETECTOR("Processing " + getId() + " script callbacks");

		if (currentMidiMessage != nullptr)
		{
			currentMessage = m;
			currentMidiMessage->setMidiMessage(&m);
			currentMidiMessage->ignoreEvent(false);

			runScriptCallbacks();

			processThisMessage = !currentMidiMessage->ignored;
		}
	}


};


void JavascriptMidiProcessor::registerApiClasses()
{
	content = new ScriptingApi::Content(this);

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, getOwnerSynth());
	samplerObject = new ScriptingApi::Sampler(this, dynamic_cast<ModulatorSampler*>(getOwnerSynth()));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(synthObject);
	scriptEngine->registerApiClass(samplerObject);
}



void JavascriptMidiProcessor::runScriptCallbacks()
{
	ScopedLock sl(compileLock);

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.03);

	if (currentMessage.isNoteOn())
	{
		synthObject->increaseNoteCounter();
		if (onNoteOnCallback->isSnippetEmpty()) return;

		Result r = Result::ok();

		scriptEngine->executeCallback(onNoteOn, &r);

		if (!r.wasOk()) debugError(this, r.getErrorMessage());

	}
	else if (currentMessage.isNoteOff())
	{
		synthObject->decreaseNoteCounter();

		if (onNoteOffCallback->isSnippetEmpty()) return;
		Result r = Result::ok();
		scriptEngine->executeCallback(onNoteOff, &r);

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if (currentMessage.isController() || currentMessage.isPitchWheel() || currentMessage.isAftertouch())
	{
		if (currentMessage.isControllerOfType(64))
		{
			synthObject->setSustainPedal(currentMessage.getControllerValue() > 64);
		}

		if (onControllerCallback->isSnippetEmpty()) return;

		// All notes off are controller message, so they should not be processed, or it can lead to loop.
		if (currentMessage.isAllNotesOff()) return;

		Result r = Result::ok();
		scriptEngine->executeCallback(onController, &r);

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if (currentMessage.isSongPositionPointer())
	{
		Result r = Result::ok();

		static const Identifier onClock("onClock");

		var args[1] = { currentMessage.getSongPositionPointerMidiBeat() };
		scriptEngine->executeWithoutAllocation(onClock, var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);

		if (!r.wasOk()) debugError(this, r.getErrorMessage());
	}
	else if (currentMessage.isMidiStart() || currentMessage.isMidiStop())
	{
		Result r = Result::ok();

		static const Identifier onClock("onTransport");

		var args[1] = { currentMessage.isMidiStart() };

		scriptEngine->executeWithoutAllocation(onClock, var::NativeFunctionArgs(dynamic_cast<ReferenceCountedObject*>(this), args, 1), &r);
	}
}


void JavascriptMidiProcessor::runTimerCallback(int /*offsetInBuffer*//*=-1*/)
{
	if (isBypassed() || onTimerCallback->isSnippetEmpty()) return;

	ScopedLock sl(compileLock);

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.002);

	Result r = Result::ok();
	scriptEngine->executeCallback(onTimer, &r);

	if (isDeferred())
	{
		sendSynchronousChangeMessage();
	}

#if USE_BACKEND
	if (!r.wasOk()) getMainController()->writeToConsole(r.getErrorMessage(), 1, this, content->getColour());
#endif
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

	if (deferredMidiMessages.getNumEvents() != 0)
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

	while (iter.getNextEvent(m, samplePos))
	{
		currentMessage = m;
		currentMidiMessage->setMidiMessage(&m);


		runScriptCallbacks();
	}

	copyBuffer.clear();
	deferredUpdatePending = false;

}

void JavascriptMidiProcessor::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	if (onControlCallback->isSnippetEmpty())
	{
		return;
	}

	Result r = Result::ok();

	scriptEngine->maximumExecutionTime = RelativeTime(0.5);
	scriptEngine->setCallbackParameter(onControl, 0, component);
	scriptEngine->setCallbackParameter(onControl, 1, controllerValue);
	scriptEngine->executeCallback(onControl, &r);

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		sendSynchronousChangeMessage();
	}
	else
	{
		sendChangeMessage();
	}


#if USE_BACKEND
	if (!r.wasOk()) debugError(this, r.getErrorMessage());
#endif
}




JavascriptMasterEffect::JavascriptMasterEffect(MainController *mc, const String &id):
JavascriptProcessor(mc),
ScriptBaseMasterEffectProcessor(mc, id),
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

ValueTree JavascriptMasterEffect::exportAsValueTree() const
{
	ValueTree v = ScriptBaseMasterEffectProcessor::exportAsValueTree();
	saveScript(v);

	return v;
}

void JavascriptMasterEffect::restoreFromValueTree(const ValueTree &v)
{
	restoreScript(v);
	ScriptBaseMasterEffectProcessor::restoreFromValueTree(v);
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
	Callbacks ca = (Callbacks)c;

	switch (ca)
	{
	case ScriptBaseMasterEffectProcessor::onInit:		 return onInitCallback;
	case ScriptBaseMasterEffectProcessor::prepareToPlay: return prepareToPlayCallback;
	case ScriptBaseMasterEffectProcessor::processBlock:	 return processBlockCallback;
	case ScriptBaseMasterEffectProcessor::onControl:	 return onControlCallback;
	case ScriptBaseMasterEffectProcessor::numCallbacks:  return nullptr;
	default:
		break;
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptMasterEffect::getSnippet(int c) const
{
	Callbacks ca = (Callbacks)c;

	switch (ca)
	{
	case ScriptBaseMasterEffectProcessor::onInit:		 return onInitCallback;
	case ScriptBaseMasterEffectProcessor::prepareToPlay: return prepareToPlayCallback;
	case ScriptBaseMasterEffectProcessor::processBlock:	 return processBlockCallback;
	case ScriptBaseMasterEffectProcessor::onControl:	 return onControlCallback;
	case ScriptBaseMasterEffectProcessor::numCallbacks:  return nullptr;
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

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader());
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));

}

void JavascriptMasterEffect::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	if (onControlCallback->isSnippetEmpty())
	{
		return;
	}

	Result r = Result::ok();

	scriptEngine->maximumExecutionTime = RelativeTime(0.5);

	ScopedLock sl(compileLock);

	scriptEngine->setCallbackParameter(onControl, 0, component);
	scriptEngine->setCallbackParameter(onControl, 1, controllerValue);
	scriptEngine->executeCallback(onControl, &r);

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		sendSynchronousChangeMessage();
	}
	else
	{
		sendChangeMessage();
	}

#if USE_BACKEND
	if (!r.wasOk()) debugError(this, r.getErrorMessage());
#endif
}

void JavascriptMasterEffect::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getBlockSize());
}

void JavascriptMasterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ScopedLock sl(compileLock);

	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	

	Array<var> channelArray;

	bufferL = new VariantBuffer(0);
	bufferR = new VariantBuffer(1);

	channelArray.add(var(bufferL));
	channelArray.add(var(bufferR));

	channels = var(channelArray);

	if (!prepareToPlayCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		scriptEngine->setCallbackParameter(Callbacks::prepareToPlay, 0, sampleRate);
		scriptEngine->setCallbackParameter(Callbacks::prepareToPlay, 1, samplesPerBlock);
		scriptEngine->executeCallback(Callbacks::prepareToPlay, &lastResult);

#if USE_BACKEND
		if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
	}
}

void JavascriptMasterEffect::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		ScopedLock sl(compileLock);

		float *l = b.getWritePointer(0, startSample);
		float *r = b.getWritePointer(1, startSample);

		bufferL->referToData(l, numSamples);
		bufferR->referToData(r, numSamples);

		scriptEngine->setCallbackParameter(Callbacks::processBlock, 0, channels);
		scriptEngine->executeCallback(Callbacks::processBlock, &lastResult);

#if USE_BACKEND
		if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
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

void JavascriptVoiceStartModulator::handleMidiEvent(const MidiMessage &m)
{
	currentMidiMessage->setMidiMessage(&m);

	if (m.isNoteOn())
	{
		synthObject->increaseNoteCounter();
	}
	else if (m.isNoteOff())
	{
		synthObject->decreaseNoteCounter();

		if (!onVoiceStopCallback->isSnippetEmpty())
		{
			ScopedLock sl(compileLock);
			scriptEngine->setCallbackParameter(onVoiceStop, 0, 0);
			scriptEngine->executeCallback(onVoiceStop, &lastResult);
		}
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		ScopedLock sl(compileLock);
		scriptEngine->executeCallback(onController, &lastResult);
	}

#if USE_BACKEND
	if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
}

void JavascriptVoiceStartModulator::startVoice(int voiceIndex)
{
	if (!onVoiceStartCallback->isSnippetEmpty())
	{
		ScopedLock sl(compileLock);

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

void JavascriptVoiceStartModulator::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	if (onControlCallback->isSnippetEmpty())
	{
		return;
	}

	scriptEngine->maximumExecutionTime = RelativeTime(0.5);

	ScopedLock sl(compileLock);

	scriptEngine->setCallbackParameter(onControl, 0, component);
	scriptEngine->setCallbackParameter(onControl, 1, controllerValue);
	scriptEngine->executeCallback(onControl, &lastResult);

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		sendSynchronousChangeMessage();
	}
	else
	{
		sendChangeMessage();
	}

#if USE_BACKEND
	if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
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
	ScopedLock sl(compileLock);

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

void JavascriptTimeVariantModulator::handleMidiEvent(const MidiMessage &m)
{
	currentMidiMessage->setMidiMessage(&m);

	if (m.isNoteOn())
	{
		synthObject->increaseNoteCounter();

		if (!onNoteOnCallback->isSnippetEmpty())
		{
			ScopedLock sl(compileLock);
			scriptEngine->executeCallback(onNoteOn, &lastResult);
		}
	}
	else if (m.isNoteOff())
	{
		synthObject->decreaseNoteCounter();

		if (!onNoteOffCallback->isSnippetEmpty())
		{
			ScopedLock sl(compileLock);
			scriptEngine->executeCallback(onNoteOff, &lastResult);
		}
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		ScopedLock sl(compileLock);
		scriptEngine->executeCallback(onController, &lastResult);
	}

#if USE_BACKEND
	if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
}

void JavascriptTimeVariantModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);


	buffer->referToData(internalBuffer.getWritePointer(0), samplesPerBlock);
	bufferVar = var(buffer);

	if (!prepareToPlayCallback->isSnippetEmpty())
	{
		ScopedLock sl(compileLock);

		scriptEngine->setCallbackParameter(Callback::prepare, 0, sampleRate);
		scriptEngine->setCallbackParameter(Callback::prepare, 1, samplesPerBlock);
		scriptEngine->executeCallback(Callback::prepare, &lastResult);

#if USE_BACKEND
		if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
	}
}

void JavascriptTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
	if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		buffer->referToData(internalBuffer.getWritePointer(0, startSample), numSamples);

		ScopedLock sl(compileLock);

		scriptEngine->setCallbackParameter(Callback::processBlock, 0, bufferVar);
		scriptEngine->executeCallback(Callback::processBlock, &lastResult);

#if USE_BACKEND
		if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
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

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader());
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptTimeVariantModulator::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	if (onControlCallback->isSnippetEmpty())
	{
		return;
	}

	scriptEngine->maximumExecutionTime = RelativeTime(0.5);

	ScopedLock sl(compileLock);

	scriptEngine->setCallbackParameter(Callback::onControl, 0, component);
	scriptEngine->setCallbackParameter(Callback::onControl, 1, controllerValue);
	scriptEngine->executeCallback(Callback::onControl, &lastResult);

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		sendSynchronousChangeMessage();
	}
	else
	{
		sendChangeMessage();
	}

#if USE_BACKEND
	if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage());
#endif
}

void JavascriptTimeVariantModulator::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getBlockSize());
}
