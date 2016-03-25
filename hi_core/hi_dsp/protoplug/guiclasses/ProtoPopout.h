#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../PluginEditor.h"

class ProtoPopout  :	public DocumentWindow
{
public:
	ProtoPopout (	LuaProtoplugJuceAudioProcessorEditor *_vstPanel,
					const String& name,
                    Colour backgroundColour,
                    int requiredButtons,
                    bool addToDesktop = true)
		:DocumentWindow(name, backgroundColour, requiredButtons, addToDesktop)
	{ vstPanel = _vstPanel; }

	void closeButtonPressed()
	{
		vstPanel->postCommandMessage(MSG_POPOUT);
		// todo, check if everything is crashed and force close
	}
private:
	LuaProtoplugJuceAudioProcessorEditor *vstPanel;
};
