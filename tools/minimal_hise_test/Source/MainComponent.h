/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component,
					    public Timer,
					    public Thread
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    void run() override
    {
	    UnitTestRunner r;
		r.setAssertOnFailure(true);
		r.setPassesAreLogged(false);
		r.runTestsInCategory("dispatch");
        
        Timer::callAfterDelay(500, [&]()
        {
            viewer.start(false);
        });
    }

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override;;

    void mouseDown(const MouseEvent& e) override
    {
        jassertfalse;
    }
    
private:
    //==============================================================================
    // Your private member variables go here...
    int counter = 0;
	OpenGLContext context;

    PerfettoWebviewer viewer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


