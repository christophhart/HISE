/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_F82F7FBC07454100__
#define __JUCE_HEADER_F82F7FBC07454100__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"

class RetriggerLabel : public Label
{

public:

	RetriggerLabel(const String& componentName = String::empty,
		const String& labelText = String::empty) :
		Label(componentName, labelText),
		notifyIfNothingChanges(false)
	{}

	void setNotifyIfNothingChanges(bool shouldNotify) noexcept{ notifyIfNothingChanges = shouldNotify; };

private:

	void textEditorReturnKeyPressed(TextEditor& ed)
	{
		if (getCurrentTextEditor() != nullptr)
		{
			jassert(&ed == getCurrentTextEditor());

			const String newText(ed.getText());

			setText(newText, dontSendNotification);

			hideEditor(true);

			WeakReference<Component> deletionChecker(this);
			textWasEdited();

			if (deletionChecker != nullptr)
				callChangeListeners();
		}
	};

	bool notifyIfNothingChanges;
};

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SamplerTable  : public Component,
                      public SamplerSubEditor,
                      public LabelListener
{
public:
    //==============================================================================
    SamplerTable (ModulatorSampler *s, SamplerBody *b);
    ~SamplerTable();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void refreshList()
	{
		table->refreshList();
	}

	void soundsSelected(const Array<ModulatorSamplerSound*> &selection) override
	{
		table->soundsSelected(selection);

		numSelected = selection.size();

		repaint();
	}

	

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);

	

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	ModulatorSampler *sampler;
	SamplerBody *body;

	int numSelected;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<SamplerSoundTable> table;
    ScopedPointer<RetriggerLabel> searchLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerTable)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_F82F7FBC07454100__
