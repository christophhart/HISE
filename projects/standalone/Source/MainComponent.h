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
							   public ProjectHandler::Listener
{
public:
    //==============================================================================
    MainContentComponent(const String &commandLine);

	void handleCommandLineArguments(const String& args);

    void projectChanged(const File& newRootDirectory)
    {
	    if(auto mc = dynamic_cast<MainController*>(standaloneProcessor->getCurrentProcessor()))
	    {
            String n;
            n << "HISE - ";
            n <<  GET_HISE_SETTING(mc->getMainSynthChain(), HiseSettings::Project::Name).toString();

            if(auto d = findParentComponentOfClass<DocumentWindow>())
                d->setName(n);
	    }
    }

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
