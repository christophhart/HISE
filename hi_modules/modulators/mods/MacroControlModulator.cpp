/*
  ==============================================================================

    MacroControlModulator.cpp
    Created: 23 Sep 2014 11:08:23pm
    Author:  Christoph

  ==============================================================================
*/

#include "MacroControlModulator.h"



MacroModulator::MacroModulator(MainController *mc, const String &id, Modulation::Mode m) :
TimeVariantModulator(mc, id, m),
Modulation(m),
macroIndex(-1),
smoothTime(200.0f),
useTable(false),
table(new MidiTable()),
learnMode(false),
currentValue(1.0f),
targetValue(1.0f)
{
	parameterNames.add("MacroIndex");
	parameterNames.add("SmoothTime");
	parameterNames.add("UseTable");
	parameterNames.add("MacroValue");

	setup(this, MacroValue, id);
}

void MacroModulator::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");
	loadAttribute(MacroIndex, "MacroIndex");
	loadAttribute(SmoothTime, "SmoothTime");

	if (useTable) loadTable(table, "MacroTableData");
}

ValueTree MacroModulator::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");
	saveAttribute(MacroIndex, "MacroIndex");
	saveAttribute(SmoothTime, "SmoothTime");

	if (useTable) saveTable(table, "MacroTableData");

	return v;
}

#if USE_BACKEND
Path MacroModulator::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData(HiBinaryData::SpecialSymbols::macros, sizeof(HiBinaryData::SpecialSymbols::macros));

	return path;
}
#endif

ProcessorEditorBody *MacroModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MacroControlModulatorEditorBody(parentEditor);

	
#else

	jassertfalse;

	return nullptr;

#endif
};

void MacroModulator::addToMacroController(int index)
{
	macroIndex = index;

	ModulatorSynthChain *macroChain = getMainController()->getMacroManager().getMacroChain();

	for(int i = 0; i < 8; i++)
	{
		macroChain->getMacroControlData(i)->removeAllParametersWithProcessor(this);

	}

	macroChain->sendChangeMessage();

	if(macroIndex != -1) macroChain->addControlledParameter(index, getId(), MacroValue, "Macro Modulator", getRange());
}


float MacroModulator::getAttribute (int parameter_index) const
{
	switch(parameter_index)
	{
	
	case MacroIndex: return (float)macroIndex;
	case SmoothTime: return smoothTime;
	case UseTable:   return useTable ? 1.0f : 0.0f;
	case MacroValue: jassertfalse; return 1.0f;

	default: 
		jassertfalse;
		return -1.0f;
	}

};

void MacroModulator::setInternalAttribute (int parameter_index, float newValue)
{
	switch(parameter_index)
	{
	case MacroValue:	macroControllerMoved(newValue); break;	
	case MacroIndex:
		{
			addToMacroController((int)newValue);
			break;
		}

	case SmoothTime:
		{
			smoothTime = newValue; 

			smoother.setSmoothingTime(smoothTime);		
			break;
		}

	case UseTable:
		useTable = (newValue != 0.0f); break;

	default: 
		jassertfalse;
	}
};


void MacroModulator::macroControllerMoved(float newValue)
{
	inputValue = jlimit<float>(0.0f, 1.0f, newValue);

	if(useTable)
	{
		targetValue = table->get((int)(inputValue * 127));
		
		sendTableIndexChangeMessage(false, table, inputValue);
	}
	else
	{
		targetValue = newValue;
	}
}

void MacroModulator::handleMidiEvent (const MidiMessage &)
{
	
}

void MacroModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
	smoother.prepareToPlay(getSampleRate());

	if (sampleRate != -1.0) setInternalAttribute(SmoothTime, smoothTime);
}

void MacroModulator::calculateBlock(int startSample, int numSamples)
{
	const bool smoothThisBlock = fabsf(targetValue - currentValue) > 0.001f;

	if (smoothThisBlock)
	{
		if (--numSamples >= 0)
		{
			currentValue = smoother.smooth(targetValue);

			internalBuffer.setSample(0, startSample, currentValue);
			++startSample;


		}

		while (--numSamples >= 0)
		{
			currentValue = smoother.smooth(targetValue);
			internalBuffer.setSample(0, startSample, currentValue);
			++startSample;
		}
	}
	else
	{
		currentValue = targetValue;

		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), currentValue, numSamples);
	}

	//setInputValue(inputValue);
	setOutputValue(currentValue);
}
