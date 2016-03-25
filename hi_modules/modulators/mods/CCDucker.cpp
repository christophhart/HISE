
CCDucker::CCDucker(MainController *mc, const String &id, Modulation::Mode m):
TimeVariantModulator(mc, id, m),
Modulation(m),
smoothTime(getDefaultValue(SmoothingTime)),
attackTime(getDefaultValue(DuckingTime)),
ccNumber((int)getDefaultValue(CCNumber)),
currentValue(1.0f),
targetValue(0.0f),
currentUptime(0.0f),
uptimeDelta(0.0f),
table(new SampleLookupTable())
{
	smoother.setDefaultValue(1.0f);
}

void CCDucker::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);

	loadAttribute(SmoothingTime, "SmoothingTime");
	loadAttribute(DuckingTime, "DuckingTime");
	loadAttribute(CCNumber, "CCNumber");

	loadTable(table, "DuckingTable");
}

ValueTree CCDucker::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveAttribute(SmoothingTime, "SmoothingTime");
	saveAttribute(DuckingTime, "DuckingTime");
	saveAttribute(CCNumber, "CCNumber");

	saveTable(table, "DuckingTable");

	return v;
}

ProcessorEditorBody * CCDucker::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new CCDuckerEditor(parentEditor);
#else

	jassertfalse;
	return nullptr;
#endif

}

float CCDucker::getDefaultValue(int parameterIndex) const
{
	SpecialParameters p = (SpecialParameters)parameterIndex;

	switch (p)
	{
	case CCDucker::CCNumber: return 1.0f;
		break;
	case CCDucker::DuckingTime: return 100.0f;
		break;
	case CCDucker::SmoothingTime: return 0.0f;
		break;
	case CCDucker::numSpecialParameters: jassertfalse;
		break;
	default:
		break;
	}

	return 0.0f;
}

float CCDucker::getAttribute(int parameterIndex) const
{
	SpecialParameters p = (SpecialParameters)parameterIndex;

	switch (p)
	{
	case CCDucker::CCNumber: return (float)ccNumber;
	case CCDucker::DuckingTime: return attackTime;
	case CCDucker::SmoothingTime: return smoothTime;
	case CCDucker::numSpecialParameters: jassertfalse;
		break;
	default:
		break;
	}

	return 0.0f;
}

void CCDucker::setInternalAttribute(int parameterIndex, float newValue)
{
	SpecialParameters p = (SpecialParameters)parameterIndex;

	switch (p)
	{
	case CCDucker::CCNumber: ccNumber = (int)newValue;
		break;
	case CCDucker::DuckingTime: setDuckingTime(newValue);
		break;
	case CCDucker::SmoothingTime: smoothTime = newValue;
								  smoother.setSmoothingTime(newValue);
		break;
	case CCDucker::numSpecialParameters: jassertfalse;
		break;
	default:
		break;
	}
}

void CCDucker::handleMidiEvent(const MidiMessage &m)
{
	if (m.isControllerOfType(ccNumber))
	{
		currentUptime = 0.0f;
		targetValue = -1.0f;
	}
}

void CCDucker::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);

	smoother.prepareToPlay(sampleRate);
	smoother.setSmoothingTime(smoothTime);
	setDuckingTime(attackTime);
}

void CCDucker::calculateBlock(int startSample, int numSamples)
{
	const bool smoothThisBlock = fabsf(targetValue - currentValue) > 0.001f;

	if (currentUptime == -1.0f)
	{
		if (smoothThisBlock)
		{
			while (--numSamples >= 0)
			{
				targetValue = 1.0f;
				currentValue = smoother.smooth(targetValue);
				internalBuffer.setSample(0, startSample, currentValue);
				++startSample;
			}
		}
		else
		{
			FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 1.0f, numSamples);
			currentValue = 1.0f;
			setOutputValue(1.0f);
			sendTableIndexChangeMessage(false, table, 1.0f);
		}
		
		return;
	}
	while (--numSamples >= 0)
	{
		targetValue = getNextValue();
		currentValue = smoother.smooth(targetValue);
		internalBuffer.setSample(0, startSample, currentValue);
		++startSample;
	}
	
	sendTableIndexChangeMessage(false, table, currentUptime / (float)SAMPLE_LOOKUP_TABLE_SIZE);
	//setInputValue(inputValue);
	setOutputValue(currentValue);
}

void CCDucker::setDuckingTime(float newValue)
{
	attackTime = newValue;

	if (getSampleRate() != 0)
	{
		const float lengthInSamples = newValue / 1000.0f * (float)getSampleRate();

		uptimeDelta = (float)SAMPLE_LOOKUP_TABLE_SIZE / lengthInSamples;
	}
}

float CCDucker::getNextValue()
{
	if (currentUptime == -1.0f) { return 1.0f; }
	else if (currentUptime > SAMPLE_LOOKUP_TABLE_SIZE)
	{
		currentUptime = -1;
		return 1.0f;
	}
	else
	{
		const float value = table->getInterpolatedValue(currentUptime);
		currentUptime += uptimeDelta;
		return value;
	}
}

