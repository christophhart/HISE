/*
  ==============================================================================

    KeyModulator.cpp
    Created: 1 Aug 2014 9:32:32pm
    Author:  Chrisboy

  ==============================================================================
*/

#include "KeyModulator.h"

KeyModulator::KeyModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
		VoiceStartModulator(mc, id, numVoices, m),
		Modulation(m),
		mode(KeyMode),
		keyTable(new DiscreteTable()),
		midiTable(new MidiTable())
{
	parameterNames.add("KeyMode");
	parameterNames.add("NumberMode");
};

ProcessorEditorBody *KeyModulator::createEditor(BetterProcessorEditor *parentEditor)
{ 
#if USE_BACKEND

	return new KeyEditor(parentEditor); 
	
#else

	jassertfalse;

	return nullptr;

#endif
};