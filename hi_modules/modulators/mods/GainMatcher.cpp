

GainMatcherModulator::GainMatcherModulator():
table(new SampleLookupTable())
{

}

StringArray GainMatcherModulator::getListOfAllGainCollectors()
{
	Modulator* thisAsMod = dynamic_cast<Modulator*>(this);

	return ProcessorHelpers::getAllIdsForType<GainCollector>(thisAsMod->getMainController()->getMainSynthChain());
}

String GainMatcherModulator::getConnectedCollectorId() const
{

	if (connectedCollector.get() != nullptr)
	{
		return connectedCollector.get()->getId();
	}
	else return String::empty;
}

void GainMatcherModulator::setConnectedCollectorId(const String &newId)
{
	Modulator* thisAsMod = dynamic_cast<Modulator*>(this);

	connectedCollector = ProcessorHelpers::getFirstProcessorWithName(thisAsMod->getMainController()->getMainSynthChain(), newId);
}


const GainCollector * GainMatcherModulator::getCollector() const
{
	return dynamic_cast<const GainCollector*>(connectedCollector.get());
}

GainMatcherVoiceStartModulator::GainMatcherVoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m) :
VoiceStartModulator(mc, id, numVoices, m),
Modulation(m),

useTable(false),
generator(Random(Time::currentTimeMillis()))
{
	this->enableConsoleOutput(false);

	parameterNames.add("UseTable");
};

void GainMatcherVoiceStartModulator::restoreFromValueTree(const ValueTree &v)
{
	VoiceStartModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");

	setConnectedCollectorId(v.getProperty("ConnectedId"));

	if (useTable) loadTable(table, "TableData");
}

ValueTree GainMatcherVoiceStartModulator::exportAsValueTree() const
{
	ValueTree v = VoiceStartModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");

	v.setProperty("ConnectedId", getConnectedCollectorId(), nullptr);

	if (useTable) saveTable(table, "TableData");

	return v;
}

ProcessorEditorBody *GainMatcherVoiceStartModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new GainMatcherEditor(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif
};

void GainMatcherVoiceStartModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue != 0.0f); break;
	default:				jassertfalse; break;
	}
}

float GainMatcherVoiceStartModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

float GainMatcherVoiceStartModulator::calculateVoiceStartValue(const MidiMessage &)
{

	if (getCollector() == nullptr) return 1.0f;

	currentValue =  getCollector()->getCurrentGain();

	if (useTable)
	{
		currentValue = table->getInterpolatedValue(currentValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

		
		sendTableIndexChangeMessage(false, table, currentValue);
	}
	
	return currentValue;
}


GainMatcherTimeVariantModulator::GainMatcherTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m) :
TimeVariantModulator(mc, id, m),
Modulation(m),
useTable(false),
currentValue(1.0f)
{
	this->enableConsoleOutput(false);

	parameterNames.add("UseTable");
};

GainMatcherTimeVariantModulator::~GainMatcherTimeVariantModulator()
{
};

void GainMatcherTimeVariantModulator::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");
	
	setConnectedCollectorId(v.getProperty("ConnectedId"));

	if (useTable) loadTable(table, "ControllerTableData");
}

ValueTree GainMatcherTimeVariantModulator::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");
	
	v.setProperty("ConnectedId", getConnectedCollectorId(), nullptr);

	if (useTable) saveTable(table, "ControllerTableData");

	return v;
}

ProcessorEditorBody *GainMatcherTimeVariantModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new GainMatcherEditor(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif
};

float GainMatcherTimeVariantModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
};

void GainMatcherTimeVariantModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue != 0.0f); break;
	default:				jassertfalse; break;
	}
};



void GainMatcherTimeVariantModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
	
}


void GainMatcherTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
	const float newValue = getCollector() != nullptr ? getCollector()->getCurrentGain() : 1.0f;

	r.setTarget(currentValue, newValue, numSamples);

	while (--numSamples >= 0)
	{	
		r.ramp(currentValue);

		currentValue = EnvelopeFollower::constrainTo0To1(currentValue);

		internalBuffer.setSample(0, startSample, currentValue);
		++startSample;
	}
	
	currentValue = newValue;

	if (useTable)sendTableIndexChangeMessage(false, table, currentValue);

	setOutputValue(currentValue);
}

float GainMatcherTimeVariantModulator::calculateNewValue()
{
	//currentValue = (fabsf(targetValue - currentValue) < 0.001) ? targetValue : smoother.smooth(targetValue);

	return currentValue;
}

/** sets the new target value if the controller number matches. */
void GainMatcherTimeVariantModulator::handleMidiEvent(const MidiMessage &)
{
	
}