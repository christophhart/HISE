#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "LuaLink.h"


#define NPARAMS 127
class ProtoWindow;
class LuaProtoplugJuceAudioProcessor;
class ProtoPopout;

class LuaProtoplugJuceAudioProcessor  : public AudioProcessor,
										public SafeChangeBroadcaster
{
public:
    LuaProtoplugJuceAudioProcessor();
    ~LuaProtoplugJuceAudioProcessor();

	// overrides
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    AudioProcessorEditor* createEditor();
    float getParameter (int index);
    void setParameter (int index, float newValue);
    const String getParameterName (int index);
    const String getParameterText (int index);
    double getTailLengthSeconds() const;
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

	// some inlined overrides
	const String getInputChannelName (int channelIndex) const	{ return String (channelIndex + 1); }
	const String getOutputChannelName (int channelIndex) const	{ return String (channelIndex + 1); }
	float getParameterDefaultValue (int /*parameterIndex*/)	{ return 0.5; }
	bool isInputChannelStereoPair (int /*index*/) const		{ return true; }
	bool isOutputChannelStereoPair (int /*index*/) const	{ return true; }
	bool acceptsMidi() const								{ return true; }
	bool producesMidi() const								{ return true; }
	bool silenceInProducesSilenceOut() const				{ return false; }
	const String getName() const							{ return JucePlugin_Name; }
	int getNumParameters()									{ return NPARAMS; }
	bool hasEditor() const									{ return true; }
	// leave 1 as per JuceVSTWrapper constructor requirement:
	int getNumPrograms()									{ return 1; }
	int getCurrentProgram()									{ return 1; }
	void setCurrentProgram (int /*index*/)					{ }
	const String getProgramName (int /*index*/)				{ return String::empty; }
	void changeProgramName (int /*index*/, const String& /*newName*/)	{ }
	void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)	{};
	void releaseResources()	{ }

	// some added methods
	ProtoWindow *getProtoEditor();
	void setProtoEditor(ProtoWindow * _ed);
    bool parameterText2Double (int index, String text, double &d);
	
    int lastUIWidth, lastUIHeight, lastUISplit, lastUIPanel;
	int lastPopoutX, lastPopoutY;
	float lastUIFontSize;
	bool popout, alwaysontop, liveMode;
	ScopedPointer<LuaLink> luli;
	double params[NPARAMS];

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuaProtoplugJuceAudioProcessor)
	ProtoWindow *lastOpenedEditor;
	char *chunk;
};
