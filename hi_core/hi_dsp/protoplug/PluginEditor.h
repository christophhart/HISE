#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "guiclasses/ProtoWindow.h"

class LuaProtoplugJuceAudioProcessorEditor  :	public AudioProcessorEditor,
												public Button::Listener
{
public:
	LuaProtoplugJuceAudioProcessorEditor (LuaProtoplugJuceAudioProcessor* ownerFilter);
	~LuaProtoplugJuceAudioProcessorEditor();
	
	void paint (Graphics& g);
	void resized();
	void handleCommandMessage(int com);
	void buttonClicked(Button *);

	void popOut();
	void popIn();
    LuaProtoplugJuceAudioProcessor *processor;

private:

	ChainBarButtonLookAndFeel laf;

	ProtoWindow content; // the actual gui is in there
	ScopedPointer<ProtoPopout> poppedWin;
	TextButton yank;
	TextButton popin;
	TextButton locateFiles;
};

