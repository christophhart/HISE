/*
  ==============================================================================

    HiseProtoPlugProcessors.h
    Created: 6 Sep 2015 12:32:52pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef HISEPROTOPLUGPROCESSORS_H_INCLUDED
#define HISEPROTOPLUGPROCESSORS_H_INCLUDED

/*
class ProtoplugMidiProcessor : public MidiProcessor
{

};

class ProtoplugModulatorSynth : public ModulatorSynth
{

};
*/

class ProtoplugEffectProcessor : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("ProtoPlugEffectProcessor", "Protoplug Effect");

	enum EditorStates
	{
		EditorShown = Processor::numEditorStates,
		AmountChainShown,
		numEditorStates
	};

	ProtoplugEffectProcessor(MainController *mc, const String &id);
	~ProtoplugEffectProcessor();

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;

	bool hasTail() const override { return false; };
	int getNumChildProcessors() const override { return 1; };
	int getNumInternalChains() const override { return 1; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return mixChain; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return mixChain; };

	AudioSampleBuffer &getBufferForChain(int /*chainIndex*/) { return dryWetBuffer; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

private:

	friend class ProtoplugEditor;

	ScopedPointer<ModulatorChain> mixChain;

	AudioSampleBuffer mixBuffer;

	AudioSampleBuffer dryWetBuffer;

	friend class ProtoPlugParameterContainer;

	ScopedPointer<LuaProtoplugJuceAudioProcessor> internalProcessor;

	ScopedPointer<AudioPlayHead> playHead;

	MidiBuffer emptyMidiBuffer;

};


class ProtoPlugParameterContainer : public Component,
									public SafeChangeListener
{
public:

	ProtoPlugParameterContainer(Processor *parent);

	~ProtoPlugParameterContainer();

	void changeListenerCallback(SafeChangeBroadcaster *) override;

	void resized() override;

	void rebuildParameters();

	void updateParameters();

	int visibleHeights;

private:

	

	WeakReference<Processor> processor;

	OwnedArray<MacroControlledObject> components;
};


class ProtoplugEditor: public ProcessorEditorBody
{
public:

	ProtoplugEditor(ProcessorEditor *parentEditor, AudioProcessorEditor *internalEditor);

	~ProtoplugEditor();

	void updateGui() override;

	int getBodyHeight() const override;

	void resized() override;

private:

	KnobLookAndFeel knlaf;

	ScopedPointer<AudioProcessorEditor> internalEditor;

	ScopedPointer<ProtoPlugParameterContainer> parameterContainer;
};



#endif  // HISEPROTOPLUGPROCESSORS_H_INCLUDED
