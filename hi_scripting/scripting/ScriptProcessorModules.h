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

#ifndef SCRIPTPROCESSORMODULES_H_INCLUDED
#define SCRIPTPROCESSORMODULES_H_INCLUDED

namespace hise { using namespace juce;


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
class JavascriptMidiProcessor : public ScriptBaseMidiProcessor,
								public JavascriptProcessor,
								public Timer
{
public:

	SET_PROCESSOR_NAME("ScriptProcessor", "Script Processor", "MIDI Processor that allows scripting.")

	enum SnippetsOpen
	{
		onNoteOnOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		onNoteOffOpen,
		onControllerOpen,
		onTimerOpen,
		onControlOpen,
		externalPopupShown,
		numScriptEditorStates
	};

	JavascriptMidiProcessor(MainController *mc, const String &id);
	~JavascriptMidiProcessor();;

	Path getSpecialSymbol() const override;

	void suspendStateChanged(bool shouldBeSuspended) override;

	ValueTree exportAsValueTree() const override;;
	void restoreFromValueTree(const ValueTree &v) override;
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override;
	void registerApiClasses() override;

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override
	{
		return getContentParameterIdentifier(parameterIndex);
	}

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		return getContentParameterIdentifierIndex(id);
	}

	int getNumAttributes() const override
	{
		return getContentParameterAmount();
	}

	void addToFront(bool addToFront_) noexcept;;
	bool isFront() const;;

	StringArray getImageFileNames() const;

	/** This defers the callbacks to the message thread.
	*
	*	It stops all timers and clears any message queues.
	*/
	void deferCallbacks(bool addToFront_);
	bool isDeferred() const;;

	void timerCallback() override;

	void processHiseEvent(HiseEvent &m) override;

	static JavascriptMidiProcessor* getFirstInterfaceScriptProcessor(MainController* mc);

	ScriptingApi::Server::WeakPtr getServerObject();

private:

	struct DeferredExecutioner : public LockfreeAsyncUpdater
	{
		DeferredExecutioner(JavascriptMidiProcessor* jp);;

		void addPendingEvent(const HiseEvent& e);

	private:

		void handleAsyncUpdate() override;

		LockfreeQueue<HiseEvent> pendingEvents;
		JavascriptMidiProcessor& parent;
	};

	DeferredExecutioner deferredExecutioner;


	void runTimerCallback(int offsetInBuffer = -1);
	void runScriptCallbacks();

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onNoteOnCallback;
	ScopedPointer<SnippetDocument> onNoteOffCallback;
	ScopedPointer<SnippetDocument> onControllerCallback;
	ScopedPointer<SnippetDocument> onControlCallback;
	ScopedPointer<SnippetDocument> onTimerCallback;

	ReadWriteLock defferedMessageLock;

	MidiBuffer deferredMidiMessages;
	MidiBuffer copyBuffer;

	HiseEventBuffer deferredEvents;
	HiseEventBuffer copyEventBuffer;

	ReferenceCountedObjectPtr<ScriptingApi::Message> currentMidiMessage;
	ReferenceCountedObjectPtr<ScriptingApi::Engine> engineObject;

	ScriptingApi::Server::WeakPtr serverObject;

	ScriptingApi::Sampler *samplerObject;
	ScriptingApi::Synth *synthObject;

	bool front, deferred, deferredUpdatePending;

	

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptMidiProcessor);
};

class JavascriptVoiceStartModulator : public JavascriptProcessor,
									  public ProcessorWithScriptingContent,
									  public VoiceStartModulator
{
public:

	enum Callback
	{
		onInit = 0,
		onVoiceStart,
		onVoiceStop,
		onController,
		onControl,
		numCallbacks
	};

	enum EditorStates
	{
		onVoiceStartOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		onVoiceStopOpen,
		onControllerOpen,
		onControlOpen,
		externalPopupShown,
		numScriptEditorStates
	};

	SET_PROCESSOR_NAME("ScriptVoiceStartModulator", "Script Voice Start Modulator", "Creates a scriptable modulation value at the start of the voice.")


	JavascriptVoiceStartModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);;
	~JavascriptVoiceStartModulator();

	Path getSpecialSymbol() const override;

	float getAttribute(int index) const override { return getControlValue(index); }
	void setInternalAttribute(int index, float newValue) override { setControlValue(index, newValue); }

	ValueTree exportAsValueTree() const override { ValueTree v = VoiceStartModulator::exportAsValueTree(); saveContent(v); saveScript(v); return v; }
	void restoreFromValueTree(const ValueTree &v) override { VoiceStartModulator::restoreFromValueTree(v); restoreScript(v); restoreContent(v); }
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	float calculateVoiceStartValue(const HiseEvent &/*m*/) override { return 0.0f; };
	virtual void handleHiseEvent(const HiseEvent &m) override;

	/** When the startNote function is called, a previously calculated value (by the handleMidiMessage function) is stored using the supplied voice index. */
	virtual float startVoice(int voiceIndex) override;;

	int getNumAttributes() const override
	{
		return getContentParameterAmount();
	}

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override
	{
		return getContentParameterIdentifier(parameterIndex);
	}

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		return getContentParameterIdentifierIndex(id);
	}

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override { return numCallbacks; }
	void registerApiClasses() override;
	
	int getControlCallbackIndex() const override { return (int)Callback::onControl; };

private:

	ReferenceCountedObjectPtr<ScriptingApi::Message> currentMidiMessage;
	ReferenceCountedObjectPtr<ScriptingApi::Engine> engineObject;

	ScriptingApi::Sampler *samplerObject;
	ScriptingApi::Synth *synthObject;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onVoiceStartCallback;
	ScopedPointer<SnippetDocument> onVoiceStopCallback;
	ScopedPointer<SnippetDocument> onControllerCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JavascriptVoiceStartModulator);
	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptVoiceStartModulator);
};


class JavascriptTimeVariantModulator : public JavascriptProcessor,
									   public ProcessorWithScriptingContent,
									   public TimeVariantModulator
{
public:

	SET_PROCESSOR_NAME("ScriptTimeVariantModulator", "Script Time Variant Modulator", "Creates a scriptable monophonic modulation signal.")

	enum Callback
	{
		onInit = 0,
		prepare,
		processBlock,
		onNoteOn,
		onNoteOff,
		onController,
		onControl,
		numCallbacks
	};

	enum EditorStates
	{
		prepareToPlayOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		processBlockOpen,
		onNoteOnOpen,
		onNoteOffOpen,
		onControllerOpen,
		onControlOpen,
		externalPopupShown,
		numScriptEditorStates
	};

	JavascriptTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m);
	~JavascriptTimeVariantModulator();

	Path getSpecialSymbol() const override;

	float getAttribute(int index) const override;

	void setInternalAttribute(int index, float newValue) override;

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override;

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameterIndexForIdentifier(id);
		else
			return contentParameterHandler.getParameterIndexForIdentifier(id);
	}

	int getNumAttributes() const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getNumParameters();
		else
			return contentParameterHandler.getNumParameters();
	}

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void handleHiseEvent(const HiseEvent &m) override;
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;;

	Processor *getChildProcessor(int /*processorIndex*/) override final;;
	const Processor *getChildProcessor(int /*processorIndex*/) const override final;;
	int getNumChildProcessors() const override final;;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override;
	void registerApiClasses() override;
	
	int getControlCallbackIndex() const override;;

	void postCompileCallback() override;

private:

	ReferenceCountedObjectPtr<ScriptingApi::Message> currentMidiMessage;
	ReferenceCountedObjectPtr<ScriptingApi::Engine> engineObject;
	ScriptingApi::Synth *synthObject;

	VariantBuffer::Ptr buffer;
	var bufferVar;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> prepareToPlayCallback;
	ScopedPointer<SnippetDocument> processBlockCallback;
	ScopedPointer<SnippetDocument> onNoteOnCallback;
	ScopedPointer<SnippetDocument> onNoteOffCallback;
	ScopedPointer<SnippetDocument> onControllerCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JavascriptTimeVariantModulator);
	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptTimeVariantModulator);

};

class ScriptnodeVoiceKiller : public EnvelopeModulator,
							  public snex::Types::VoiceResetter
{
public:


	SET_PROCESSOR_NAME("ScriptnodeVoiceKiller", "Scriptnode Voice Killer", "kills the voices from a scriptnode envelope's gate output")

		ScriptnodeVoiceKiller(MainController* mc, const String& id, int numVoices);;

	static void initialiseNetworks(ScriptnodeVoiceKiller& v);

	void setInternalAttribute(int parameter_index, float newValue) override;
	float getDefaultValue(int parameterIndex) const override;
	float getAttribute(int parameter_index) const;

	int getNumInternalChains() const override;;
	int getNumChildProcessors() const override;;
	Processor *getChildProcessor(int) override;;
	const Processor *getChildProcessor(int) const override;;

	float startVoice(int voiceIndex) final override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) final override;
	bool isPlaying(int voiceIndex) const override;

	void calculateBlock(int startSample, int numSamples) override;
	void handleHiseEvent(const HiseEvent& m) override;


	struct State : public ModulatorState
	{
		State(int v);;
		std::atomic<bool> active = { false };
	};

	int getNumActiveVoices() const;

	void onVoiceReset(bool allVoices, int voiceIndex) final override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	State* getState(int i);
	const State* getState(int i) const;

	ModulatorState *createSubclassedState(int voiceIndex) const override;;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptnodeVoiceKiller);

	bool initialised = false;
};




class JavascriptEnvelopeModulator : public JavascriptProcessor,
								    public ProcessorWithScriptingContent,
									public EnvelopeModulator,
									public snex::Types::VoiceResetter
{
public:

	SET_PROCESSOR_NAME("ScriptEnvelopeModulator", "Script Envelope Modulator", "Creates a scriptable polyphonic modulation signal.")

	enum Callback
	{
		onInit = 0,
		onControl,
		numCallbacks
	};

	enum EditorStates
	{
		prepareToPlayOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		onControlOpen,
		externalPopupShown,
		numScriptEditorStates
	};

	JavascriptEnvelopeModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);
	~JavascriptEnvelopeModulator();

	Path getSpecialSymbol() const override;

	bool isPolyphonic() const override;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;


	int getNumActiveVoices() const override;

	void onVoiceReset(bool allVoices, int voiceIndex) final override;

	int getNumParameters() const override;

	void setInternalAttribute(int index, float newValue) override;

	float getAttribute(int index) const override;

	Identifier getIdentifierForParameterIndex(int index) const override;

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameterIndexForIdentifier(id);
		else
			return contentParameterHandler.getParameterIndexForIdentifier(id);
	}

	int getNumAttributes() const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getNumParameters();
		else
			return contentParameterHandler.getNumParameters();
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void handleHiseEvent(const HiseEvent &m) override;
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;;

	float startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) final override;
	bool isPlaying(int voiceIndex) const override;

	Processor *getChildProcessor(int /*processorIndex*/) override final;;
	const Processor *getChildProcessor(int /*processorIndex*/) const override final;;
	int getNumChildProcessors() const override final;;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override;
	void registerApiClasses() override;

	int getControlCallbackIndex() const override;;

	void postCompileCallback() override;

private:

	int currentVoiceIndex = -1;

	struct ScriptEnvelopeState : public EnvelopeModulator::ModulatorState
	{
		ScriptEnvelopeState(int voiceIndex_);;

		float uptime = 0.0f;
		bool isPlaying = false;
		bool isRingingOff = false;
		HiseEvent noteOnEvent;
	};

	HiseEvent lastNoteOn;
	
	VoiceDataStack voiceData;

	ModulatorState *createSubclassedState(int voiceIndex) const override;;

	ReferenceCountedObjectPtr<ScriptingApi::Message> currentMidiMessage;
	ReferenceCountedObjectPtr<ScriptingApi::Engine> engineObject;
	ScriptingApi::Synth *synthObject;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JavascriptEnvelopeModulator);
	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptEnvelopeModulator);
};


class JavascriptMasterEffect : public JavascriptProcessor,
							   public ProcessorWithScriptingContent,
							   public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("ScriptFX", "Script FX", "A scriptable audio effect.");

	enum class Callback
	{
		onInit,
		prepareToPlay,
		processBlock,
		onControl,
		numCallbacks
	};

	enum EditorStates
	{
		prepareToPlayOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		processBlockOpen,
		onControlOpen,	
		externalPopupShown,
		numScriptEditorStates
	};
	
	JavascriptMasterEffect(MainController *mc, const String &id);
	~JavascriptMasterEffect();
	

	Path getSpecialSymbol() const override;

	void connectionChanged() override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override;
	void registerApiClasses() override;
	void postCompileCallback() override;


	void voicesKilled() override;

	bool hasTail() const override;;

	bool isSuspendedOnSilence() const override;

	Processor *getChildProcessor(int /*processorIndex*/) override;;
	const Processor *getChildProcessor(int /*processorIndex*/) const override;;

	int getNumInternalChains() const override;;
	int getNumChildProcessors() const override;;

	virtual void renderWholeBuffer(AudioSampleBuffer &buffer);;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override;

    
    
	float getAttribute(int index) const override;

	void setInternalAttribute(int index, float newValue) override;

	void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler) noexcept override;

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override;

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameterIndexForIdentifier(id);
		else
			return contentParameterHandler.getParameterIndexForIdentifier(id);
	}

	int getNumAttributes() const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getNumParameters();
		else
			return contentParameterHandler.getNumParameters();
	}

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	int getControlCallbackIndex() const override;;

private:

	var buffers[NUM_MAX_CHANNELS];

	Array<var> channels;

	var channelData;

	Array<int, DummyCriticalSection> channelIndexes;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> prepareToPlayCallback;
	ScopedPointer<SnippetDocument> processBlockCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	ScriptingApi::Engine* engineObject;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptMasterEffect);
};

class JavascriptPolyphonicEffect : public JavascriptProcessor,
	public ProcessorWithScriptingContent,
	public VoiceEffectProcessor,
    public VoiceResetter
{
public:

	SET_PROCESSOR_NAME("PolyScriptFX", "Polyphonic Script FX", "A polyphonic scriptable audio effect.");

	enum class Callback
	{
		onInit,
		onControl,
		numCallbacks
	};

	enum EditorStates
	{
		onControlOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		externalPopupShown,
		numScriptEditorStates
	};

	JavascriptPolyphonicEffect(MainController *mc, const String &id, int numVoices);
	~JavascriptPolyphonicEffect();

	bool isPolyphonic() const override { return true; }

	Path getSpecialSymbol() const override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override { return (int)Callback::numCallbacks; }
	void registerApiClasses() override;
	void postCompileCallback() override;

	bool hasTail() const override;;

	bool isSuspendedOnSilence() const override;

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	ValueTree exportAsValueTree() const override { ValueTree v = VoiceEffectProcessor::exportAsValueTree(); saveContent(v); saveScript(v); return v; }
	void restoreFromValueTree(const ValueTree &v) override { VoiceEffectProcessor::restoreFromValueTree(v); restoreScript(v); restoreContent(v); }

	float getAttribute(int index) const override
	{
		return getCurrentNetworkParameterHandler(&contentParameterHandler)->getParameter(index);
	}

	void setInternalAttribute(int index, float newValue) override
	{
		getCurrentNetworkParameterHandler(&contentParameterHandler)->setParameter(index, newValue);
	}

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override
	{
		return getCurrentNetworkParameterHandler(&contentParameterHandler)->getParameterId(parameterIndex);
	}

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameterIndexForIdentifier(id);
		else
			return contentParameterHandler.getParameterIndexForIdentifier(id);
	}

	int getNumAttributes() const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getNumParameters();
		else
			return contentParameterHandler.getNumParameters();
	}

	int getControlCallbackIndex() const override { return (int)Callback::onControl; };

	void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) final override;

	void startVoice(int voiceIndex, const HiseEvent& e) final override;

	void stopVoice(int ) final override {}

	void reset(int voiceIndex) final override;

	void handleHiseEvent(const HiseEvent &m) final override;;

	void applyEffect(int , AudioSampleBuffer &, int , int ) final override
	{
		jassertfalse;
	}

	void renderNextBlock(AudioSampleBuffer &, int , int ) final override
	{

	}

    int getNumActiveVoices() const override
    {
        return voiceData.voiceNoteOns.size();
    }

    void onVoiceReset(bool allVoices, int voiceIndex) override
    {
        if (allVoices)
            voiceData.voiceNoteOns.clear();
        else
            voiceData.reset(voiceIndex);
    }
    
private:

	VoiceDataStack voiceData;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	ScriptingApi::Engine* engineObject;
};

class JavascriptSynthesiser : public JavascriptProcessor,
						  public ProcessorWithScriptingContent,
						  public ModulatorSynth
{
public:

	enum ModChains
	{
		Extra1 = 2,
		Extra2,
		numModChains
	};

	struct Sound : public ModulatorSynthSound
	{
		bool appliesToNote(int ) final override;;
		bool appliesToChannel(int ) final override;;
		bool appliesToVelocity(int ) final override;;
	};

	struct Voice : public ModulatorSynthVoice
	{
		Voice(JavascriptSynthesiser* p);

		void calculateBlock(int startSample, int numSamples) override;

		void setVoiceStartDataForNextRenderCallback();

		virtual void resetVoice() override;

		JavascriptSynthesiser* synth;

		bool isVoiceStart = false;
	};
	
	SET_PROCESSOR_NAME("ScriptSynth", "Scriptnode Synthesiser", "A polyphonic scriptable synthesiser.");

	enum class Callback
	{
		onInit,
		onControl,
		numCallbacks
	};

	enum EditorStates
	{
		onControlOpen = ProcessorWithScriptingContent::EditorStates::numEditorStates,
		externalPopupShown,
		numScriptEditorStates
	};

	JavascriptSynthesiser(MainController *mc, const String &id, int numVoices);
		
	~JavascriptSynthesiser();

	Path getSpecialSymbol() const override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override;
	void registerApiClasses() override;
	void postCompileCallback() override;

	void preHiseEventCallback(HiseEvent &e) override;

	void preStartVoice(int voiceIndex, const HiseEvent& e) override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	bool isPolyphonic() const override;

	float getModValueForNode(int modIndex, int startSample) const;

	Processor* getChildProcessor(int processorIndex) override;

	const Processor* getChildProcessor(int processorIndex) const override;

	int getNumInternalChains() const override;;

	int getNumChildProcessors() const override;;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	int getNumParameters() const override;

	float getAttribute(int index) const override;

	void setInternalAttribute(int index, float newValue) override;

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override;

	int getParameterIndexForIdentifier(const Identifier& id) const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getParameterIndexForIdentifier(id);
		else
			return contentParameterHandler.getParameterIndexForIdentifier(id);
	}

	int getNumAttributes() const override
	{
		if (auto n = getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getNumParameters();
		else
			return contentParameterHandler.getNumParameters();
	}

	int getControlCallbackIndex() const override;;

	ModulatorChain::ModChainWithBuffer* nodeChains[3];

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	VoiceDataStack voiceData;

	ScriptingApi::Engine* engineObject;

	int currentVoiceStartSample = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptSynthesiser);
};


} // namespace hise
#endif  // SCRIPTPROCESSORMODULES_H_INCLUDED
