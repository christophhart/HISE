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

#ifndef SCRIPTPROCESSORMODULES_H_INCLUDED
#define SCRIPTPROCESSORMODULES_H_INCLUDED




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
								public Timer,
								public ExternalFileProcessor,
								public AsyncUpdater
{
public:

	SET_PROCESSOR_NAME("ScriptProcessor", "Script Processor")

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

	JavascriptMidiProcessor(MainController *mc, const String &id);;
	~JavascriptMidiProcessor();;

	Path getSpecialSymbol() const override;

	ValueTree exportAsValueTree() const override;;
	void restoreFromValueTree(const ValueTree &v) override;
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void replaceReferencesWithGlobalFolder() override;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override { return numCallbacks; }
	void registerApiClasses() override;
	void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue) override;

	void addToFront(bool addToFront_) noexcept{ front = addToFront_; };
	bool isFront() const { return front; };

	StringArray getImageFileNames() const;

	/** This defers the callbacks to the message thread.
	*
	*	It stops all timers and clears any message queues.
	*/
	void deferCallbacks(bool addToFront_);
	bool isDeferred() const { return deferred; };

	void handleAsyncUpdate() override;

	void timerCallback() override
	{
		jassert(isDeferred());
		runTimerCallback();
	}

	void synthTimerCallback(int offsetInBuffer) override
	{
		jassert(!isDeferred());
		runTimerCallback(offsetInBuffer);
	};

	/** This calls the processMidiMessage() function in the compiled Javascript code. @see ScriptingApi */
	void processMidiMessage(MidiMessage &m) override;

private:

	void runTimerCallback(int offsetInBuffer = -1);
	void runScriptCallbacks();

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> onNoteOnCallback;
	ScopedPointer<SnippetDocument> onNoteOffCallback;
	ScopedPointer<SnippetDocument> onControllerCallback;
	ScopedPointer<SnippetDocument> onControlCallback;
	ScopedPointer<SnippetDocument> onTimerCallback;

	MidiBuffer deferredMidiMessages;
	MidiBuffer copyBuffer;

	ReferenceCountedObjectPtr<ScriptingApi::Message> currentMidiMessage;
	ReferenceCountedObjectPtr<ScriptingApi::Engine> engineObject;

	ScriptingApi::Sampler *samplerObject;
	ScriptingApi::Synth *synthObject;

	bool front, deferred, deferredUpdatePending;
};


class JavascriptMasterEffect : public JavascriptProcessor,
							   public ScriptBaseMasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("ScriptFX", "Script FX")

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

	ValueTree exportAsValueTree() const override;;
	void restoreFromValueTree(const ValueTree &v) override;
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	SnippetDocument *getSnippet(int c) override;
	const SnippetDocument *getSnippet(int c) const override;
	int getNumSnippets() const override { return numCallbacks; }
	void registerApiClasses() override;
	void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue) override;
	void postCompileCallback() override;


	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int processorIndex) override { return nullptr; };
	const Processor *getChildProcessor(int processorIndex) const override { return nullptr; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	void prepareToPlay(double sampleRate, int samplesPerBlock);
	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override;

private:

	VariantBuffer::Ptr bufferL;
	VariantBuffer::Ptr bufferR;

	var channels;

	Result lastResult;

	ScopedPointer<SnippetDocument> onInitCallback;
	ScopedPointer<SnippetDocument> prepareToPlayCallback;
	ScopedPointer<SnippetDocument> processBlockCallback;
	ScopedPointer<SnippetDocument> onControlCallback;

	ScriptingApi::Engine* engineObject;
};


#endif  // SCRIPTPROCESSORMODULES_H_INCLUDED
