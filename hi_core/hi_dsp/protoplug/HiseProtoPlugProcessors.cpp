/*
  ==============================================================================

    HiseProtoPlugProcessors.cpp
    Created: 6 Sep 2015 12:32:52pm
    Author:  Christoph

  ==============================================================================
*/

#include "HiseProtoPlugProcessors.h"

ProtoplugEffectProcessor::ProtoplugEffectProcessor(MainController *mc, const String &id):
MasterEffectProcessor(mc, id),
mixChain(new ModulatorChain(mc, "Amount Modulation", 1, Modulation::GainMode, this)),
internalProcessor(new LuaProtoplugJuceAudioProcessor())
{
	editorStateIdentifiers.add("EditorShown");
	editorStateIdentifiers.add("AmountChainShown");

	internalProcessor->setPlayHead(dynamic_cast<AudioProcessor*>(getMainController())->getPlayHead());

	useStepSizeCalculation(false);

}

ProtoplugEffectProcessor::~ProtoplugEffectProcessor()
{
	internalProcessor = nullptr;
	mixChain = nullptr;
}

float ProtoplugEffectProcessor::getAttribute(int parameterIndex) const
{
	return internalProcessor->getParameter(parameterIndex);
}

void ProtoplugEffectProcessor::setInternalAttribute(int parameterIndex, float newValue)
{
	internalProcessor->setParameter(parameterIndex, newValue);
}

void ProtoplugEffectProcessor::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	MemoryBlock mb(*v.getProperty("Data").getBinaryData());

	internalProcessor->setStateInformation(mb.getData(), mb.getSize());
}

ValueTree ProtoplugEffectProcessor::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	MemoryBlock mb;

	internalProcessor->getStateInformation(mb);

	v.setProperty("Data", var(mb), nullptr);

	return v;
}

void ProtoplugEffectProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	internalProcessor->setPlayConfigDetails(2, 2, sampleRate, samplesPerBlock);

	internalProcessor->prepareToPlay(sampleRate, samplesPerBlock);

	mixChain->prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0.0)
	{
		mixBuffer = AudioSampleBuffer(2, samplesPerBlock);
	}

}

void ProtoplugEffectProcessor::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	

	FloatVectorOperations::copy(mixBuffer.getWritePointer(0, startSample), buffer.getReadPointer(0, startSample), numSamples);
	FloatVectorOperations::copy(mixBuffer.getWritePointer(1, startSample), buffer.getReadPointer(1, startSample), numSamples);

	internalProcessor->processBlock(mixBuffer, emptyMidiBuffer);
	
	float *modValues = dryWetBuffer.getWritePointer(0, startSample);

	FloatVectorOperations::multiply(mixBuffer.getWritePointer(0, startSample), modValues, numSamples);
	FloatVectorOperations::multiply(mixBuffer.getWritePointer(1, startSample), modValues, numSamples);

	FloatVectorOperations::multiply(modValues, -1.0f, numSamples);
	FloatVectorOperations::add(modValues, 1.0f, numSamples);

	FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), modValues, numSamples);
	FloatVectorOperations::multiply(buffer.getWritePointer(1, startSample), modValues, numSamples);

	FloatVectorOperations::add(buffer.getWritePointer(0, startSample), mixBuffer.getReadPointer(0, startSample), numSamples);
	FloatVectorOperations::add(buffer.getWritePointer(1, startSample), mixBuffer.getReadPointer(1, startSample), numSamples);
}

ProcessorEditorBody * ProtoplugEffectProcessor::createEditor(BetterProcessorEditor *parentEditor)
{

	AudioProcessorEditor *editor = internalProcessor->createEditorIfNeeded();

	return new ProtoplugEditor(parentEditor, editor);
}

ProtoPlugParameterContainer::ProtoPlugParameterContainer(Processor *parent):
processor(parent)
{
	LuaProtoplugJuceAudioProcessor *p = dynamic_cast<ProtoplugEffectProcessor*>(processor.get())->internalProcessor;

	p->addChangeListener(this);


	rebuildParameters();
}

ProtoPlugParameterContainer::~ProtoPlugParameterContainer()
{
	LuaProtoplugJuceAudioProcessor *p = dynamic_cast<ProtoplugEffectProcessor*>(processor.get())->internalProcessor;

	p->removeChangeListener(this);
}

void ProtoPlugParameterContainer::changeListenerCallback(SafeChangeBroadcaster *)
{
	rebuildParameters();
	dynamic_cast<ProcessorEditorBody*>(getParentComponent())->refreshBodySize();
}

void ProtoPlugParameterContainer::resized()
{
	for (int i = 0; i < components.size(); i++)
	{
		dynamic_cast<Component*>(components[i])->setSize(128, 48);

		dynamic_cast<Component*>(components[i])->setTopLeftPosition((i % 6) * 128, i / 6 * 48);
	}
}


void ProtoPlugParameterContainer::rebuildParameters()
{

	components.clear();

	LuaProtoplugJuceAudioProcessor *p = dynamic_cast<ProtoplugEffectProcessor*>(processor.get())->internalProcessor;

	int visibleParameters = 0;

	for (int i = 0; i < 24; i++)
	{
		if(p->getParameterName(i).isEmpty()) continue;

		visibleParameters++;

		HiSlider *s = new HiSlider("Parameter 1");

		s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
		s->setTextBoxStyle(Slider::TextBoxRight, false, 80, 20);

		

		s->setup(processor, i, p->getParameterName(i));
		
		s->setMode(HiSlider::NormalizedPercentage);

		addAndMakeVisible(s);

		components.add(s);

		s->updateValue();
	}

	visibleHeights = visibleParameters == 0 ? 0 : 1 + visibleParameters / 4;

	resized();
}

void ProtoPlugParameterContainer::updateParameters()
{
	for (int i = 0; i < components.size(); i++)
	{
		components[i]->updateValue();
	}
}

ProtoplugEditor::ProtoplugEditor(BetterProcessorEditor *parentEditor, AudioProcessorEditor *internalEditor_) :
ProcessorEditorBody(parentEditor),
internalEditor(internalEditor_)
{
	setLookAndFeel(&knlaf);

	if (internalEditor != nullptr)
	{
		
		addAndMakeVisible(internalEditor);

		dynamic_cast<LuaProtoplugJuceAudioProcessorEditor*>(internalEditor_)->popOut();

		addAndMakeVisible(parameterContainer = new ProtoPlugParameterContainer(parentEditor->getProcessor()));
	}
}

ProtoplugEditor::~ProtoplugEditor()
{
	dynamic_cast<ProtoplugEffectProcessor*>(getProcessor())->internalProcessor->editorBeingDeleted(internalEditor);

	internalEditor = nullptr;
}

void ProtoplugEditor::updateGui()
{
	parameterContainer->updateParameters();
}

int ProtoplugEditor::getBodyHeight() const
{
	if (internalEditor != nullptr)
	{
		return 20 + parameterContainer->visibleHeights * 48 + 10;
	}
}

void ProtoplugEditor::resized()
{
	if (internalEditor != nullptr)
	{
		internalEditor->setBounds(0, 0, getWidth(), 20);
		parameterContainer->setBounds(12, 20, getWidth()-24, getHeight() - 25);
	}
}

