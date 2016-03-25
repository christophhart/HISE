
RandomModulator::RandomModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
		VoiceStartModulator(mc, id, numVoices, m),
		Modulation(m),
		table(new MidiTable()),
		useTable(false),
		generator(Random (Time::currentTimeMillis()))
{
	this->enableConsoleOutput(false);

	parameterNames.add("UseTable");
};

void RandomModulator::restoreFromValueTree(const ValueTree &v)
{
	VoiceStartModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");

	if (useTable) loadTable(table, "RandomTableData");
}

ValueTree RandomModulator::exportAsValueTree() const
{
	ValueTree v = VoiceStartModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");

	if (useTable) saveTable(table, "RandomTableData");

	return v;
}

ProcessorEditorBody *RandomModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new RandomEditorBody(parentEditor);

	
#else

	jassertfalse;

	return nullptr;

#endif
};

void RandomModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue != 0.0f); break;
	default:				jassertfalse; break;
	}
}

float RandomModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

float RandomModulator::calculateVoiceStartValue(const MidiMessage &)
{
	float randomValue;

	if (useTable)
	{
		const int index = generator.nextInt(Range<int>(0, 127));
		randomValue = table->get(index);

		sendTableIndexChangeMessage(false, table, (float)index / 127.0f);
	}
	else
	{
		randomValue = generator.nextFloat();
	}

	return randomValue;
}
