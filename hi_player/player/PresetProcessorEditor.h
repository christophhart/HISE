/*
  ==============================================================================

    PluginPlayerProcessorEditor.h
    Created: 24 Oct 2014 2:35:34pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef PLUGINPLAYERPROCESSOREDITOR_H_INCLUDED
#define PLUGINPLAYERPROCESSOREDITOR_H_INCLUDED

class ScriptContentContainer;

class PresetProcessorEditor: public AudioProcessorEditor,
							 public VerticalListComponent::ChildComponent
{
public:

	PresetProcessorEditor(PresetProcessor *pp_);

	~PresetProcessorEditor();

	void resized();

	int getComponentHeight() const override;

	void paint(Graphics &g);

private:

	PresetProcessor *pp;

	ScopedPointer<PresetHeader> header;

	ScopedPointer<ScriptContentContainer> interfaceComponent;

};



#endif  // PLUGINPLAYERPROCESSOREDITOR_H_INCLUDED
