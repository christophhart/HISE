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
class MainContentComponent   : public Component,
							   public hise::ComponentWithBackendConnection,
							   public hise::ModalBaseWindow
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&) override;
    void resized() override;

	
	MainController* getMainControllerToUse() override { return &bp; }
	const MainController* getMainControllerToUse() const override { return &bp; }

	BackendRootWindow* getBackendRootWindow() { return b; }

	const BackendRootWindow* getBackendRootWindow() const { return b; }

	FloatingTile* getRootFloatingTile() { return &ft; }

private:

	BackendProcessor bp;
	Component::SafePointer<BackendRootWindow> b;
	FloatingTile ft;

	MarkdownDataBase database;

	hise::GlobalHiseLookAndFeel laf;

	juce::TooltipWindow tooltip;

	Component::SafePointer<MarkdownPreview> preview;
	

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
