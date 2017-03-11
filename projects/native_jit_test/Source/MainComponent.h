/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"




//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component,
							   public ButtonListener,
							   public Timer
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&) override;
    void resized() override;

	void buttonClicked(Button* b) override;

	void timerCallback() override
	{
		table->updateContent();
		table->repaint();
	}

private:

	struct VariableTableModel : public TableListBoxModel
	{
		VariableTableModel(MainContentComponent* parent_) :
			parent(parent_)
		{};

		int getNumRows() override
		{
			if (parent->module != nullptr)
			{
				return parent->module->getScope()->getNumGlobalVariables();
			}
			return 0;
		}

		void paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected)
		{
			g.fillAll(rowIsSelected ? Colours::lightblue : Colours::lightgrey);
		}

		void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
		{
			if (parent->module == nullptr) return;

			g.setColour(Colours::black);

			if (columnId == 1)
			{
				String t = (parent->module->getScope()->getGlobalVariableType(rowNumber).name());
				g.drawText(t, 0, 0, width, height, Justification::centred);
			}

			if (columnId == 2)
			{
				String t = (parent->module->getScope()->getGlobalVariableName(rowNumber).toString());
				g.drawText(t, 0, 0, width, height, Justification::centred);
			}

			if (columnId == 3)
			{
				String t = (parent->module->getScope()->getGlobalVariableValue(rowNumber).toString());
				g.drawText(t, 0, 0, width, height, Justification::centred);
			}
		}

		MainContentComponent* parent;
	};

	ScopedPointer<CodeDocument> doc;
	ScopedPointer<CodeTokeniser> tokeniser;
	ScopedPointer<CodeEditorComponent> editor;
	ScopedPointer<TextButton> runButton;
	ScopedPointer<Label> messageBox;
	
	ScopedPointer<NativeJITCompiler> compiler;

	ScopedPointer<NativeJITDspModule> module;

	ScopedPointer<TableListBox> table;
	ScopedPointer<TableListBoxModel> tableModel;

	ScopedPointer<FileLogger> fileLogger;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
