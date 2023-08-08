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

RandomModulator::RandomModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
		VoiceStartModulator(mc, id, numVoices, m),
		Modulation(m),
		LookupTableProcessor(mc, 1),
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

	loadTable(getTableUnchecked(0), "RandomTableData");
}

ValueTree RandomModulator::exportAsValueTree() const
{
	ValueTree v = VoiceStartModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");

	saveTable(getTableUnchecked(0), "RandomTableData");

	return v;
}

ProcessorEditorBody *RandomModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new RandomEditorBody(parentEditor);
	
#else

	ignoreUnused(parentEditor);
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

float RandomModulator::calculateVoiceStartValue(const HiseEvent &)
{
    auto randomValue = generator.nextFloat();

	if (useTable)
	{
		randomValue = getTableUnchecked()->getInterpolatedValue(randomValue, sendNotificationAsync);
	}
	
	return randomValue;
}

} // namespace hise
