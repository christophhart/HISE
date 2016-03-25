/*
  ==============================================================================

    PluginPlayerProcessorEditor.cpp
    Created: 24 Oct 2014 2:35:34pm
    Author:  Chrisboy

  ==============================================================================
*/

#include "PresetProcessorEditor.h"

PresetProcessorEditor::PresetProcessorEditor(PresetProcessor *pp_) :
AudioProcessorEditor(pp_),
pp(pp_)
{

	addAndMakeVisible(header = new PresetHeader(pp_));

	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(pp->getMainSynthChain()));

	interfaceComponent->checkInterfaces();

	interfaceComponent->refreshContentBounds();
	interfaceComponent->setIsFrontendContainer(true);

	setSize(700, getComponentHeight());
}

PresetProcessorEditor::~PresetProcessorEditor()
{
	header = nullptr;

	interfaceComponent = nullptr;
}

void PresetProcessorEditor::resized()
{
	header->setBounds(0, 0, 700, 35);

	const int interfaceWidth = interfaceComponent->getContentWidth() != -1 ? interfaceComponent->getContentWidth() : 700 - 6;

	const int interfaceX = jmax<int>(3, (getWidth() - interfaceWidth) / 2);

	interfaceComponent->setVisible(!pp->getMainSynthChain()->getEditorState(Processor::Folded));

	interfaceComponent->setBounds(interfaceX, 30, interfaceWidth, interfaceComponent->getContentHeight());
	
}

void PresetProcessorEditor::paint(Graphics &g)
{
	
	ProcessorEditorLookAndFeel::drawBackground(g, 700, getHeight(), Colour(0xFF444444), false, 2);
}

int PresetProcessorEditor::getComponentHeight() const
{
	if (pp->getMainSynthChain()->getEditorState(Processor::Folded))
	{
		return 35;
	}
	else
	{
		return interfaceComponent->getContentHeight() + 36;
	}
}
