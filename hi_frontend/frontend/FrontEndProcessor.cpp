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
#if USE_COPY_PROTECTION
	if ((unlockCounter & 1023 == 0) && !unlocker.isUnlocked()) return;
#endif

    AudioPlayHead::CurrentPositionInfo newTime;

	getMacroManager().getMidiControlAutomationHandler()->handleParameterData(midiMessages);

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

FrontendProcessor::FrontendProcessor(ValueTree &synthData, ValueTree *imageData_/*=nullptr*/, ValueTree *impulseData/*=nullptr*/, ValueTree *externalScriptData/*=nullptr*/, ValueTree *userPresets) :
MainController(),
synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES)),
samplesCorrectlyLoaded(true),
keyFileCorrectlyLoaded(true),
presets(*userPresets),
currentlyLoadedProgram(0),
unlockCounter(0)
{
#if USE_COPY_PROTECTION

	if (PresetHandler::loadKeyFile(unlocker))
	{
		DBG("TUT");
	}
	else
	{
		DBG("FAIL");

		keyFileCorrectlyLoaded = false;
	}

#endif

	loadImages(imageData_);

	if (externalScriptData != nullptr)
	{
		setExternalScriptData(*externalScriptData);
	}

	if (impulseData != nullptr)
	{
		getSampleManager().getAudioSampleBufferPool()->restoreFromValueTree(*impulseData);
	}

	numParameters = 0;

	getMacroManager().setMacroChain(synthChain);

	synthChain->setId(synthData.getProperty("ID", String::empty));

	suspendProcessing(true);

	synthChain->restoreFromValueTree(synthData);

	synthChain->compileAllScripts();

	synthChain->loadMacrosFromValueTree(synthData);

	for (int i = 0; i < 8; i++)
	{
		if (synthChain->getMacroControlData(i)->getNumParameters() != 0)
		{
			numParameters++;
		}
	}

	CHECK_COPY_AND_RETURN_6(synthChain);

	if (getSampleRate() > 0)
	{
		synthChain->prepareToPlay(getSampleRate(), getBlockSize());
	}

	suspendProcessing(false);
}

void FrontendProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	suspendProcessing(true);

	CHECK_COPY_AND_RETURN_1(synthChain);
		
	MainController::prepareToPlay(sampleRate, samplesPerBlock);
	synthChain->prepareToPlay(sampleRate, samplesPerBlock);

	suspendProcessing(false);
};

AudioProcessorEditor* FrontendProcessor::createEditor()
{
	

	return new FrontendProcessorEditor(this);
}

void FrontendProcessor::setCurrentProgram(int index)
{
	if (index == currentlyLoadedProgram) return;

	currentlyLoadedProgram = index;

	if (index == 0) return;

	if (presets.getNumChildren() != 0)
	{
		const ValueTree child = presets.getChild(index-1);

		const String name = child.getProperty("FileName");

		Processor::Iterator<ScriptProcessor> iter(synthChain);

		while (ScriptProcessor *sp = iter.getNextProcessor())
		{
			if (sp->isFront())
			{
				sp->getScriptingContent()->restoreAllControlsFromPreset(name);
			}
		}
	}
}

