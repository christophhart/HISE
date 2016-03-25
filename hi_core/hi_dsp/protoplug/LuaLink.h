#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "LuaState.h"

class LuaProtoplugJuceAudioProcessor;
class ProtoWindow;

class LuaLink
{
public:
	LuaLink(LuaProtoplugJuceAudioProcessor *pfx);
	~LuaLink();
	
	void addToLog(String buf, bool isInput = false);
	void compile();
	void stackDump();
	bool runString(String toRun);
	void runStringInteractive(String toRun);

	// Audio plugin overrides :
	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages, AudioPlayHead* ph);
	String getParameterName (int index);
	String getParameterText (int index);
	bool parameterText2Double (int index, String text, double &d);
	void paramChanged(int index);
	double getTailLengthSeconds();
	String save();
	void load(String loadData);
	void preClose();
	void initProtoplugDir();
	
	// Custom GUI component overrides :
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
	void focusGained (Component::FocusChangeType cause);
	void focusLost (Component::FocusChangeType cause);

	String code, libFolder, log, saveData;
	ScopedPointer<protolua::LuaState> ls;
	Component *customGui;
	LuaProtoplugJuceAudioProcessor *pfx;

private:
	bool startOverride(const char *fname);
	int startVarargOverride(const char *fname, va_list _args);
	int safepcall(const char *fname, int nargs, int nresults, int errfunc);

	/** Calls a lua function if it has been defined, ignoring the return value

		add any parameters to be sent to the function, preceded by their type (as 
		defined in luastate.h) Last argument must be 0.
	*/
	bool callVoidOverride(const char *fname, ...);
	String callStringOverride(const char *fname, ...);
	bool callBoolOverride(const char *fname, ...);
	bool safetobool();
	String safetostring();
    void mouseOverride (const char *fname, const MouseEvent& event);

	ProtoWindow *ped;
	CriticalSection cs;
	bool guiThreadRunning,  workable, iLuaLoaded;
};
