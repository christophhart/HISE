#ifndef ROUNDROBIN_H_INCLUDED
#define ROUNDROBIN_H_INCLUDED

class ModulatorSynthGroup;

class RoundRobinMidiProcessor: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("RoundRobin", "Round Robin");

	RoundRobinMidiProcessor(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		counter(0)
	{};

	/** Reactivates all groups on shutdown. */
	~RoundRobinMidiProcessor();

	

	float getAttribute(int) const override { return 0.0f; };

	void setInternalAttribute(int, float ) override { };
	
	/** deactivates the child synths in a round robin cycle. */
	void processMidiMessage(MidiMessage &m) override;
	
private:

	int counter;
};




#endif