/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"



#define DECLARE_HASHED_ID(x)   static const HashedCharPtr x(CharPtr(Identifier(#x)));
#define DECLARE_HASHED_ENUM(enumclass, x) static const HashedCharPtr x(Identifier(#x));
namespace enum_strings
{




}


#define USE_JUCE_WEBVIEW2_BROWSER JUCE_WINDOWS

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
		//r.setPassesAreLogged(true);
		r.runTestsInCategory("dispatch");

        
        Timer::callAfterDelay(500, [&]()
        {
            startButton.setToggleState(false, sendNotificationSync);
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

    
    TextButton b;

#if USE_JUCE_WEBVIEW2_BROWSER
    ScopedPointer<WindowsWebView2WebBrowserComponent> browser;
#else

    WebViewData::Ptr data;
    ScopedPointer<WebViewWrapper> browser;
#endif
    
    
    struct Paths: public PathFactory
    {
        Path createPath(const String& url) const override
        {
            Path p;
            
            LOAD_EPATH_IF_URL("start", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
            LOAD_EPATH_IF_URL("cancel", EditorIcons::cancelIcon);
            LOAD_EPATH_IF_URL("drag", EditorIcons::dragIcon);
            
            return p;
        }
    };
    
    Paths f;
    HiseShapeButton startButton;
    HiseShapeButton cancelButton;
    
    struct Dragger: public Component,
    public juce::DragAndDropContainer
    {
        Dragger()
        {
            Paths f;
            p = f.createPath("drag");
            setMouseCursor(MouseCursor::PointingHandCursor);
            setRepaintsOnMouseActivity(true);
        }
        
        void mouseDown(const MouseEvent& e) override
        {
            if(fileToDrag.existsAsFile())
            {
                DragAndDropContainer::performExternalDragDropOfFiles ({fileToDrag.getFullPathName()}, false, this);
            }
        }
        
        void resized() override
        {
            Paths f;
            f.scalePath(p, getLocalBounds().removeFromLeft(getHeight()).reduced(10).toFloat());
        }
        
        void setFile(const File& f)
        {
            fileToDrag = f;
            repaint();
        }
        
        File fileToDrag;
        
        void paint(Graphics& g) override
        {
            auto ok = fileToDrag.existsAsFile();
            
            if(ok)
            {
                g.setColour(Colour(HISE_OK_COLOUR).withAlpha(0.4f));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(10.0f), 3.0f);
            }
            
            float alpha = ok ? 0.5f : 0.2f;
            if(ok && isMouseOver())
                alpha += 0.2f;
            if(ok && isMouseOverOrDragging() && isMouseButtonDown())
                alpha += 0.3f;
            
            
            
            g.setFont(GLOBAL_BOLD_FONT());
            g.setColour(Colours::white.withAlpha(alpha));
            g.drawText("Drag me down!", getLocalBounds().toFloat().reduced(20), Justification::right);
            g.fillPath(p);
        }
        Path p;
    } dragger;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


