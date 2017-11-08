/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

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
	else return String();
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

ProcessorEditorBody *GainMatcherVoiceStartModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new GainMatcherEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
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

float GainMatcherVoiceStartModulator::calculateVoiceStartValue(const HiseEvent &)
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

ProcessorEditorBody *GainMatcherTimeVariantModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new GainMatcherEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
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
void GainMatcherTimeVariantModulator::handleHiseEvent(const HiseEvent &)
{
	
}

} // namespace hise
