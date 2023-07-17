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

#pragma once

namespace hise {
using namespace juce;

namespace raw {


/** A connection to processor attributes as plugin parameter. */
template <class FunctionType> class PluginParameter : public AudioProcessorParameterWithID,
													  public Data<float>,
													  public ControlledObject
{
public:

	// ================================================================================================================

	

	PluginParameter(MainController* mc, const String& name) :
		AudioProcessorParameterWithID(name, name),
		ControlledObject(mc),
		Data(name)
	{}

	void setup(int t, const String& processorId, NormalisableRange<float> range, float midPoint)
	{
		type = t;
		p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), processorId);
		parameterRange = range;
		parameterRange.setSkewForCentre(midPoint);
	}

	// ================================================================================================================

	float getValue() const override
	{
		return FunctionType::save(p.get());
	}

	void setValue(float newValue) override
	{
		bool *enableUpdate = &getMainController()->getPluginParameterUpdateState();

		if (enableUpdate)
		{
			ScopedValueSetter<bool> setter(*enableUpdate, false, true);

			const float convertedValue = parameterRange.convertFrom0to1(newValue);
			const float snappedValue = parameterRange.snapToLegalValue(convertedValue);

			if (!lastValueInitialised || lastValue != snappedValue)
			{
				lastValue = snappedValue;
				lastValueInitialised = true;

				FunctionType::load(p.get(), snappedValue);
			}
		}
	}

	float getDefaultValue() const override
	{
		return 0.0f;
	}

	String getLabel() const override
	{
		if (type == IDs::UIWidgets::Slider)
		{
			return suffix;
		}
		else return String();
	}

	String getText(float value, int) const override
	{
		switch (type)
		{
		case IDs::UIWidgets::Slider:

			return String(parameterRange.convertFrom0to1(jlimit(0.0f, 1.0f, value)), 1);
			break;
		case IDs::UIWidgets::Button:
			return value > 0.5f ? "On" : "Off";
			break;
		case IDs::UIWidgets::ComboBox:
		{
			const int index = jlimit<int>(0, itemList.size() - 1, (int)(value*(float)itemList.size()));
			return itemList[index];
			break;
		}
		default:
			jassertfalse;
			break;
		}

		return String();
	}


	float getValueForText(const String &text) const override
	{
		switch (type)
		{
		case IDs::UIWidgets::Slider:
			return text.getFloatValue();
			break;
		case IDs::UIWidgets::Button:
			return text == "On" ? 1.0f : 0.0f;
			break;
		case IDs::UIWidgets::ComboBox:
			return (float)itemList.indexOf(text);
			break;
		default:
			break;
		}

		return 0.0f;
	}


	int getNumSteps() const override
	{
		switch (type)
		{
		case IDs::UIWidgets::Slider:
			return (int)((float)parameterRange.getRange().getLength() / parameterRange.interval);
		case IDs::UIWidgets::Button:
			return 2;
		case IDs::UIWidgets::ComboBox:
			return itemList.size();
		default:
			break;
		}

		jassertfalse;
		return 2;
	}


	void setParameterNotifyingHost(int index, float newValue)
	{
		ScopedValueSetter<bool> setter(getMainController()->getPluginParameterUpdateState(), false);

		auto sanitizedValue = jlimit<float>(parameterRange.start, parameterRange.end, newValue);
		auto parentProcessor = dynamic_cast<AudioProcessor*>(getMainController());

		parentProcessor->setParameterNotifyingHost(index, parameterRange.convertTo0to1(sanitizedValue));
	}

	bool isAutomatable() const override { return true; };

	bool isMetaParameter() const override
	{
		return false;
	}

private:

	// ================================================================================================================

	int type;

	WeakReference<Processor> p;
	NormalisableRange<float> parameterRange;

	String suffix;
	StringArray itemList;
	bool isMeta = false;

	float lastValue = -1.0f;
	bool lastValueInitialised = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginParameter);

	// ================================================================================================================
};

}
}