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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;

struct SliderWithLimit : public PropertyComponent
{
	SliderWithLimit(ValueTree& data, const Identifier& id, UndoManager* um) :
		PropertyComponent(id.toString())
	{
		addAndMakeVisible(c);

		using namespace PropertyIds;

		auto v = data.getProperty(id);

		auto min = jmin(v, data.getProperty(LowerLimit, 0.0));
		auto max = jmax(v, data.getProperty(UpperLimit, 1.0));
		auto stepSize = data.getProperty(StepSize, 0.01);

		c.setRange(min, max, stepSize);
		c.getValueObject().referTo(data.getPropertyAsValue(id, um, true));
	}

	void refresh() override {}

	struct SliderWithLimitSetter : public juce::Slider
	{
		SliderWithLimitSetter()
		{
			setSliderStyle(Slider::LinearBar);
			setLookAndFeel(&laf);
			setColour(Slider::ColourIds::thumbColourId, Colour(0xFF666666));
			setColour(Label::ColourIds::textColourId, Colours::white);
			setColour(TextEditor::ColourIds::textColourId, Colours::white);
		}

		String getTextFromValue(double v) override
		{
			return CppGen::Emitter::createPrettyNumber(v, false);
		}

		double getValueFromText(const String& text) override
		{
			auto v = text.getDoubleValue();
			int numDigits = 0;	double check = v;

			while (fmod(check, 1.0) != 0 && numDigits < 4)
			{
				numDigits++;
				check *= 10.0;
			}

			numDigits++;
			auto interval = std::pow(10.0, -1.0 * (double)numDigits);

			if (getMinimum() > v)
				setRange(v, getMaximum(), interval);
			if (getMaximum() < v)
				setRange(getMinimum(), v, interval);
			else
				setRange(getMinimum(), getMaximum(), interval);

			return v;
		}

		LookAndFeel_V3 laf;

	} c;
};

juce::PropertyComponent* PropertyHelpers::createPropertyComponent(ValueTree& d, const Identifier& id, UndoManager* um)
{
	using namespace PropertyIds;

	auto value = d.getPropertyAsValue(id, um, true);
	auto name = id.toString();

	if (id == Converter || id == OpType)
	{
		Array<Identifier> ids;

		if (id == Converter)
			ids = { ConverterIds::Identity, ConverterIds::Decibel2Gain, ConverterIds::Gain2Decibel,
								  ConverterIds::DryAmount, ConverterIds::WetAmount, ConverterIds::SubtractFromOne, };
		else
			ids = { OperatorIds::SetValue, OperatorIds::Multiply, OperatorIds::Add };

		StringArray sa;
		Array<var> values;

		for (auto id : ids)
		{
			sa.add(id.toString());
			values.add(id.toString());
		}

		return new juce::ChoicePropertyComponent(value, name, sa, values);
	}

	if (id == MinValue || id == MaxValue)
		return new SliderWithLimit(d, id, um);

	return new TextPropertyComponent(value, name, 256, false);
}

}

