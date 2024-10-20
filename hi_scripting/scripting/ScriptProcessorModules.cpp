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

	serverObject = nullptr;

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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

void JavascriptMidiProcessor::suspendStateChanged(bool shouldBeSuspended)
{
	ScriptBaseMidiProcessor::suspendStateChanged(shouldBeSuspended);

	deferredExecutioner.suspend(shouldBeSuspended);
}

int JavascriptMidiProcessor::getNumSnippets() const
{ return numCallbacks; }

bool JavascriptMidiProcessor::isFront() const
{ return front; }

bool JavascriptMidiProcessor::isDeferred() const
{ return deferred; }

void JavascriptMidiProcessor::timerCallback()
{
	jassert(isDeferred());
	runTimerCallback();
}

ScriptingApi::Server::WeakPtr JavascriptMidiProcessor::getServerObject()
{ return serverObject; }

JavascriptMidiProcessor::DeferredExecutioner::DeferredExecutioner(JavascriptMidiProcessor* jp):
	parent(*jp),
	pendingEvents(512)
{}

void JavascriptMidiProcessor::DeferredExecutioner::addPendingEvent(const HiseEvent& e)
{
	pendingEvents.push(e);
	triggerAsyncUpdate();
}

void JavascriptMidiProcessor::DeferredExecutioner::handleAsyncUpdate()
{
	jassert(parent.isDeferred());
			
	HiseEvent m;

	while (pendingEvents.pop(m))
	{
		if (m.isIgnored() || m.isArtificial())
			continue;

		auto f = [m](JavascriptProcessor* p)
		{
			auto jmp = dynamic_cast<JavascriptMidiProcessor*>(p);

			HiseEvent copy(m);

			ScopedValueSetter<HiseEvent*> svs(jmp->currentEvent, &copy);
			jmp->currentMidiMessage->setHiseEvent(m);
			jmp->runScriptCallbacks();

			return jmp->lastResult;
		};

		parent.getMainController()->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution, &parent, f);
	}
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

JavascriptMidiProcessor* JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(MainController* mc)
{
	Processor::Iterator<JavascriptMidiProcessor> iter(mc->getMainSynthChain());

	while (auto jsp = iter.getNextProcessor())
	{
		if (jsp->isFront())
		{
			return jsp;
		}
	}

	return nullptr;
}

void JavascriptMidiProcessor::registerApiClasses()
{
	

	//content = new ScriptingApi::Content(this);
    front = false;

	currentMidiMessage = new ScriptingApi::Message(this);
	engineObject = new ScriptingApi::Engine(this);
	synthObject = new ScriptingApi::Synth(this, currentMidiMessage.get(), getOwnerSynth());

	scriptEngine->registerApiClass(new ScriptingApi::ModuleIds(getOwnerSynth()));

	samplerObject = new ScriptingApi::Sampler(this, dynamic_cast<ModulatorSampler*>(getOwnerSynth()));

	scriptEngine->registerNativeObject("Content", getScriptingContent());
	scriptEngine->registerApiClass(currentMidiMessage.get());
	scriptEngine->registerApiClass(engineObject.get());
	scriptEngine->registerApiClass(new ScriptingApi::Settings(this));
	scriptEngine->registerApiClass(new ScriptingApi::FileSystem(this));
	scriptEngine->registerApiClass(new ScriptingApi::Threads(this));
	scriptEngine->registerApiClass(new ScriptingApi::Date(this));

	scriptEngine->registerApiClass(serverObject = new ScriptingApi::Server(this));
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::Colours());
	scriptEngine->registerApiClass(synthObject);
	scriptEngine->registerApiClass(samplerObject);
    
    scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
    scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
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
		synthObject->handleNoteCounter(*currentEvent);
		// All notes off are controller message, so they should not be processed, or it can lead to loop.
		currentMidiMessage->onAllNotesOff();
        return;
    }
    
#if ENABLE_SCRIPTING_BREAKPOINTS
	breakpointWasHit(-1);
#endif

	scriptEngine->maximumExecutionTime = HiseJavascriptEngine::getDefaultTimeOut();

	synthObject->handleNoteCounter(*currentEvent);

	switch (currentEvent->getType())
	{
	case HiseEvent::Type::NoteOn:
	{
		if (onNoteOnCallback->isSnippetEmpty()) return;

		scriptEngine->executeCallback(onNoteOn, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));

		break;
	}
	case HiseEvent::Type::NoteOff:
	{
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

	scriptEngine->maximumExecutionTime = HiseJavascriptEngine::getDefaultTimeOut();

	if (lastResult.failed()) return;

	scriptEngine->executeCallback(onTimer, &lastResult);

	if (isDeferred())
	{
		sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::OtherUnused, dispatch::sendNotificationSync);
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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
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

	scriptEngine->registerNativeObject("Content", content.get());
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerApiClass(new ScriptingApi::Settings(this));
	scriptEngine->registerApiClass(new ScriptingApi::FileSystem(this));
	scriptEngine->registerApiClass(new ScriptingApi::Threads(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptPolyphonicEffect::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}

bool JavascriptPolyphonicEffect::hasTail() const
{
	if (auto n = getActiveNetwork())
	{
		return n->hasTail();
	}

	return false;
}

bool JavascriptPolyphonicEffect::isSuspendedOnSilence() const
{
	if (auto n = getActiveNetwork())
	{
		return n->isSuspendedOnSilence();
	}

	return true;
}

void JavascriptPolyphonicEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate == -1.0)
		return;

	if (auto n = getActiveNetwork())
	{
		auto numChannels = dynamic_cast<RoutableProcessor*>(getParentProcessor(true))->getMatrix().getNumSourceChannels();

        setVoiceKillerToUse(this);
        
		n->setNumChannels(numChannels);
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

		scriptnode::ProcessDataDyn d(channels, numSamples, numChannels);

		if (checkPreSuspension(voiceIndex, d))
			return;

		scriptnode::DspNetwork::VoiceSetter vs(*n, voiceIndex);
		n->getRootNode()->process(d);
        
		checkPostSuspension(voiceIndex, d);

		// overwrite the tailing with the voice index to cater in
		// voice resetting calls...
		isTailing = voiceData.containsVoiceIndex(voiceIndex);
	}
}

void JavascriptPolyphonicEffect::startVoice(int voiceIndex, const HiseEvent& e)
{
	VoiceEffectProcessor::startVoice(voiceIndex, e);

	if (auto n = getActiveNetwork())
	{
		voiceData.startVoice(*n, *n->getPolyHandler(), voiceIndex, e);
		
	}
}

void JavascriptPolyphonicEffect::reset(int voiceIndex)
{
	VoiceEffectProcessor::reset(voiceIndex);

	voiceData.reset(voiceIndex);
}

void JavascriptPolyphonicEffect::handleHiseEvent(const HiseEvent &m)
{
	if (m.isNoteOn())
		return;

	HiseEvent c(m);

	if (auto n = getActiveNetwork())
	{
		voiceData.handleHiseEvent(*n, *n->getPolyHandler(), m);
	}
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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
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

int JavascriptMasterEffect::getNumSnippets() const
{ return (int)Callback::numCallbacks; }

Processor* JavascriptMasterEffect::getChildProcessor(int)
{ return nullptr; }

const Processor* JavascriptMasterEffect::getChildProcessor(int) const
{ return nullptr; }

int JavascriptMasterEffect::getNumInternalChains() const
{ return 0; }

int JavascriptMasterEffect::getNumChildProcessors() const
{ return 0; }

float JavascriptMasterEffect::getAttribute(int index) const
{ 
	return getCurrentNetworkParameterHandler(&contentParameterHandler)->getParameter(index);
}

void JavascriptMasterEffect::setInternalAttribute(int index, float newValue)
{ 
	getCurrentNetworkParameterHandler(&contentParameterHandler)->setParameter(index, newValue);
}

Identifier JavascriptMasterEffect::getIdentifierForParameterIndex(int parameterIndex) const
{
	return getCurrentNetworkParameterHandler(&contentParameterHandler)->getParameterId(parameterIndex);
}

ValueTree JavascriptMasterEffect::exportAsValueTree() const
{ ValueTree v = MasterEffectProcessor::exportAsValueTree(); saveContent(v); saveScript(v); return v; }

void JavascriptMasterEffect::restoreFromValueTree(const ValueTree& v)
{ MasterEffectProcessor::restoreFromValueTree(v); restoreScript(v); restoreContent(v); }

int JavascriptMasterEffect::getControlCallbackIndex() const
{ return (int)Callback::onControl; }

void JavascriptMasterEffect::registerApiClasses()
{
	//content = new ScriptingApi::Content(this);

	engineObject = new ScriptingApi::Engine(this);
	
	scriptEngine->registerNativeObject("Content", content.get());
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerApiClass(new ScriptingApi::Settings(this));
	scriptEngine->registerApiClass(new ScriptingApi::FileSystem(this));
	scriptEngine->registerApiClass(new ScriptingApi::Threads(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));

}


void JavascriptMasterEffect::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}



void JavascriptMasterEffect::voicesKilled()
{
	if (auto n = getActiveNetwork())
	{

		n->reset();
	}
}

bool JavascriptMasterEffect::hasTail() const
{
	if (auto n = getActiveNetwork())
	{
		return n->hasTail();
	}

	return false;
}

bool JavascriptMasterEffect::isSuspendedOnSilence() const
{
	if (auto n = getActiveNetwork())
		return n->isSuspendedOnSilence();

	return false;
}

void JavascriptMasterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	
    connectionChanged();
    
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
			auto numChannels = channelIndexes.size();
			auto numSamples = buffer.getNumSamples();

			auto data = (float**)alloca(numChannels * sizeof(float*));

			for(int i = 0; i < numChannels; i++)
				data[i] = buffer.getWritePointer(channelIndexes[i]);

			ProcessDataDyn pd(data, numSamples, numChannels);
			pd.setEventBuffer(*eventBuffer);
			getActiveNetwork()->process(pd);
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

void JavascriptMasterEffect::setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler) noexcept
{
	MasterEffectProcessor::setBypassed(shouldBeBypassed, notifyChangeHandler);

	if (!shouldBeBypassed)
	{
		if (auto n = getActiveNetwork())
		{
			n->reset();
		}
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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
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

	synthObject->handleNoteCounter(m);

	if (m.isNoteOff())
	{
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
	synthObject = new ScriptingApi::Synth(this, currentMidiMessage.get(), dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content.get());
	scriptEngine->registerApiClass(currentMidiMessage.get());
	scriptEngine->registerApiClass(engineObject.get());
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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

float JavascriptTimeVariantModulator::getAttribute(int index) const
{
	if (auto n = getActiveOrDebuggedNetwork())
		return n->networkParameterHandler.getParameter(index);
	else
		return contentParameterHandler.getParameter(index);
}

void JavascriptTimeVariantModulator::setInternalAttribute(int index, float newValue)
{
	if (auto n = getActiveOrDebuggedNetwork())
		n->networkParameterHandler.setParameter(index, newValue);
	else
		contentParameterHandler.setParameter(index, newValue);
}

Identifier JavascriptTimeVariantModulator::getIdentifierForParameterIndex(int parameterIndex) const
{
	if (auto n = getActiveOrDebuggedNetwork())
		return n->networkParameterHandler.getParameterId(parameterIndex);
	else
		return contentParameterHandler.getParameterId(parameterIndex);
}

ValueTree JavascriptTimeVariantModulator::exportAsValueTree() const
{ ValueTree v = TimeVariantModulator::exportAsValueTree(); saveContent(v); saveScript(v); return v; }

void JavascriptTimeVariantModulator::restoreFromValueTree(const ValueTree& v)
{ TimeVariantModulator::restoreFromValueTree(v); restoreScript(v); restoreContent(v); }

Processor* JavascriptTimeVariantModulator::getChildProcessor(int)
{ return nullptr; }

const Processor* JavascriptTimeVariantModulator::getChildProcessor(int) const
{ return nullptr; }

int JavascriptTimeVariantModulator::getNumChildProcessors() const
{ return 0; }

int JavascriptTimeVariantModulator::getNumSnippets() const
{ return Callback::numCallbacks; }

int JavascriptTimeVariantModulator::getControlCallbackIndex() const
{ return (int)Callback::onControl; }

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

	synthObject->handleNoteCounter(m);

	if (m.isNoteOn())
	{
		if (!onNoteOnCallback->isSnippetEmpty())
			scriptEngine->executeCallback(onNoteOn, &lastResult);

		BACKEND_ONLY(if (!lastResult.wasOk()) debugError(this, lastResult.getErrorMessage()));
	}
	else if (m.isNoteOff())
	{
		if (!onNoteOffCallback->isSnippetEmpty())
			scriptEngine->executeCallback(onNoteOff, &lastResult);

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
    {
		n->prepareToPlay(getControlRate(), samplesPerBlock / HISE_EVENT_RASTER);
        n->setNumChannels(1);
    }

	if(internalBuffer.getNumChannels() > 0)
		buffer->referToData(internalBuffer.getWritePointer(0), samplesPerBlock);

	bufferVar = var(buffer.get());

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

		snex::Types::ProcessDataDyn d(&ptr, numSamples, 1);

		if (auto s = SimpleReadWriteLock::ScopedTryReadLock(n->getConnectionLock()))
		{
			if(n->getExceptionHandler().isOk())
				n->getRootNode()->process(d);
		}

		FloatVectorOperations::clip(ptr, ptr, 0.0f, 1.0f, numSamples);
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
	synthObject = new ScriptingApi::Synth(this, currentMidiMessage.get(), dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content.get());
	scriptEngine->registerApiClass(currentMidiMessage.get());
	scriptEngine->registerApiClass(engineObject.get());
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
	setVoiceKillerToUse(this);

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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
}

bool JavascriptEnvelopeModulator::isPolyphonic() const
{ return true; }

ValueTree JavascriptEnvelopeModulator::exportAsValueTree() const
{ ValueTree v = EnvelopeModulator::exportAsValueTree(); saveContent(v); saveScript(v); return v; }

void JavascriptEnvelopeModulator::restoreFromValueTree(const ValueTree& v)
{ EnvelopeModulator::restoreFromValueTree(v); restoreScript(v); restoreContent(v); }

void JavascriptEnvelopeModulator::onVoiceReset(bool allVoices, int voiceIndex)
{
	if (allVoices)
	{
		for (int i = 0; i < polyManager.getVoiceAmount(); i++)
			reset(i);
	}
	else
		reset(voiceIndex);
}

int JavascriptEnvelopeModulator::getNumParameters() const
{
	return getCurrentNetworkParameterHandler(&contentParameterHandler)->getNumParameters() + (int)hise::EnvelopeModulator::Parameters::numParameters;
}

void JavascriptEnvelopeModulator::setInternalAttribute(int index, float newValue)
{
	if (index < hise::EnvelopeModulator::Parameters::numParameters)
		EnvelopeModulator::setInternalAttribute(index, newValue);
	else
	{
		index -= (int)hise::EnvelopeModulator::Parameters::numParameters;

		if (auto n = getActiveOrDebuggedNetwork())
			n->networkParameterHandler.setParameter(index, newValue);
		else
			contentParameterHandler.setParameter(index, newValue);
	}
}

float JavascriptEnvelopeModulator::getAttribute(int index) const
{
	if (index < hise::EnvelopeModulator::Parameters::numParameters)
		return EnvelopeModulator::getAttribute(index);
	else
	{
		index -= (int)hise::EnvelopeModulator::Parameters::numParameters;

		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameter(index);
		else
			return contentParameterHandler.getParameter(index);
	}
}

Identifier JavascriptEnvelopeModulator::getIdentifierForParameterIndex(int index) const
{
	if (index < hise::EnvelopeModulator::Parameters::numParameters)
		return parameterNames[index];
	else
	{
		index -= (int)hise::EnvelopeModulator::Parameters::numParameters;

		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameterId(index);
		else
			return contentParameterHandler.getParameterId(index);
	}
		
}

Processor* JavascriptEnvelopeModulator::getChildProcessor(int)
{ return nullptr; }

const Processor* JavascriptEnvelopeModulator::getChildProcessor(int) const
{ return nullptr; }

int JavascriptEnvelopeModulator::getNumChildProcessors() const
{ return 0; }

int JavascriptEnvelopeModulator::getNumSnippets() const
{ return Callback::numCallbacks; }

int JavascriptEnvelopeModulator::getControlCallbackIndex() const
{ return (int)Callback::onControl; }

JavascriptEnvelopeModulator::ScriptEnvelopeState::ScriptEnvelopeState(int voiceIndex_):
	EnvelopeModulator::ModulatorState(voiceIndex_)
{}

EnvelopeModulator::ModulatorState* JavascriptEnvelopeModulator::createSubclassedState(int voiceIndex) const
{ return new ScriptEnvelopeState(voiceIndex); }

int JavascriptEnvelopeModulator::getNumActiveVoices() const
{
	int counter = 0;

	for (int i = 0; i < polyManager.getVoiceAmount(); i++)
	{
		if (auto ses = static_cast<ScriptEnvelopeState*>(states[i]))
		{
			if (ses->isPlaying)
				counter++;
		}
	}

	return counter;
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
		voiceData.handleHiseEvent(*n, *n->getPolyHandler(), m);
	}
}

void JavascriptEnvelopeModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	if (auto n = getActiveNetwork())
	{
		n->prepareToPlay(getControlRate(), samplesPerBlock / HISE_EVENT_RASTER);
        n->setNumChannels(1);
	}
}

void JavascriptEnvelopeModulator::calculateBlock(int startSample, int numSamples)
{
	if (auto n = getActiveNetwork())
	{
		scriptnode::DspNetwork::VoiceSetter vs(*n, polyManager.getCurrentVoice());

		float* ptr = internalBuffer.getWritePointer(0, startSample);

		memset(ptr, 0, sizeof(float)*numSamples);

		scriptnode::ProcessDataDyn d(&ptr, numSamples, 1);

		if (auto s = SimpleReadWriteLock::ScopedTryReadLock(n->getConnectionLock()))
		{
			if(n->getExceptionHandler().isOk())
				n->getRootNode()->process(d);
		}
	}

#if ENABLE_ALL_PEAK_METERS
	setOutputValue(internalBuffer.getSample(0, startSample));
#endif

}

float JavascriptEnvelopeModulator::startVoice(int voiceIndex)
{
	ScriptEnvelopeState* state = static_cast<ScriptEnvelopeState*>(states[voiceIndex]);

	state->uptime = 0.0f;
	state->isPlaying = true;
	state->isRingingOff = false;

	if (auto n = getActiveNetwork())
	{
		voiceData.startVoice(*n, *n->getPolyHandler(), voiceIndex, lastNoteOn);
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

	voiceData.reset(voiceIndex);
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
	synthObject = new ScriptingApi::Synth(this, currentMidiMessage.get(), dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(this, true)));

	scriptEngine->registerNativeObject("Content", content.get());
	scriptEngine->registerApiClass(currentMidiMessage.get());
	scriptEngine->registerApiClass(engineObject.get());
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));
	scriptEngine->registerApiClass(new ScriptingApi::ModulatorApi(this));
	scriptEngine->registerApiClass(new ScriptingApi::Settings(this));
	scriptEngine->registerApiClass(new ScriptingApi::FileSystem(this));
	scriptEngine->registerApiClass(new ScriptingApi::Threads(this));

	scriptEngine->registerApiClass(synthObject);

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptEnvelopeModulator::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
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
	Path path; path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor)); return path;
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

	scriptEngine->registerNativeObject("Content", content.get());
	scriptEngine->registerApiClass(engineObject);
	scriptEngine->registerApiClass(new ScriptingApi::Console(this));

	scriptEngine->registerApiClass(new ScriptingApi::Settings(this));
	scriptEngine->registerApiClass(new ScriptingApi::FileSystem(this));
	scriptEngine->registerApiClass(new ScriptingApi::Threads(this));

	scriptEngine->registerNativeObject("Libraries", new DspFactory::LibraryLoader(this));
	scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));
}

void JavascriptSynthesiser::postCompileCallback()
{
	prepareToPlay(getSampleRate(), getLargestBlockSize());
}

void JavascriptSynthesiser::preHiseEventCallback(HiseEvent &e)
{
	ModulatorSynth::preHiseEventCallback(e);

	if (e.isNoteOn())
		return; // will be handled by preStartVoice

	if (auto n = getActiveNetwork())
	{
		voiceData.handleHiseEvent(*n, *n->getPolyHandler(), e);
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
		if (auto vk = ProcessorHelpers::getFirstProcessorWithType<ScriptnodeVoiceKiller>(gainChain))
			setVoiceKillerToUse(vk);

        n->prepareToPlay(newSampleRate, (double)samplesPerBlock);
        n->setNumChannels(getMatrix().getNumSourceChannels());
		
		
	}
}




void JavascriptSynthesiser::restoreFromValueTree(const ValueTree &v)
{
	ModulatorSynth::restoreFromValueTree(v); 

	if (auto vk = ProcessorHelpers::getFirstProcessorWithType<ScriptnodeVoiceKiller>(gainChain))
		setVoiceKillerToUse(vk);

	restoreScript(v); 
	restoreContent(v);
}

bool JavascriptSynthesiser::Sound::appliesToNote(int)
{ return true; }

bool JavascriptSynthesiser::Sound::appliesToChannel(int)
{ return true; }

bool JavascriptSynthesiser::Sound::appliesToVelocity(int)
{ return true; }

JavascriptSynthesiser::Voice::Voice(JavascriptSynthesiser* p):
	ModulatorSynthVoice(p),
	synth(p)
{}

void JavascriptSynthesiser::Voice::setVoiceStartDataForNextRenderCallback()
{
	isVoiceStart = true;
}

void JavascriptSynthesiser::Voice::resetVoice()
{
	ModulatorSynthVoice::resetVoice();
	synth->voiceData.reset(getVoiceIndex());
}

int JavascriptSynthesiser::getNumSnippets() const
{ return (int)Callback::numCallbacks; }

bool JavascriptSynthesiser::isPolyphonic() const
{ return true; }

float JavascriptSynthesiser::getModValueForNode(int modIndex, int startSample) const
{
	if (startSample == -1)
		startSample = currentVoiceStartSample;

	if (modIndex == BasicChains::PitchChain)
	{
		auto& pc = modChains[BasicChains::PitchChain];
		if (auto pValues = pc.getReadPointerForVoiceValues(0))
			return pValues[startSample];
		else
			return pc.getConstantModulationValue();
	}
	else
	{
		return modChains[modIndex].getOneModulationValue(startSample);
	}
		
}

Processor* JavascriptSynthesiser::getChildProcessor(int processorIndex)
{
	if (processorIndex < ModulatorSynth::numInternalChains)
		return ModulatorSynth::getChildProcessor(processorIndex);
	if (processorIndex == ModulatorSynth::numInternalChains)
		return modChains[Extra1].getChain();
	if (processorIndex == ModulatorSynth::numInternalChains + 1)
		return modChains[Extra2].getChain();

	return nullptr;
}

const Processor* JavascriptSynthesiser::getChildProcessor(int processorIndex) const
{
	return const_cast<JavascriptSynthesiser*>(this)->getChildProcessor(processorIndex);
}

int JavascriptSynthesiser::getNumInternalChains() const
{ return ModulatorSynth::numInternalChains + 2; }

int JavascriptSynthesiser::getNumChildProcessors() const
{ return getNumInternalChains(); }

ValueTree JavascriptSynthesiser::exportAsValueTree() const
{ ValueTree v = ModulatorSynth::exportAsValueTree(); saveContent(v); saveScript(v); return v; }

int JavascriptSynthesiser::getNumParameters() const
{
	return getCurrentNetworkParameterHandler(&contentParameterHandler)->getNumParameters() + (int)ModulatorSynth::Parameters::numModulatorSynthParameters;
}

float JavascriptSynthesiser::getAttribute(int index) const
{
	if (index < ModulatorSynth::Parameters::numModulatorSynthParameters)
	{
		return ModulatorSynth::getAttribute(index);
	}

	index -= ModulatorSynth::Parameters::numModulatorSynthParameters;

	return getCurrentNetworkParameterHandler(&contentParameterHandler)->getParameter(index);
}

void JavascriptSynthesiser::setInternalAttribute(int index, float newValue)
{
	if (index < ModulatorSynth::Parameters::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(index, newValue);
		return;
	}

	index -= ModulatorSynth::Parameters::numModulatorSynthParameters;

	getCurrentNetworkParameterHandler(&contentParameterHandler)->setParameter(index, newValue);
}

Identifier JavascriptSynthesiser::getIdentifierForParameterIndex(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::Parameters::numModulatorSynthParameters)
	{
		return ModulatorSynth::getIdentifierForParameterIndex(parameterIndex);
	}

	parameterIndex -= ModulatorSynth::Parameters::numModulatorSynthParameters;

	return getCurrentNetworkParameterHandler(&contentParameterHandler)->getParameterId(parameterIndex);
}

int JavascriptSynthesiser::getControlCallbackIndex() const
{ return (int)Callback::onControl; }

void JavascriptSynthesiser::Voice::calculateBlock(int startSample, int numSamples)
{
	
	if (auto n = synth->getActiveNetwork())
	{
		if (isVoiceStart)
		{
			n->setVoiceKiller(synth->vk);
			synth->voiceData.startVoice(*n, *n->getPolyHandler(), getVoiceIndex(), getCurrentHiseEvent());
			isVoiceStart = false;
		}

		float* channels[NUM_MAX_CHANNELS];

		voiceBuffer.clear();

		int numChannels = voiceBuffer.getNumChannels();
		memcpy(channels, voiceBuffer.getArrayOfWritePointers(), sizeof(float*) * numChannels);

		for (int i = 0; i < numChannels; i++)
			channels[i] += startSample;

		scriptnode::ProcessDataDyn d(channels, numSamples, numChannels);

		{
			scriptnode::DspNetwork::VoiceSetter vs(*n, getVoiceIndex());
            n->process(d);
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

ScriptnodeVoiceKiller::ScriptnodeVoiceKiller(MainController* mc, const String& id, int numVoices):
	EnvelopeModulator(mc, id, numVoices, Modulation::GainMode),
	Modulation(Modulation::GainMode)
{
	for (int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	SafeAsyncCall::callWithDelay<ScriptnodeVoiceKiller>(*this, initialiseNetworks, 300);
}

void ScriptnodeVoiceKiller::setInternalAttribute(int parameter_index, float newValue)
{}

float ScriptnodeVoiceKiller::getDefaultValue(int parameterIndex) const
{ return 0.0f; }

float ScriptnodeVoiceKiller::getAttribute(int parameter_index) const
{ return 0.0f; }

int ScriptnodeVoiceKiller::getNumInternalChains() const
{ return 0; }

int ScriptnodeVoiceKiller::getNumChildProcessors() const
{ return 0; }

Processor* ScriptnodeVoiceKiller::getChildProcessor(int)
{ return nullptr; }

const Processor* ScriptnodeVoiceKiller::getChildProcessor(int) const
{ return nullptr; }

void ScriptnodeVoiceKiller::stopVoice(int voiceIndex)
{}

void ScriptnodeVoiceKiller::reset(int voiceIndex)
{ getState(voiceIndex)->active = false; }

bool ScriptnodeVoiceKiller::isPlaying(int voiceIndex) const
{ return getState(voiceIndex)->active; }

void ScriptnodeVoiceKiller::calculateBlock(int startSample, int numSamples)
{ FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 1.0f, numSamples); }

void ScriptnodeVoiceKiller::handleHiseEvent(const HiseEvent& m)
{}

ScriptnodeVoiceKiller::State::State(int v): ModulatorState(v)
{}

int ScriptnodeVoiceKiller::getNumActiveVoices() const
{
	int counter = 0;

	for (int i = 0; i < polyManager.getVoiceAmount(); i++)
	{
		if (getState(i)->active)
			counter++;
	}

	return counter;
}

void ScriptnodeVoiceKiller::onVoiceReset(bool allVoices, int voiceIndex)
{
	if (allVoices)
	{
		for (int i = 0; i < polyManager.getVoiceAmount(); i++)
			getState(i)->active.store(false);
	}
	else
		reset(voiceIndex);
}

ScriptnodeVoiceKiller::State* ScriptnodeVoiceKiller::getState(int i)
{ return  static_cast<State*>(states[i]); }

const ScriptnodeVoiceKiller::State* ScriptnodeVoiceKiller::getState(int i) const
{ return  static_cast<const State*>(states[i]); }

EnvelopeModulator::ModulatorState* ScriptnodeVoiceKiller::createSubclassedState(int voiceIndex) const
{ return new State(voiceIndex); }

void ScriptnodeVoiceKiller::initialiseNetworks(ScriptnodeVoiceKiller& v)
{
	if (!v.initialised)
	{
		auto parentSynth = v.getParentProcessor(true, false);
		auto modParent = v.getParentProcessor(false, false);

		if (parentSynth == nullptr)
			return;

		auto modParentIsGain = parentSynth->getChildProcessor(ModulatorSynth::GainModulation) == modParent;

		if (!modParentIsGain)
		{
			// nothing to do here...
			v.initialised = true;
			return;
		}

		if (auto holder = dynamic_cast<DspNetwork::Holder*>(parentSynth))
		{
			holder->setVoiceKillerToUse(&v);
			v.initialised = true;
		}
	}
}

float ScriptnodeVoiceKiller::startVoice(int voiceIndex)
{
	getState(voiceIndex)->active.store(true); return 1.0f;
}

hise::ProcessorEditorBody * ScriptnodeVoiceKiller::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new EmptyProcessorEditorBody(parentEditor);;
#else
	ignoreUnused(parentEditor);
	return nullptr;
#endif
}




} // namespace hise
