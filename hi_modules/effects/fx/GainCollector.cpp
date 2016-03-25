
GainCollector::GainCollector(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id),
smoothingTime(200.0f),
mode(SimpleLP),
attack(50.0f),
release(50.0f),
currentGain(1.0f)
{
	parameterNames.add("Smoothing");
	parameterNames.add("Mode");
	parameterNames.add("Attack");
	parameterNames.add("Release");
}

float GainCollector::getAttribute(int parameterIndex) const
{
	Parameters s = (Parameters)parameterIndex;

	switch (s)
	{
	case GainCollector::Smoothing:
		return smoothingTime;
		break;
	case GainCollector::Mode:
		return (float)(int)mode;
		break;
	case GainCollector::Attack:
		return attack;
		break;
	case GainCollector::Release:
		return release;
		break;
	case GainCollector::numEffectParameters:
	default:
		return -1.0f;
		break;
	}
}

void GainCollector::setInternalAttribute(int parameterIndex, float value)
{
	Parameters s = (Parameters)parameterIndex;

	switch (s)
	{
	case GainCollector::Smoothing:
		smoothingTime = value;
		smoother.setSmoothingTime(smoothingTime);
		break;
	case GainCollector::Mode:
		mode = (EnvelopeFollowerMode)(int)value;
		break;
	case GainCollector::Attack:
		attack = value;
		break;
	case GainCollector::Release:
		release = value;
		break;
	case GainCollector::numEffectParameters:
	default:
		break;
	}

}


void GainCollector::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	loadAttribute(Smoothing, "Smoothing");
	loadAttribute(Parameters::Mode, "Mode");
	loadAttribute(Attack, "Attack");
	loadAttribute(Release, "Release");

}

ValueTree GainCollector::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(Smoothing, "Smoothing");
	saveAttribute(Parameters::Mode, "Mode");
	saveAttribute(Attack, "Attack");
	saveAttribute(Release, "Release");

	return v;
}

void GainCollector::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	smoother.prepareToPlay(sampleRate);
}

void GainCollector::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	const float *samples = buffer.getReadPointer(0, startSample);
	int i = 0;
	float maximumThisTime = 0.0f;

	while (--numSamples >= 0)
	{
		float value = EnvelopeFollower::prepareAudioInput(samples[i++], 1.0f);
		value = EnvelopeFollower::constrainTo0To1(value);
		if (value > maximumThisTime) maximumThisTime = value;

		currentGain = smoother.smooth(maximumThisTime);
	}
}

ProcessorEditorBody *GainCollector::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new GainCollectorEditor(parentEditor);
#else 
	jassertfalse;
	return nullptr;
#endif
}

