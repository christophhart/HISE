/*
  ==============================================================================

    PitchWheelModulator.cpp
    Created: 3 May 2015 5:26:33pm
    Author:  Christoph

  ==============================================================================
*/

#include "PitchWheelModulator.h"


PitchwheelModulator::PitchwheelModulator(MainController *mc, const String &id, Modulation::Mode m):
	TimeVariantModulator(mc, id, m),
    targetValue(1.0f),
	Modulation(m),
	intensity(1.0f),
	inverted(false),
	useTable(false),
	table(new MidiTable()),
	smoothTime(200.0f),
	currentValue(1.0f)
{
	this->enableConsoleOutput(false);
	
	parameterNames.add("Inverted");
	parameterNames.add("UseTable");
	parameterNames.add("SmoothTime");

};

PitchwheelModulator::~PitchwheelModulator()
{
};

ProcessorEditorBody *PitchwheelModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new PitchWheelEditorBody(parentEditor);

	
#else

	jassertfalse;

	return nullptr;

#endif
};

float PitchwheelModulator::getAttribute (int parameter_index) const
{
	switch(parameter_index)
	{
	case Parameters::Inverted:
		return inverted ? 1.0f : 0.0f;
	case Parameters::SmoothTime:
		return smoothTime;
	case Parameters::UseTable:
		return useTable ? 1.0f : 0.0f;
	default: 
		jassertfalse;
		return -1.0f;
	}

};

void PitchwheelModulator::setInternalAttribute (int parameter_index, float newValue)
{
	switch(parameter_index)
	{
	case Parameters::Inverted:
		inverted = (newValue != 0.0f); break;
	case Parameters::SmoothTime:
		{
			smoothTime = newValue; 

			smoother.setSmoothingTime(smoothTime);		
			break;
		}
	case Parameters::UseTable:
		useTable = (newValue != 0.0f); break;


	default: 
		jassertfalse;
	}
};


float PitchwheelModulator::calculateNewValue ()
{
	currentValue = (fabsf(targetValue - currentValue) < 0.001) ? targetValue : smoother.smooth(targetValue);
	
	return currentValue;
}

	/** sets the new target value if the controller number matches. */
void PitchwheelModulator::handleMidiEvent (const MidiMessage &m)
{
	
	if(m.isPitchWheel())
	{
		inputValue = m.getPitchWheelValue() / 16383.0f;
		float value;

		if(useTable) value = table->get((int)(inputValue * 127.0f));
		else value = inputValue;

		if(inverted) value = 1.0f - value;

		targetValue = value;

		debugMod(" New Value: " + String(value));
	};
}