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


struct ColourSelectorPropertyComponent : public PropertyComponent
{
	Value value;

	ColourSelectorPropertyComponent(ValueTree d, const Identifier& id, UndoManager* um):
		PropertyComponent(id.toString()),
		value(d.getPropertyAsValue(id, um))
	{
		addAndMakeVisible(comp);
		refresh();
	}

	void refresh() override
	{
		Colour c;

		auto v = value.getValue();

		if (v.isString())
		{
			c = Colour((uint32)v.toString().getLargeIntValue());
		}
		else if (v.isInt() || v.isInt64())
		{
			c = Colour((uint32)(int64)v);
		}

		comp.setDisplayedColour(c);

		repaint();
	}

	struct ColourComp : public Component,
		public Label::Listener,
		public ChangeListener
	{
		struct Popup : public Component
		{
			Popup(ColourComp* parent) :
				selector(ColourSelector::ColourSelectorOptions::showAlphaChannel |
					ColourSelector::ColourSelectorOptions::showColourspace |
					ColourSelector::ColourSelectorOptions::showSliders)
			{
				selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
				selector.setColour(ColourSelector::ColourIds::labelTextColourId, Colours::white);

				selector.setCurrentColour(parent->colour);

				addAndMakeVisible(selector);

				selector.addChangeListener(parent);

				setSize(300, 300);
			}

			void resized() override
			{
				selector.setBounds(getLocalBounds().reduced(10));
			}

			ColourSelector selector;
		};

		ColourComp()
		{
			addAndMakeVisible(l);

			l.setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
			l.setColour(Label::ColourIds::outlineColourId, Colours::transparentBlack);
			l.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
			l.addListener(this);
			l.setFont(GLOBAL_BOLD_FONT());
			l.setEditable(true);
		}

		void setDisplayedColour(Colour& c)
		{
			colour = c;

			Colour textColour = Colours::white;

			l.setColour(Label::ColourIds::textColourId, Colours::white);
			l.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
			l.setColour(TextEditor::ColourIds::highlightColourId, textColour.contrasting(0.5f));
			l.setColour(TextEditor::ColourIds::highlightedTextColourId, textColour);

			l.setText("#" + colour.toDisplayString(true), dontSendNotification);

			repaint();
		}

		void mouseDown(const MouseEvent& event)
		{
			auto p = new Popup(this);

			auto root = findParentComponentOfClass<FloatingTile>()->getRootFloatingTile();

			CallOutBox::launchAsynchronously(p, root->getLocalArea(this, getLocalBounds()), root);
		}

		void changeListenerCallback(ChangeBroadcaster* b)
		{
			auto selector = dynamic_cast<ColourSelector*>(b);

			updateColour(selector->getCurrentColour());
		}

		void resized() override
		{
			l.setBounds(getLocalBounds().withWidth(80));
		}

		void labelTextChanged(Label* labelThatHasChanged) override
		{
			const String t = l.getText().trimCharactersAtStart("#");

			auto c = Colour::fromString(t);

			updateColour(c);
		}

		void paint(Graphics& g) override
		{
			auto r = getLocalBounds();
			r.removeFromLeft(80);
			g.fillCheckerBoard(r.toFloat(), 10, 10, Colour(0xFF888888), Colour(0xFF444444));
			g.setColour(colour);
			g.fillRect(r);
			g.setColour(Colours::white.withAlpha(0.5f));
			g.drawRect(r, 1);
		}

	private:

		void updateColour(Colour c)
		{
			auto prop = findParentComponentOfClass<ColourSelectorPropertyComponent>();
			var newValue = var((int64)c.getARGB());
			prop->value.setValue(newValue);
		}

		Label l;

		Colour colour;
	};

	ColourComp comp;
};

struct ToggleButtonPropertyComponent : public PropertyComponent,
									   public Value::Listener,
									   public Button::Listener
{
	ToggleButtonPropertyComponent(ValueTree& data, const Identifier& id, UndoManager* um):
		PropertyComponent(id.toString()),
		v(data.getPropertyAsValue(id, um)),
		b("")
	{
		addAndMakeVisible(b);
		b.setLookAndFeel(&laf);
		b.setClickingTogglesState(true);
		v.addListener(this);
		b.addListener(this);
		update(data[id]);
	}

	~ToggleButtonPropertyComponent()
	{
		v.removeListener(this);
	}

	void buttonClicked(Button*) override
	{
		v.setValue(b.getToggleState());
	}

	void update(const var& value)
	{
		b.setToggleState((bool)value, dontSendNotification);
		b.setButtonText(b.getToggleState() ? "Enabled" : "Disabled");
	}

	void valueChanged(Value& value)
	{
		update(value.getValue());
	}

	void refresh() override
	{

	}

	HiPropertyPanelLookAndFeel laf;
	TextButton b;
	juce::Value v;
};

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
		struct Laf : public LookAndFeel_V3
		{
			Label* createSliderTextBox(Slider& s) override
			{
				auto l = LookAndFeel_V3::createSliderTextBox(s);
				l->setFont(GLOBAL_BOLD_FONT());
				return l;
			}

			void drawLinearSlider(Graphics& g, int , int , int , int , float , float , float , const Slider::SliderStyle, Slider& s)
			{
				NormalisableRange<double> range = { s.getMinimum(), s.getMaximum() };

				range.interval = s.getInterval();
				range.skew = s.getSkewFactor();

				auto value = s.getValue();
				auto bipolar = range.start < 0.0 && range.end > 0.0;

				auto b = s.getLocalBounds().toFloat().reduced(2.0f);

				g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff393939)));
				g.fillRect(s.getLocalBounds());

				g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));
				g.drawRect(s.getLocalBounds());

				g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff646464)));

				float x, y, w, h;

				y = b.getY();
				w = range.convertTo0to1(value) * b.getWidth();
				h = b.getHeight();

				if (bipolar)
				{
					if (value > 0.0)
					{
						x = b.getCentreX();
						w -= x;
					}
					else
					{
						x = w;
						w = b.getCentreX() - x;
					}
				}
				else
				{
					x = b.getX();
				}

				Rectangle<float> ar = { x, y, w, h };

				g.fillRect(ar);
			}
		};

		SliderWithLimitSetter()
		{
			setSliderStyle(Slider::LinearBar);
			setLookAndFeel(&laf);
			setColour(Slider::ColourIds::thumbColourId, Colour(0xFF666666));
			setColour(Slider::ColourIds::textBoxTextColourId, Colours::white);
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

		Laf laf;

	} c;
};

juce::PropertyComponent* PropertyHelpers::createPropertyComponent(ProcessorWithScriptingContent* p, ValueTree& d, const Identifier& id, UndoManager* um)
{
	using namespace PropertyIds;

	auto value = d.getPropertyAsValue(id, um, true);
	auto name = id.toString();

	Identifier propId = Identifier(name.fromLastOccurrenceOf(".", false, false));

	if (propId == Converter || propId == OpType)
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

	if (id == NodeColour)
		return new ColourSelectorPropertyComponent(d, id, um);

	if (id == MinValue || id == MaxValue)
		return new SliderWithLimit(d, id, um);

	if (propId == LockNumChannels)
	{
		return new ToggleButtonPropertyComponent(d, id, um);
	}

	bool isComment = id == PropertyIds::Comment;

	return new TextPropertyComponent(value, name, isComment ? 2048 : 256, isComment);
}

}

