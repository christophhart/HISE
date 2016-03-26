/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


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


