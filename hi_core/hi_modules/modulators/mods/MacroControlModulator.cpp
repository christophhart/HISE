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

hise::ProcessorMetadata MacroModulator::createMetadata()
{
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata(getClassType())
		.withPrettyName("Macro Modulator")
		.withDescription("A modulator controlled by a macro controller slot, allowing real-time automation and MIDI learn functionality.")
		.withType<hise::TimeVariantModulator>()
		.withComplexDataInterface(ExternalData::DataType::Table)
		.withParameter(ProcessorMetadata::ParameterMetadata(MacroIndex)
			.withId("MacroIndex")
			.withDescription("The macro controller slot index (0-7) to link to")
			.withSliderMode(HiSlider::Discrete, scriptnode::InvertableParameterRange(-1.0, 7.0, 1.0))
			.withDefault(-1.0f))
		.withParameter(ProcessorMetadata::ParameterMetadata(SmoothTime)
			.withId("SmoothTime")
			.withDescription("Smoothing time in milliseconds for value transitions")
			.withSliderMode(HiSlider::Time, Range(0.0, 1000.0, 0.0).withCentreSkew(100.0))
			.withDefault(200.0f))
		.withParameter(ProcessorMetadata::ParameterMetadata(UseTable)
			.withId("UseTable")
			.withDescription("Enables a lookup table for custom macro value response curves")
			.asToggle()
			.withDefault(0.0f))
		.withParameter(ProcessorMetadata::ParameterMetadata(MacroValue)
			.withId("MacroValue")
			.withDescription("The current macro value (automated by macro controller)")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(1.0f));
}

MacroModulator::MacroModulator(MainController *mc, const String &id, Modulation::Mode m):
	TimeVariantModulator(mc, id, m),
	LookupTableProcessor(mc, 1),
	Modulation(m),
	targetValue(1.0f),
	currentValue(1.0f),
	macroIndex(-1),
	smoothTime(200.0f),
	useTable(false)
{
	referenceShared(ExternalData::DataType::Table, 0);

	updateParameterSlots();
}

void MacroModulator::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");
	loadAttribute(MacroIndex, "MacroIndex");
	loadAttribute(SmoothTime, "SmoothTime");

	loadTable(getTableUnchecked(0), "MacroTableData");
}

ValueTree MacroModulator::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");
	saveAttribute(MacroIndex, "MacroIndex");
	saveAttribute(SmoothTime, "SmoothTime");

	saveTable(getTableUnchecked(0), "MacroTableData");

	return v;
}

#if USE_BACKEND
Path MacroModulator::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData(HiBinaryData::SpecialSymbols::macros, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::macros));

	return path;
}
#endif

void MacroModulator::addToMacroController(int index)
{
	if (macroIndex != index)
	{
		macroIndex = index;

		ModulatorSynthChain *macroChain = getMainController()->getMacroManager().getMacroChain();

		auto mc = getMainController();
		auto numMacros = HISE_GET_PREPROCESSOR(mc, HISE_NUM_MACROS);
        ignoreUnused(mc);

		for (int i = 0; i < numMacros; i++)
		{
			macroChain->getMacroControlData(i)->removeAllParametersWithProcessor(this);

		}

		macroChain->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);

		if (macroIndex != -1) macroChain->addControlledParameter(index, getId(), MacroValue, "Macro Modulator", getValueToTextConverter(), getRange());
	}
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
        targetValue = getTableUnchecked()->getInterpolatedValue(inputValue, sendNotificationAsync);
	}
	else
	{
		targetValue = newValue;
	}
}

void MacroModulator::handleHiseEvent(const HiseEvent &)
{
	
}

void MacroModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
	smoother.prepareToPlay(getControlRate());

	if (sampleRate != -1.0) setInternalAttribute(SmoothTime, smoothTime);
}

void MacroModulator::calculateBlock(int startSample, int numSamples)
{
    const bool smoothThisBlock = FloatSanitizers::isNotSilence(targetValue - currentValue);

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

}

ProcessorEditorBody *MacroModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MacroControlModulatorEditorBody(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};

} // namespace hise
