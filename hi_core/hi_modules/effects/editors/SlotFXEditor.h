/*
  ==============================================================================

    SlotFXEditor.h
    Created: 28 Jun 2017 2:52:03pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef SLOTFXEDITOR_H_INCLUDED
#define SLOTFXEDITOR_H_INCLUDED

namespace hise { using namespace juce;

class SlotFXEditor : public ProcessorEditorBody,
					 public ComboBox::Listener
{
public:

	SlotFXEditor(ProcessorEditor* parentEditor);;

	void resized() override
	{
		effectSelector->setBounds(40, 0, getWidth() - 80, 28);
	}

	int getBodyHeight() const override { return 32; }

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void updateGui() override
	{
		effectSelector->setSelectedId(dynamic_cast<SlotFX*>(getProcessor())->getCurrentEffectID() + 1, dontSendNotification);
	};

private:

	ScopedPointer<ComboBox> effectSelector;

};

} // namespace hise


#endif  // SLOTFXEDITOR_H_INCLUDED
