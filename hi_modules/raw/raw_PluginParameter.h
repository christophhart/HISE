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
class PluginParameter : public AudioProcessorParameterWithID,
						public ControlledObject
{
public:

	struct ProcessorConnection
	{
		ProcessorConnection() {};

		ProcessorConnection(MainController* mc, const String& id, int index_, NormalisableRange<float> range_) :
			processor(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), id)),
			index(index_),
			range(range_)
		{}

		WeakReference<Processor> processor;
		int index = -1;
		NormalisableRange<float> range;
	};

	// ================================================================================================================

	enum class Type
	{
		Slider = 0,
		Button,
		ComboBox,
		Unsupported
	};

	PluginParameter(MainController* mc, Type t, const String& name, const Array<ProcessorConnection>& connections_) :
		AudioProcessorParameterWithID(name, name),
		ControlledObject(mc),
		type(t),
		connections(connections_)
	{

	}

	// ================================================================================================================

	float getValue() const override;
	void setValue(float newValue) override;
	float getDefaultValue() const override;

	String getLabel() const override;
	String getText(float value, int) const override;

	float getValueForText(const String &text) const override;
	int getNumSteps() const override;

	void setParameterNotifyingHost(int index, float newValue);
	bool isAutomatable() const override { return true; };

	bool isMetaParameter() const override;

private:

	NormalisableRange<float> getRangeToUse() const
	{
		return connections.size() == 1 ? connections.getFirst().range : parameterRange;
	}

	// ================================================================================================================

	Type type;

	NormalisableRange<float> parameterRange;

	Array<ProcessorConnection> connections;

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