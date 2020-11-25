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

namespace hise { using namespace juce;

JavascriptMidiProcessor::JavascriptMidiProcessor(MainController *mc, const String &id) :
ScriptBaseMidiProcessor(mc, id),
JavascriptProcessor(mc),
onInitCallback(new SnippetDocument("onInit")),
onNoteOnCallback(new SnippetDocument("onNoteOn")),
onNoteOffCallback(new SnippetDocument("onNoteOff")),
onControllerCallback(new SnippetDocument("onController")),
onTimerCallback(new SnippetDocument("onTimer")),
onControlCallback(new SnippetDocument("onControl", "number value")),
front(false),
deferred(false),
deferredExecutioner(this),
deferredUpdatePending(false)
{
	initContent();

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
	cleanupEngine();
	clearExternalWindows();

	onInitCallback = nullptr;
	onNoteOnCallback = nullptr;
	onNoteOffCallback = nullptr;
	onControllerCallback = nullptr;
	onTimerCallback = nullptr;
	onControlCallback = nullptr;

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
		deferredExecutioner.addPendingEvent(m);
	}
	else
	{
		ADD_GLITCH_DETECTOR(this, DebugLogger::Location::ScriptMidiEventCallback);

		if (currentMidiMessage != nullptr)
		{
			ScopedValueSetter<HiseEvent*> svs(currentEvent, &m);
			currentMidiMessage->setHiseEvent(m);
			runScriptCallbacks();
		}
	}

}

void JavascriptMidiProcessor::registerApiClasses()
{
	

	//content = new ScriptingApi::Content(this);
    front = false;

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, getOwnerSynth());

	scriptEngine->registerApiClass(new ScriptingApi::ModuleIds(getOwnerSynth()));

	samplerObject = new ScriptingApi::Sampler(this, dynamic_cast<ModulatorSampler*>(getOwnerSynth()));

	scriptEngine->registerNativeObject("Content", getScriptingContent());
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::FileSystem(this));
	scriptEngine->registerApiClass(new ScriptingApi::Server(this));
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::Colours());
	scriptEngine->registerApiClass(new ScriptExpansionHandler(this));
	scriptEngine->registerApiClass(synthObject);
	scriptEngine->registerApiClass(samplerObject);
    
    scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
    scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
    
	scriptEngine->registerApiClass(new cube::CubeApi());
}



void JavascriptMidiProcessor::addToFront(bool addToFront_) noexcept
{
	front = addToFront_;

#if USE_FRONTEND
	content->getUpdateDispatcher()->suspendUpdates(false);
#endif
}

void JavascriptMidiProcessor::runScriptCallbacks()
{
    if (currentEvent->isAllNotesOff())
    {
        // All notes off are controller message, so they should not be processed, or it can lead to loop.
        return;
    }
    
#if ENABLE_SCRIPTING_BREAKPOINTS
	breakpointWasHit(-1);
#endif

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.03);

	switch (currentEvent->getType())
	{
	case HiseEvent::Type::NoteOn:
	{
		synthObject->handleNoteCounter(*currentEvent, true);

		if (onNoteOnCallback->isSnippetEmpty()) return;

		scriptEngine->executeCallback(onNoteOn, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));

		break;
	}
	case HiseEvent::Type::NoteOff:
	{
		synthObject->handleNoteCounter(*currentEvent, false);

		if (onNoteOffCallback->isSnippetEmpty()) return;

		scriptEngine->executeCallback(onNoteOff, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));

		break;
	}
	case HiseEvent::Type::Controller:
	case HiseEvent::Type::PitchBend:
	case HiseEvent::Type::Aftertouch:
	case HiseEvent::Type::ProgramChange:
	{
		if (currentEvent->isControllerOfType(64))
		{
			synthObject->setSustainPedal(currentEvent->getControllerValue() > 64);
		}

		if (onControllerCallback->isSnippetEmpty()) return;

		
		

		Result r = Result::ok();
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
		break;
	}
	case HiseEvent::Type::TimerEvent:
	{
		if (!currentEvent->isIgnored() && currentEvent->getChannel() == getIndexInChain())
		{
			runTimerCallback(currentEvent->getTimeStamp());
			currentEvent->ignoreEvent(true); 
		}
		break;
	}
	case HiseEvent::Type::AllNotesOff:
	{
		synthObject->clearNoteCounter();
		break;
	}
        case HiseEvent::Type::Empty:
        case HiseEvent::Type::SongPosition:
        case HiseEvent::Type::MidiStart:
        case HiseEvent::Type::MidiStop:
        case HiseEvent::Type::VolumeFade:
        case HiseEvent::Type::PitchFade:
        case HiseEvent::Type::numTypes:
        break;
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

	scriptEngine->maximumExecutionTime = isDeferred() ? RelativeTime(0.5) : RelativeTime(0.002);

	if (lastResult.failed()) return;

	scriptEngine->executeCallback(onTimer, &lastResult);

	if (isDeferred())
	{
		sendSynchronousChangeMessage();
	}

	BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
}

void JavascriptMidiProcessor::deferCallbacks(bool addToFront_)
{
	deferred = addToFront_;
	if (deferred)
	{
		getOwnerSynth()->stopSynthTimer(getIndexInChain());
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


JavascriptPolyphonicEffect::JavascriptPolyphonicEffect(MainController *mc, const String &id, int numVoices):
	JavascriptProcessor(mc),
	ProcessorWithScriptingContent(mc),
	VoiceEffectProcessor(mc, id, numVoices),
	onInitCallback(new SnippetDocument("onInit")),
	onControlCallback(new SnippetDocument("onControl"))
{
	initContent();
	finaliseModChains();
	
	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");
}




JavascriptPolyphonicEffect::~JavascriptPolyphonicEffect()
{
	clearExternalWindows();
	cleanupEngine();

#if USE_BACKEND
	if (consoleEnabled)
	{
		getMainController()->setWatchedScriptProcessor(nullptr, nullptr);
	}
#endif
}

juce::Path JavascriptPolyphonicEffect::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

hise::ProcessorEditorBody * JavascriptPolyphonicEffect::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

hise::JavascriptProcessor::SnippetDocument * JavascriptPolyphonicEffect::getSnippet(int c)
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case Callback::onInit:			return onInitCallback;
	case Callback::onControl:		return onControlCallback;
	case Callback::numCallbacks:	return nullptr;
	default:
		break;
	}

	return nullptr;
}

const hise::JavascriptProcessor::SnippetDocument * JavascriptPolyphonicEffect::getSnippet(int c) const
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case Callback::onInit:			return onInitCallback;
	case Callback::onControl:		return onControlCallback;
	case Callback::numCallbacks:	return nullptr;
	default:
		break;
	}

	return nullptr;
}

void JavascriptPolyphonicEffect::registerApiClasses()
{
	engineObject = new ScriptingApi::Engine(this);

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptPolyphonicEffect::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}

void JavascriptPolyphonicEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate == -1.0)
		return;

	if (auto n = getActiveNetwork())
	{
		n->prepareToPlay(sampleRate, (double)samplesPerBlock);
	}
}

void JavascriptPolyphonicEffect::renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	if (auto n = getActiveNetwork())
	{
		float* channels[NUM_MAX_CHANNELS];

		int numChannels = b.getNumChannels();
		memcpy(channels, b.getArrayOfWritePointers(), sizeof(float*) * numChannels);

		for (int i = 0; i < numChannels; i++)
			channels[i] += startSample;

		scriptnode::ProcessData d(channels, numChannels, numSamples);

		scriptnode::DspNetwork::VoiceSetter vs(*n, voiceIndex);
		n->getRootNode()->process(d);
	}
}

void JavascriptPolyphonicEffect::startVoice(int voiceIndex, const HiseEvent& e)
{
	if (auto n = getActiveNetwork())
	{
		voiceNoteOns.insertWithoutSearch({ voiceIndex, e });
		HiseEvent c(e);

		scriptnode::DspNetwork::VoiceSetter vs(*n, voiceIndex);
		n->getRootNode()->reset();
		n->getRootNode()->handleHiseEvent(c);
	}
}

void JavascriptPolyphonicEffect::reset(int voiceIndex)
{
	for (int i = 0; i < voiceNoteOns.size(); i++)
	{
		if (voiceNoteOns[i].voiceIndex == voiceIndex)
		{
			voiceNoteOns.removeElement(i);
			return;
		}
	}
}

void JavascriptPolyphonicEffect::handleHiseEvent(const HiseEvent &m)
{
	if (m.isNoteOn())
		return;

	HiseEvent c(m);

	if (m.isNoteOff())
	{
		for (auto& av : voiceNoteOns)
		{
			if (av.noteOn.getEventId() == m.getEventId())
			{
				if (auto n = getActiveNetwork())
				{
					scriptnode::DspNetwork::VoiceSetter vs(*n, av.voiceIndex);
					n->getRootNode()->handleHiseEvent(c);
				}

				return;
			}
		}
	}

	if (auto n = getActiveNetwork())
		n->getRootNode()->handleHiseEvent(c);
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
	initContent();

	finaliseModChains();

	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("prepareToPlayOpen");
	editorStateIdentifiers.add("processBlockOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");

	getMatrix().setNumAllowedConnections(NUM_MAX_CHANNELS);

	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
	{
		buffers[i] = new VariantBuffer(0);
	}

	channels.ensureStorageAllocated(16);
	channelIndexes.ensureStorageAllocated(16);

	channelData = var(channels);

	connectionChanged();
}

JavascriptMasterEffect::~JavascriptMasterEffect()
{
	clearExternalWindows();
	cleanupEngine();

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

void JavascriptMasterEffect::connectionChanged()
{
	channels.clear();
	channelIndexes.clear();

	for (int i = 0; i < getMatrix().getNumSourceChannels(); i++)
	{
		if (getMatrix().getConnectionForSourceChannel(i) >= 0)
		{
			channels.add(buffers[channelIndexes.size()]);
			channelIndexes.add(i);
		}
	}

	auto numChannels = channels.size();

	for (auto n : networks)
	{
		n->setNumChannels(numChannels);
	}

	channelData = var(channels);
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
	//content = new ScriptingApi::Content(this);

	engineObject = new ScriptingApi::Engine(this);
	
	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));

}


void JavascriptMasterEffect::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}



void JavascriptMasterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	
	if (getActiveNetwork() != nullptr)
		getActiveNetwork()->prepareToPlay(sampleRate, samplesPerBlock);

	if (!prepareToPlayCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		scriptEngine->setCallbackParameter((int)Callback::prepareToPlay, 0, sampleRate);
		scriptEngine->setCallbackParameter((int)Callback::prepareToPlay, 1, samplesPerBlock);
		scriptEngine->executeCallback((int)Callback::prepareToPlay, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
}


void JavascriptMasterEffect::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (channelIndexes.size() == 2)
	{
		MasterEffectProcessor::renderWholeBuffer(buffer);

	}
	else
	{
		if (getActiveNetwork() != nullptr)
		{
			getActiveNetwork()->process(buffer, eventBuffer);
			return;
		}

		if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
		{
			const int numSamples = buffer.getNumSamples();

			jassert(channelIndexes.size() == channels.size());

			for (int i = 0; i < channelIndexes.size(); i++)
			{
				float* d = buffer.getWritePointer(channelIndexes[i], 0);

				CHECK_AND_LOG_BUFFER_DATA(this, DebugLogger::Location::ScriptFXRendering, d, true, numSamples);

				auto b = channels[i].getBuffer();

				if (b != nullptr)
					b->referToData(d, numSamples);
			}

			scriptEngine->setCallbackParameter((int)Callback::processBlock, 0, channelData);
			scriptEngine->executeCallback((int)Callback::processBlock, &lastResult);

			BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
		}
	}
}


void JavascriptMasterEffect::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	ignoreUnused(startSample);

	if (getActiveNetwork() != nullptr)
	{
		getActiveNetwork()->process(b, eventBuffer);
		return;
	}

	if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		jassert(startSample == 0);
		CHECK_AND_LOG_ASSERTION(this, DebugLogger::Location::ScriptFXRendering, startSample == 0, startSample);

		float *l = b.getWritePointer(0, 0);
		float *r = b.getWritePointer(1, 0);
        
		if (auto lb = channels[0].getBuffer())
			lb->referToData(l, numSamples);

		if (auto rb = channels[1].getBuffer())
			rb->referToData(r, numSamples);

		scriptEngine->setCallbackParameter((int)Callback::processBlock, 0, channelData);
		scriptEngine->executeCallback((int)Callback::processBlock, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
}

JavascriptVoiceStartModulator::JavascriptVoiceStartModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
JavascriptProcessor(mc),
ProcessorWithScriptingContent(mc),
VoiceStartModulator(mc, id, voiceAmount, m),
Modulation(m)
{
	initContent();

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
	clearExternalWindows();
	cleanupEngine();

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
		synthObject->handleNoteCounter(m, true);
	}
	else if (m.isNoteOff())
	{
		synthObject->handleNoteCounter(m, false);

		if (!onVoiceStopCallback->isSnippetEmpty())
		{
			scriptEngine->setCallbackParameter(onVoiceStop, 0, 0);
			scriptEngine->executeCallback(onVoiceStop, &lastResult);

			BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
		}
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
}

float JavascriptVoiceStartModulator::startVoice(int voiceIndex)
{
	if (!onVoiceStartCallback->isSnippetEmpty())
	{
		synthObject->setVoiceGainValue(voiceIndex, 1.0f);
		synthObject->setVoicePitchValue(voiceIndex, 1.0f);
		scriptEngine->setCallbackParameter(onVoiceStart, 0, voiceIndex);
		unsavedValue = (float)scriptEngine->executeCallback(onVoiceStart, &lastResult);
	}

	return VoiceStartModulator::startVoice(voiceIndex);
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
	initContent();

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
	clearExternalWindows();

	cleanupEngine();

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
	if (auto n = getActiveNetwork())
	{
		HiseEvent c(m);
		n->getRootNode()->handleHiseEvent(c);
	}

	currentMidiMessage->setHiseEvent(m);

	if (m.isNoteOn())
	{
		synthObject->handleNoteCounter(m, true);

		if (!onNoteOnCallback->isSnippetEmpty())
		{
			scriptEngine->executeCallback(onNoteOn, &lastResult);
		}

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
	else if (m.isNoteOff())
	{
		synthObject->handleNoteCounter(m, false);

		if (!onNoteOffCallback->isSnippetEmpty())
		{
			scriptEngine->executeCallback(onNoteOff, &lastResult);
		}

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
	else if (m.isController() && !onControllerCallback->isSnippetEmpty())
	{
		scriptEngine->executeCallback(onController, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}


}

void JavascriptTimeVariantModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);

	if (auto n = getActiveNetwork())
		n->prepareToPlay(getControlRate(), samplesPerBlock / HISE_EVENT_RASTER);

	if(internalBuffer.getNumChannels() > 0)
		buffer->referToData(internalBuffer.getWritePointer(0), samplesPerBlock);

	bufferVar = var(buffer);

	if (!prepareToPlayCallback->isSnippetEmpty())
	{
		scriptEngine->setCallbackParameter(Callback::prepare, 0, sampleRate);
		scriptEngine->setCallbackParameter(Callback::prepare, 1, samplesPerBlock);
		scriptEngine->executeCallback(Callback::prepare, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
}

void JavascriptTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
	if (auto n = getActiveNetwork())
	{
		auto ptr = internalBuffer.getWritePointer(0, startSample);
		FloatVectorOperations::clear(ptr, numSamples);

		scriptnode::ProcessData d(&ptr, 1, numSamples);

		ScopedLock sl(n->getConnectionLock());
		n->getRootNode()->process(d);
	}
	else if (!processBlockCallback->isSnippetEmpty() && lastResult.wasOk())
	{
		buffer->referToData(internalBuffer.getWritePointer(0, startSample), numSamples);

		scriptEngine->setCallbackParameter(Callback::processBlock, 0, bufferVar);
		scriptEngine->executeCallback(Callback::processBlock, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
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
	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::ModulatorApi(this));
	scriptEngine->registerApiClass(synthObject);

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}


void JavascriptTimeVariantModulator::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}



JavascriptEnvelopeModulator::JavascriptEnvelopeModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
ProcessorWithScriptingContent(mc),
JavascriptProcessor(mc),
EnvelopeModulator(mc, id, numVoices, m),
Modulation(m)
{
	initContent();

	onInitCallback = new SnippetDocument("onInit");
	onControlCallback = new SnippetDocument("onControl", "number value");

	for (int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onControlOpen");
	editorStateIdentifiers.add("externalPopupShown");
}



JavascriptEnvelopeModulator::~JavascriptEnvelopeModulator()
{
	cleanupEngine();
	clearExternalWindows();
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
		lastNoteOn = m;

	if (auto n = getActiveNetwork())
	{
		if (m.isNoteOff())
		{
			for (auto vd : voiceNoteOns)
			{
				if (vd.noteOn.getEventId() == m.getEventId())
				{
					HiseEvent c(m);
					scriptnode::DspNetwork::VoiceSetter vs(*n, vd.voiceIndex);
					n->getRootNode()->handleHiseEvent(c);
				}
			}
		}
	}
}

void JavascriptEnvelopeModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	if (auto n = getActiveNetwork())
	{
		n->setNumChannels(1);
		n->prepareToPlay(getControlRate(), samplesPerBlock / HISE_EVENT_RASTER);
	}
}

void JavascriptEnvelopeModulator::calculateBlock(int startSample, int numSamples)
{
	if (auto n = getActiveNetwork())
	{
		scriptnode::DspNetwork::VoiceSetter vs(*n, polyManager.getCurrentVoice());

		float* ptr = internalBuffer.getWritePointer(0, startSample);

		memset(ptr, 0, sizeof(float)*numSamples);

		scriptnode::ProcessData d(&ptr, 1, numSamples);

		d.shouldReset = false;

		ScopedLock sl(n->getConnectionLock());
		n->getRootNode()->process(d);

		if (d.shouldReset)
			reset(polyManager.getCurrentVoice());
			
	}

#if ENABLE_ALL_PEAK_METERS
	setOutputValue(internalBuffer.getSample(0, startSample));
#endif

}

float JavascriptEnvelopeModulator::startVoice(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);

	voiceNoteOns.insertWithoutSearch({ voiceIndex, lastNoteOn });

	state->uptime = 0.0f;
	state->isPlaying = true;
	state->isRingingOff = false;

	if (auto n = getActiveNetwork())
	{
		scriptnode::DspNetwork::VoiceSetter vs(*n, voiceIndex);
		n->getRootNode()->reset();
		HiseEvent c(lastNoteOn);
		n->getRootNode()->handleHiseEvent(c);
	}

    return 0.0f;
}

void JavascriptEnvelopeModulator::stopVoice(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);
	state->isRingingOff = true;
}

void JavascriptEnvelopeModulator::reset(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);

	state->uptime = 0.0f;
	state->isPlaying = false;
	state->isRingingOff = false;

	for (int i = 0; i < voiceNoteOns.size(); i++)
	{
		if (voiceNoteOns[i].voiceIndex == voiceIndex)
		{
			voiceNoteOns.removeElement(i--);
			break;
		}
	}
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
	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(currentMidiMessage);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::ModulatorApi(this));
	scriptEngine->registerApiClass(synthObject);

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptEnvelopeModulator::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
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

		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::startVoice, 0, getVoiceIndex());
		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::startVoice, 1, midiNoteNumber);
		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::startVoice, 2, velocity);

		voiceUptime = 0.0;
		uptimeDelta = (double)jms->scriptEngine->executeCallback((int)JavascriptModulatorSynth::Callback::startVoice, &jms->lastResult);

		BACKEND_ONLY(if (!jms->lastResult.wasOk()) debugError(jms, jms->lastResult.getErrorMessage()));
	}

	void calculateBlock(int startSample, int numSamples) override
	{
		const int startIndex = startSample;
		const int samplesToCopy = numSamples;

		const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();
		const float *modValues = getOwnerSynth()->getVoiceGainValues();

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

		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::renderVoice, 0, getVoiceIndex());
		jms->scriptEngine->setCallbackParameter((int)JavascriptModulatorSynth::Callback::renderVoice, 1, var(channels));

		voiceUptime += (double)jms->scriptEngine->executeCallback((int)JavascriptModulatorSynth::Callback::renderVoice, &jms->lastResult);

		BACKEND_ONLY(if (!jms->lastResult.wasOk()) debugError(jms, jms->lastResult.getErrorMessage()));

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
onControlCallback(new SnippetDocument("onControl", "number value")),
scriptChain1Buffer(1, 0),
scriptChain2Buffer(1, 0)
{
	initContent();

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

	addSound(new Sound());

	for (int i = 0; i < numVoices; i++)
	{
		addVoice(new Voice(this));
	}
}

JavascriptModulatorSynth::~JavascriptModulatorSynth()
{
	clearExternalWindows();

	cleanupEngine();

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
		ProcessorHelpers::increaseBufferIfNeeded(scriptChain1Buffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(scriptChain2Buffer, samplesPerBlock);

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

void JavascriptModulatorSynth::preStartVoice(int voiceIndex, const HiseEvent& e)
{
	ModulatorSynth::preStartVoice(voiceIndex, e);

	scriptChain1->startVoice(voiceIndex);
	scriptChain2->startVoice(voiceIndex);
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

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}



void JavascriptModulatorSynth::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
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

JavascriptSynthesiser::JavascriptSynthesiser(MainController *mc, const String &id, int numVoices):
	JavascriptProcessor(mc),
	ProcessorWithScriptingContent(mc),
	ModulatorSynth(mc, id, numVoices)
{
	initContent();

	onInitCallback = new SnippetDocument("onInit");
	onControlCallback = new SnippetDocument("onControl", "number value");

	editorStateIdentifiers.add("contentShown");
	editorStateIdentifiers.add("onInitOpen");
	editorStateIdentifiers.add("onControlOpen");

	modChains += { this, "Extra1" };
	modChains += { this, "Extra2" };

	finaliseModChains();

	modChains[Extra1].setIncludeMonophonicValuesInVoiceRendering(true);
	modChains[Extra1].setExpandToAudioRate(false);

	modChains[Extra2].setIncludeMonophonicValuesInVoiceRendering(true);
	modChains[Extra2].setExpandToAudioRate(false);

	modChains[Extra1].getChain()->setColour(Colour(0xFF888888));
	modChains[Extra2].getChain()->setColour(Colour(0xFF888888));

	for (int i = 0; i < numVoices; i++)
	{
		addVoice(new Voice(this));
	}

	addSound(new Sound());
}

JavascriptSynthesiser::~JavascriptSynthesiser()
{

}

juce::Path JavascriptSynthesiser::getSpecialSymbol() const
{
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

hise::ProcessorEditorBody * JavascriptSynthesiser::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ScriptingEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

hise::JavascriptProcessor::SnippetDocument * JavascriptSynthesiser::getSnippet(int c)
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case Callback::onInit:			return onInitCallback;
	case Callback::onControl:		return onControlCallback;
	case Callback::numCallbacks:	return nullptr;
	default:
		break;
	}

	return nullptr;
}


const hise::JavascriptProcessor::SnippetDocument * JavascriptSynthesiser::getSnippet(int c) const
{
	Callback ca = (Callback)c;

	switch (ca)
	{
	case Callback::onInit:			return onInitCallback;
	case Callback::onControl:		return onControlCallback;
	case Callback::numCallbacks:	return nullptr;
	default:
		break;
	}

	return nullptr;
}

void JavascriptSynthesiser::registerApiClasses()
{
	engineObject = new ScriptingApi::Engine(this);

	scriptEngine->registerNativeObject("Content", content);
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptSynthesiser::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}

void JavascriptSynthesiser::preHiseEventCallback(const HiseEvent &e)
{
	ModulatorSynth::preHiseEventCallback(e);

	if (e.isNoteOn())
		return; // will be handled by preStartVoice

	if (auto n = getActiveNetwork())
	{
		for (auto av : activeVoices)
		{
			if (e.isNoteOff() && av->getCurrentHiseEvent().getEventId() != e.getEventId())
				continue;

			scriptnode::DspNetwork::VoiceSetter vs(*n, av->getVoiceIndex());

			HiseEvent copy(e);
			n->getRootNode()->handleHiseEvent(copy);
		}
	}
}

void JavascriptSynthesiser::preStartVoice(int voiceIndex, const HiseEvent& e)
{
	ModulatorSynth::preStartVoice(voiceIndex, e);

	if (auto n = getActiveNetwork())
	{
		static_cast<Voice*>(getVoice(voiceIndex))->setVoiceStartDataForNextRenderCallback();

		currentVoiceStartSample = jlimit(0, getLargestBlockSize(), e.getTimeStamp());


	}
}

void JavascriptSynthesiser::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	if (newSampleRate == -1.0)
		return;

	if (auto n = getActiveNetwork())
	{
		n->prepareToPlay(newSampleRate, (double)samplesPerBlock);
	}
}


float JavascriptSynthesiser::getModValueAtVoiceStart(int modIndex) const
{
	return getModValueForNode(modIndex, currentVoiceStartSample);
}

void JavascriptSynthesiser::Voice::calculateBlock(int startSample, int numSamples)
{
	if (auto n = synth->getActiveNetwork())
	{
		auto rootNode = n->getRootNode();

		if (isVoiceStart)
		{
			scriptnode::DspNetwork::VoiceSetter vs(*n, getVoiceIndex());
			// There are some nodes which need to setup some properties before
			// calling reset based on the incoming event
			auto event = getCurrentHiseEvent();
			rootNode->handleHiseEvent(event);
			rootNode->reset();

			isVoiceStart = false;
		}

		float* channels[NUM_MAX_CHANNELS];

		voiceBuffer.clear();

		int numChannels = voiceBuffer.getNumChannels();
		memcpy(channels, voiceBuffer.getArrayOfWritePointers(), sizeof(float*) * numChannels);

		for (int i = 0; i < numChannels; i++)
			channels[i] += startSample;

		scriptnode::ProcessData d(channels, numChannels, numSamples);

		{
			scriptnode::DspNetwork::VoiceSetter vs(*n, getVoiceIndex());
			rootNode->process(d);
		}
		
		if (auto modValues = getOwnerSynth()->getVoiceGainValues())
		{
			for(int i = 0; i < voiceBuffer.getNumChannels(); i++)
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(i, startSample), modValues + startSample, numSamples);
		}
		else
		{
			const float gainValue = getOwnerSynth()->getConstantGainModValue();

			for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(i, startSample), gainValue, numSamples);
		}

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startSample, numSamples);
	}
}

} // namespace hise
