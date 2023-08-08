/*
  ==============================================================================

    SlotFXEditor.cpp
    Created: 28 Jun 2017 2:52:03pm
    Author:  Christoph

  ==============================================================================
*/

namespace hise { using namespace juce;

#include "SlotFXEditor.h"

SlotFXEditor::SlotFXEditor(ProcessorEditor* parentEditor) :
	ProcessorEditorBody(parentEditor)
{
	addAndMakeVisible(effectSelector = new ComboBox());

	effectSelector->addItemList(dynamic_cast<SlotFX*>(getProcessor())->getModuleList(), 1);

	getProcessor()->getMainController()->skin(*effectSelector);

	effectSelector->addListener(this);
}

void SlotFXEditor::comboBoxChanged(ComboBox* /*comboBoxThatHasChanged*/)
{
	auto newEffectName = effectSelector->getText();

	auto f = [newEffectName](Processor*p)
	{
		dynamic_cast<SlotFX*>(p)->setEffect(newEffectName);
		return SafeFunctionCall::OK;
	};

	auto p = getProcessor();

	p->getMainController()->getKillStateHandler().killVoicesAndCall(p, f, MainController::KillStateHandler::SampleLoadingThread);
	
}

} // namespace hise
