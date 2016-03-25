
PresetProcessor::PresetProcessor(const File &presetData) :
MainController(),
synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES))
{
	getMacroManager().setMacroChain(synthChain);

	loadPreset(presetData);
}



PresetProcessor::~PresetProcessor()
{
	synthChain = nullptr;
}

void PresetProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
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

	//handleControllersForMacroKnobs(midiMessages);

	

	

	synthChain->renderNextBlockWithModulators(buffer, midiMessages);

	uptime += double(buffer.getNumSamples()) / getSampleRate();
};

void PresetProcessor::processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
	midiMessages.clear();
	allNotesOff();
}

void PresetProcessor::handleControllersForMacroKnobs(const MidiBuffer &midiMessages)
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

void PresetProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MainController::prepareToPlay(sampleRate, samplesPerBlock);

	synthChain->prepareToPlay(sampleRate, samplesPerBlock);
	
};

ValueTree PresetProcessor::exportAsValueTree() const
{
	ValueTree v("ControlData");


	synthChain->saveMacrosToValueTree(v);

	synthChain->saveInterfaceValues(v);

	
	const bool folded = synthChain->getEditorState(Processor::Folded); 
	v.setProperty("Folded", folded, nullptr);
	
	const float gain = synthChain->getAttribute(ModulatorSynth::Parameters::Gain); 
	v.setProperty("Gain", gain, nullptr);

	return v;
}

void PresetProcessor::restoreFromValueTree(const ValueTree &v)
{
	const bool folded = v.getProperty("Folded", false);
	synthChain->setEditorState(Processor::Folded, folded);
	
	const float gain = v.getProperty("Gain", 1.0f);
	synthChain->setAttribute(ModulatorSynth::Parameters::Gain, gain, dontSendNotification);

	synthChain->loadMacrosFromValueTree(v);

	synthChain->restoreInterfaceValues(v);
}


AudioProcessorEditor* PresetProcessor::createEditor()
{
	return new PresetProcessorEditor(this);
}

