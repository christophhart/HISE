/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "multipage.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component,
					    public Timer,
					    public Thread,
					    public ButtonListener
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

    void buttonClicked(Button* b) override
    {
        if(b == &editButton)
        {
	        if(preview != nullptr)
				preview->setVisible(false);

        	stateViewer.setVisible(false);
            c->setVisible(true);
        }
        

        if(b == &codeButton)
        {
            if(preview != nullptr)
                preview->setVisible(false);

            if(c != nullptr)
            {
	            auto ok = c->checkCurrentPage();

                stateViewer.setVisible(ok.wasOk());
            	c->setVisible(ok.failed());

                if(ok.wasOk())
                    doc.replaceAllContent(JSON::toString(rt.globalState, false));
            }
        }

        if(b == &previewButton)
        {
            auto ok = c->checkCurrentPage();

            if(ok.wasOk())
            {
	            stateViewer.setVisible(false);
	            c->setVisible(false);

		        addAndMakeVisible(preview = new multipage::Dialog(rt.globalState, pt));

            	preview->showFirstPage();
    
				preview->setFinishCallback([](){});

	            resized();
            }
        }
        
    }

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override;;
    
    void build();
    
private:
    //==============================================================================
    // Your private member variables go here...
    int counter = 0;
	OpenGLContext context;

    PerfettoWebviewer viewer;

    multipage::State pt;
    multipage::State rt;
    ScopedPointer<multipage::Dialog> c;

    juce::CodeDocument doc;
    mcl::TextDocument stateDoc;

    mcl::TextEditor stateViewer;

    ScopedPointer<multipage::Dialog> preview;

    AlertWindowLookAndFeel alaf;
    TextButton editButton, codeButton, previewButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


