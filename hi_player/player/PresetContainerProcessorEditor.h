/*
  ==============================================================================

    PresetContainerProcessorEditor.h
    Created: 30 Jul 2015 1:18:21pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PRESETCONTAINERPROCESSOREDITOR_H_INCLUDED
#define PRESETCONTAINERPROCESSOREDITOR_H_INCLUDED




class PresetContainerProcessorEditor : public AudioProcessorEditor,
	public FileDragAndDropTarget,
	public Button::Listener
{
public:

	PresetContainerProcessorEditor(PresetContainerProcessor *ppc_);;

	void buttonClicked(Button *b) override;

	virtual bool 	isInterestedInFileDrag(const StringArray &files) override;

	void filesDropped(const StringArray &files, int, int);
	void refreshPresetList(bool forceRefresh=false);

	void paint(Graphics &g);

	void resized();
	void removePresetProcessor(PresetProcessor * pp, VerticalListComponent::ChildComponent *presetEditor);
	void toggleBrowserWindow();

	void mouseDown(const MouseEvent &e) override;

	void addNewPreset(const File &f);

private:

	PresetContainerProcessor *ppc;

	ScopedPointer<CustomKeyboard> keyboard;

	ScopedPointer<Viewport> viewport;

	ScopedPointer<TooltipBar> tooltipBar;

	LookAndFeel_V3 laf;

	ScopedPointer<VerticalListComponent> verticalList;

	ScopedPointer<VerticalListComponent> browserList;

	ScopedPointer<ResizableBorderComponent> borderDragger;

	ScopedPointer<AnimatedPanelViewport> presetBrowser;

	ScopedPointer<ComponentBoundsConstrainer> constrainer;

	ScopedPointer<ShapeButton> browserButton;

#if JUCE_DEBUG && INCLUDE_COMPONENT_DEBUGGER

	ScopedPointer<jcf::ComponentDebugger> debugger;

#endif

};



#endif  // PRESETCONTAINERPROCESSOREDITOR_H_INCLUDED
