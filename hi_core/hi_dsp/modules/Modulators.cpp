/*
  ==============================================================================

    Modulators.cpp

  ==============================================================================
*/

Modulator::Modulator(MainController *mc, const String &id_):
	Processor(mc, id_),
	attachedPlotter(nullptr),
	colour(Colour(0x00000000))
{		
};

void Modulator::setPlotter(Plotter *targetPlotter)
{
	attachedPlotter = targetPlotter;
	
};

bool Modulator::isPlotted() const 
{
	return attachedPlotter.getComponent() != nullptr; 
};

void Modulator::addValueToPlotter(float v) const
{
	if(attachedPlotter.getComponent() != nullptr) 
	{
		attachedPlotter.getComponent()->addValue(this, v);
	}
};

bool TimeModulation::isInitialized() { return getProcessor()->getSampleRate() != -1.0f; };

VoiceStartModulator::VoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
		VoiceModulation(numVoices, m),
		Modulator(mc, id),
		Modulation(m),
		unsavedValue(-1.0f)
{
	voiceValues.insertMultiple(0, -1.0f, numVoices);
};

EnvelopeModulator::EnvelopeModulator(MainController *mc, const String &id, int voiceAmount_, Modulation::Mode m):
	Modulator(mc, id),
	Modulation(m),
	TimeModulation(m),
	VoiceModulation(voiceAmount_, m)
{ 
	globalSaveValues = AudioSampleBuffer(voiceAmount_, 1);
};

void EnvelopeModulator::saveValuesForGlobalModulator(const AudioSampleBuffer &internalBuffer, int startSample, int numSamples, int voiceIndex)
{
	FloatVectorOperations::copy(globalSaveValues.getWritePointer(voiceIndex, startSample), internalBuffer.getReadPointer(0, startSample), numSamples);
}

Processor *VoiceStartModulatorFactoryType::createProcessor(int typeIndex, const String &id)
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
		case constantModulator: return new ConstantModulator(m, id, numVoices, mode);
		case velocityModulator: return new VelocityModulator(m, id, numVoices, mode);
		case keyModulator:		return new KeyModulator(m, id, numVoices, mode);
		case randomModulator: return new RandomModulator(m, id, numVoices, mode);
		case globalVoiceStartModulator:	return new GlobalVoiceStartModulator(m, id, numVoices, mode);
		case gainMatcherVoiceStartModulator:	return new GainMatcherVoiceStartModulator(m, id, numVoices, mode);
		case arrayModulator:	return new ArrayModulator(m, id, numVoices, mode);
		default: jassertfalse; return nullptr;
	}
};

Processor *TimeVariantModulatorFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case lfoModulator: return new LfoModulator(m, id, mode);
	case controlModulator: return new ControlModulator(m, id, mode);
	case pitchWheel:		return new PitchwheelModulator(m, id, mode);
	case macroModulator:	return new MacroModulator(m, id, mode);
	case audioFileEnvelope:	return new AudioFileEnvelope(m, id, mode);
	case pluginParameter: return new PluginParameterModulator(m, id, mode);
	case globalTimeVariantModulator:	return new GlobalTimeVariantModulator(m, id, mode);
	case gainMatcherTimeVariantModulator:	return new GainMatcherTimeVariantModulator(m, id, mode);
	case ccDucker:			return new CCDucker(m, id, mode);
	default: jassertfalse; return nullptr;

	}
};

Processor *EnvelopeModulatorFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case simpleEnvelope:	return new SimpleEnvelope(m, id, numVoices, mode);
	case ahdsrEnvelope:		return new AhdsrEnvelope(m, id, numVoices, mode);
	case tableEnvelope:		return new TableEnvelope(m, id, numVoices, mode);
	case globalEnvelope:	return new GlobalEnvelopeModulator(m, id, numVoices, mode);
	case ccEnvelope:		return new CCEnvelope(m, id, numVoices, mode);
	default: jassertfalse;	return nullptr;

	}
};