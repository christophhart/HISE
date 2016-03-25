#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../LuaLink.h"

class CustomGuiPanel	:	public Component, public KeyListener
{
public:
	CustomGuiPanel (LuaLink *_luli)
	{
		luli = _luli;
		luli->customGui = this;
	}
	void paint (Graphics& g);
	void resized ();

    void mouseMove (const MouseEvent& event);
    void mouseEnter (const MouseEvent& event);
    void mouseExit (const MouseEvent& event);
    void mouseDown (const MouseEvent& event);
    void mouseDrag (const MouseEvent& event);
    void mouseUp (const MouseEvent& event);
    void mouseDoubleClick (const MouseEvent& event);
    void mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel);

	bool keyPressed (const KeyPress &key, Component *originatingComponent);
	bool keyStateChanged (bool isKeyDown, Component *originatingComponent);
	void modifierKeysChanged (const ModifierKeys &modifiers);
	void focusGained (FocusChangeType cause);
	void focusLost (FocusChangeType cause);

private:
	LuaLink *luli;
};
