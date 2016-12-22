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
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
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

BackendProcessor::BackendProcessor(AudioDeviceManager *deviceManager_/*=nullptr*/, AudioProcessorPlayer *callback_/*=nullptr*/) :
MainController(),
AudioProcessorDriver(deviceManager_, callback_),
viewUndoManager(new UndoManager())
{
    synthChain = new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES, viewUndoManager);

    
	synthChain->addProcessorsWhenEmpty();

	getSampleManager().getModulatorSamplerSoundPool()->setDebugProcessor(synthChain);

	getMacroManager().setMacroChain(synthChain);

	handleEditorData(false);

	createUserPresetData();
}


BackendProcessor::~BackendProcessor()
{
	clearPreset();

	synthChain = nullptr;

	jassert(editorInformation.getType() == Identifier("editorData"));

	handleEditorData(true);
}

void BackendProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	processBlockCommon(buffer, midiMessages);
};

void BackendProcessor::handleControllersForMacroKnobs(const MidiBuffer &/*midiMessages*/)
{
	
}


void BackendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails(newSampleRate, samplesPerBlock);
    
	MainController::prepareToPlay(newSampleRate, samplesPerBlock);
};

AudioProcessorEditor* BackendProcessor::createEditor()
{
	return new BackendProcessorEditor(this, editorInformation);
}

void BackendProcessor::setEditorState(ValueTree &editorState)
{
	editorInformation = ValueTree(editorState);
}

