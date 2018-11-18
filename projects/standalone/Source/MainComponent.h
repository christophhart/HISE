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
class MainContentComponent   : public Component
{
public:
    //==============================================================================
    MainContentComponent(const String &commandLine);

	void handleCommandLineArguments(const String& args);

	~MainContentComponent();

    void paint (Graphics&);
    void resized();

	void requestQuit();

private:

	ScopedPointer<AudioProcessorEditor> editor;
	ScopedPointer<hise::StandaloneProcessor> standaloneProcessor;

	ScopedPointer<hise::FloatingTile> root;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
