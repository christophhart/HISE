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

	effectSelector->addItemList(dynamic_cast<SlotFX*>(getProcessor())->getEffectList(), 1);

	getProcessor()->getMainController()->skin(*effectSelector);

	effectSelector->addListener(this);
}

void SlotFXEditor::comboBoxChanged(ComboBox* /*comboBoxThatHasChanged*/)
{
	dynamic_cast<SlotFX*>(getProcessor())->setEffect(effectSelector->getText());
}

} // namespace hise
