
RoundRobinMidiProcessor::~RoundRobinMidiProcessor()
{
	ModulatorSynthGroup *g = static_cast<ModulatorSynthGroup*>(getOwnerSynth());

	if(g != nullptr) g->setAllowStateForAllChildSynths(true);
}

void RoundRobinMidiProcessor::processMidiMessage(MidiMessage &m)
{
	ModulatorSynthGroup *group = static_cast<ModulatorSynthGroup*>(getOwnerSynth());

	if(group == nullptr)
	{
		debugMod("WARNING: NOT USED ON GROUP!");
		return;
	}

	const int roundRobinNumber = group->getHandler()->getNumProcessors();

	group->setAllowStateForAllChildSynths(false);

	if(m.isNoteOn() && roundRobinNumber != 0)
	{
		counter = (counter + 1) % roundRobinNumber;

		group->allowChildSynth(counter, true);
	}
}