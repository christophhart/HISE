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

ControlModulator::ControlModulator(MainController *mc, const String &id, Modulation::Mode m):
	TimeVariantModulator(mc, id, m),
	LookupTableProcessor(mc, 1),
    targetValue(1.0f),
	Modulation(m),
	intensity(1.0f),
	inverted(false),
	useTable(false),
	smoothTime(200.0f),
	currentValue(1.0f),
	learnMode(false),
	controllerNumber(1),
	defaultValue(0.0f)
{
    referenceShared(ExternalData::DataType::Table, 0);

	for (int i = 0; i < 128; i++)
	{
		polyValues[i] = -1.0;
	}

	parameterNames.add("Inverted");
	parameterNames.add("UseTable");
	parameterNames.add("ControllerNumber");
	parameterNames.add("SmoothTime");
	parameterNames.add("DefaultValue");

	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
};

ControlModulator::~ControlModulator()
{
	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
};

void ControlModulator::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");
	loadAttribute(Inverted, "Inverted");
	loadAttribute(ControllerNumber, "ControllerNumber");
	loadAttribute(SmoothTime, "SmoothTime");
	loadAttribute(DefaultValue, "DefaultValue");

	if (useTable) loadTable(table, "ControllerTableData");
}

ValueTree ControlModulator::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");
	saveAttribute(Inverted, "Inverted");
	saveAttribute(ControllerNumber, "ControllerNumber");
	saveAttribute(SmoothTime, "SmoothTime");
	saveAttribute(DefaultValue, "DefaultValue");

	if (useTable) saveTable(table, "ControllerTableData");

	return v;
}

ProcessorEditorBody *ControlModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ControlEditorBody(parentEditor);
	
#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};

float ControlModulator::getAttribute (int parameter_index) const
{
	switch(parameter_index)
	{
	case Parameters::Inverted:
		return inverted ? 1.0f : 0.0f;

	case Parameters::ControllerNumber:
		return (float)controllerNumber;

	case Parameters::SmoothTime:
		return smoothTime;

	case Parameters::UseTable:
		return useTable ? 1.0f : 0.0f;

	case Parameters::DefaultValue:
		return defaultValue;

	default: 
		jassertfalse;
		return -1.0f;
	}

};

void ControlModulator::setInternalAttribute (int parameter_index, float newValue)
{
	switch(parameter_index)
	{
	case Parameters::Inverted:
		inverted = (newValue != 0.0f); break;

	case Parameters::ControllerNumber:
		controllerNumber = int(newValue); break;

	case Parameters::SmoothTime:
		{
			smoothTime = newValue; 

			smoother.setSmoothingTime(smoothTime);		
			break;
		}

	case Parameters::UseTable:
		useTable = (newValue != 0.0f); break;

	case Parameters::DefaultValue:
		{
		defaultValue = newValue;

		handleHiseEvent(HiseEvent(HiseEvent::Type::Controller, (uint8)controllerNumber, (uint8)defaultValue, 1));

		break;

		}

	default: 
		jassertfalse;
	}
};



void ControlModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
	smoother.prepareToPlay(getControlRate());

	if (sampleRate != -1.0) setInternalAttribute(SmoothTime, smoothTime);
}


void ControlModulator::calculateBlock(int startSample, int numSamples)
{
	const bool smoothThisBlock = fabsf(targetValue - currentValue) > 0.001f;

	if (smoothThisBlock)
	{
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

	if (useTable && lastInputValue != inputValue)
    {
        lastInputValue = inputValue;
    }
}

float ControlModulator::calculateNewValue()
{
	currentValue = (fabsf(targetValue - currentValue) < 0.001) ? targetValue : smoother.smooth(targetValue);
	
	

	return currentValue;
}

	/** sets the new target value if the controller number matches. */
void ControlModulator::handleHiseEvent(const HiseEvent &m)
{
	if (mpeEnabled && m.getChannel() != 1)
		return;

	if (m.isNoteOff())
	{
		polyValues[m.getNoteNumber()] = -1.0f;
		return;
	}

	if(learnMode)
	{
		if (m.isController())
		{
			controllerNumber = m.getControllerNumber();
			disableLearnMode();
		}
		else if (m.isChannelPressure() || m.isAftertouch())
		{
			controllerNumber = 128;
			disableLearnMode();
		}
		else if (m.isPitchWheel())
		{
			controllerNumber = 129;
			disableLearnMode();
		}
	}

	const bool isAftertouch = controllerNumber == 128 && (m.isAftertouch() || m.isChannelPressure());

	const bool isPitchWheel = controllerNumber == 129 && m.isPitchWheel();

	if(isAftertouch || m.isControllerOfType(controllerNumber) || isPitchWheel)
	{
		if (m.isController())
		{
			inputValue = (float)m.getControllerValue() / 127.0f;
		}
		else if (controllerNumber == 129 && m.isPitchWheel())
		{
			inputValue = (float)m.getPitchWheelValue() / 16383.0f;
		}
		else if (m.isChannelPressure())
		{
			inputValue = (float)m.getChannelPressureValue() / 127.0f;
		}
		else if (m.isAftertouch())
		{
			const int noteNumber = m.getNoteNumber();

			polyValues[noteNumber] = (float)m.getAfterTouchValue() / 127.0f;

			inputValue = FloatVectorOperations::findMaximum(polyValues, 128);

			jassert(inputValue != -1.0f);

			if (inputValue < 0.0f) inputValue = 0.0f;

		}
		else
		{
			jassertfalse;
		}

		inputValue = CONSTRAIN_TO_0_1(inputValue);
		
		float value;

		if(useTable)
            value = table->getInterpolatedValue((double)inputValue, sendNotificationAsync);
		else
            value = inputValue;

		if(inverted) value = 1.0f - value;

		targetValue = value;
	}
}
} // namespace hise
