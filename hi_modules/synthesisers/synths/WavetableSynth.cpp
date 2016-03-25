#include "ClarinetData.cpp"

ProcessorEditorBody* WavetableSynth::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new WavetableBody(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}


WavetableSynthVoice::WavetableSynthVoice(ModulatorSynth *ownerSynth):
	ModulatorSynthVoice(ownerSynth),
	wavetableSynth(dynamic_cast<WavetableSynth*>(ownerSynth)),
	octaveTransposeFactor(1),
	currentSound(nullptr),
	hqMode(true)
{
		
};

float WavetableSynthVoice::getGainValue(float modValue)
{
	return wavetableSynth->getGainValueFromTable(modValue);
}

const float *WavetableSynthVoice::getTableModulationValues(int startSample, int numSamples)
{
	

	dynamic_cast<WavetableSynth*>(getOwnerSynth())->calculateTableModulationValuesForVoice(voiceIndex, startSample, numSamples);



	return dynamic_cast<WavetableSynth*>(getOwnerSynth())->getTableModValues(voiceIndex);
}

void WavetableSynthVoice::stopNote(float velocity, bool allowTailoff)
{

	ModulatorSynthVoice::stopNote(velocity, allowTailoff);

	ModulatorChain *c = static_cast<ModulatorChain*>(getOwnerSynth()->getChildProcessor(WavetableSynth::TableIndexModulation));

	c->stopVoice(voiceIndex);
}

int WavetableSynthVoice::getSmoothSize() const
{
	return wavetableSynth->getMorphSmoothing();
}