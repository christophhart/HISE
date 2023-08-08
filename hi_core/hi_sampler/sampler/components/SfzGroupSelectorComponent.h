/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_819F089F4B4869C0__
#define __JUCE_HEADER_819F089F4B4869C0__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SfzGroupSelectorComponent  : public Component,
                                   public ComboBoxListener
{
public:
    //==============================================================================
    SfzGroupSelectorComponent ();
    ~SfzGroupSelectorComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void setData(int index, const String &name, int numGroups)
	{
		

		for(int i = 0; i < numGroups; i++)
		{
			rrGroupSelector->addItem(String(i + 1), i +1);
			
		}

		rrGroupSelector->addItem("Ignore", -1);

		GroupName->setText(name, dontSendNotification);

		rrGroupSelector->setSelectedId(index + 1, dontSendNotification);

	}

	int getGroupIndex() const
	{
		return fixedReturnValue ? 1 : rrGroupSelector->getSelectedId();
	}

	void setFixedReturnValue()
	{
		fixedReturnValue = true;
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	

	bool fixedReturnValue;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<ComboBox> rrGroupSelector;
    ScopedPointer<Label> GroupName;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfzGroupSelectorComponent)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_819F089F4B4869C0__
