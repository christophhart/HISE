/*
  ==============================================================================

    FrontEndProcessor.cpp
    Created: 16 Oct 2014 9:33:01pm
    Author:  Chrisboy

  ==============================================================================
*/

#include "FrontEndProcessor.h"


void FrontendProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    AudioPlayHead::CurrentPositionInfo newTime;

    if (getPlayHead() != nullptr && getPlayHead()->getCurrentPosition (newTime))
    {
        lastPosInfo = newTime;
    }
    else
    {
        lastPosInfo.resetToDefault();
    };

	setBpm(lastPosInfo.bpm);

	if(isSuspended())
	{
		buffer.clear();
	}

	checkAllNotesOff(midiMessages);

#if USE_MIDI_CONTROLLERS_FOR_MACROS

	handleControllersForMacroKnobs(midiMessages);

#endif

	startCpuBenchmark(buffer.getNumSamples());

	keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

	buffer.clear();

	synthChain->renderNextBlockWithModulators(buffer, midiMessages);

	midiMessages.clear();

	uptime += double(buffer.getNumSamples()) / getSampleRate();

	stopCpuBenchmark();

	
};

void FrontendProcessor::handleControllersForMacroKnobs(const MidiBuffer &midiMessages)
{
	if(!getMacroManager().macroControlMidiLearnModeActive() && !getMacroManager().midiMacroControlActive()) return;

	MidiBuffer::Iterator it(midiMessages);

	int samplePos;
	MidiMessage message;

	while(it.getNextEvent(message, samplePos))
	{
		if(message.isController())
		{
			const int controllerNumber = message.getControllerNumber();

			if(getMacroManager().macroControlMidiLearnModeActive())
			{
				getMacroManager().setMidiControllerForMacro(controllerNumber);
			}

			const int macroNumber = getMacroManager().getMacroControlForMidiController(controllerNumber);

			if(macroNumber != -1)
			{
				getMacroManager().getMacroChain()->setMacroControl(macroNumber, (float)message.getControllerValue(), sendNotification);
			}
		}
	}

}

void FrontendProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	checkKey();
	
		
	MainController::prepareToPlay(sampleRate, samplesPerBlock);

	synthChain->prepareToPlay(sampleRate, samplesPerBlock);
	
	
};

AudioProcessorEditor* FrontendProcessor::createEditor()
{
	return new FrontendProcessorEditor(this);
}


