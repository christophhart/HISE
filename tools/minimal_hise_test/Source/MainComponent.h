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
					    public Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override;;

private:
    //==============================================================================
    // Your private member variables go here...
    int counter = 0;
	OpenGLContext context;

    
    TextButton b;

    WebViewData::Ptr data;
    WebViewWrapper webview;
    
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
