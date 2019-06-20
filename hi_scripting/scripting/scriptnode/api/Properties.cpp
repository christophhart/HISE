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

	return new TextPropertyComponent(value, name, 256, false);
}

namespace Icons
{
static const unsigned char splitIcon[] = { 110,109,63,181,100,66,139,204,55,67,108,0,0,0,0,244,61,253,66,108,205,204,76,61,90,36,253,66,108,150,195,5,66,90,36,253,66,108,98,144,5,66,244,61,253,66,108,51,51,145,66,123,212,37,67,108,51,51,145,66,90,36,253,66,108,186,137,200,66,90,36,253,66,108,
186,137,200,66,135,22,37,67,108,246,200,10,67,90,36,253,66,108,80,45,44,67,90,36,253,66,108,254,20,231,66,254,52,55,67,98,43,135,237,66,186,105,60,67,131,64,241,66,139,140,66,67,131,64,241,66,244,29,73,67,98,131,64,241,66,20,238,91,67,182,179,210,66,
123,52,107,67,117,19,173,66,123,52,107,67,98,176,114,135,66,123,52,107,67,205,204,81,66,20,238,91,67,205,204,81,66,244,29,73,67,98,205,204,81,66,199,203,66,67,45,178,88,66,0,224,60,67,63,181,100,66,139,204,55,67,99,109,113,61,98,66,156,68,73,66,98,137,
193,87,66,195,245,53,66,205,204,81,66,16,216,31,66,205,204,81,66,29,90,8,66,98,205,204,81,66,78,98,116,65,176,114,135,66,0,0,0,0,117,19,173,66,0,0,0,0,98,182,179,210,66,0,0,0,0,131,64,241,66,78,98,116,65,131,64,241,66,29,90,8,66,98,131,64,241,66,106,
188,32,66,68,11,238,66,209,162,55,66,14,109,232,66,182,115,75,66,108,80,45,44,67,109,167,213,66,108,131,32,44,67,6,193,213,66,108,106,188,10,67,6,193,213,66,108,246,200,10,67,109,167,213,66,108,109,39,199,66,106,60,135,66,108,109,39,199,66,6,193,213,
66,108,229,208,143,66,6,193,213,66,108,229,208,143,66,82,184,136,66,108,98,144,5,66,6,193,213,66,108,0,0,0,0,6,193,213,66,108,113,61,98,66,156,68,73,66,99,101,0,0 };

static const unsigned char chainIcon[] = { 110,109,154,153,148,65,145,45,174,66,108,154,153,148,65,150,67,124,66,98,102,102,242,64,123,148,102,66,0,0,0,0,162,69,57,66,0,0,0,0,164,240,4,66,98,0,0,0,0,162,69,110,65,162,69,110,65,0,0,0,0,164,240,4,66,0,0,0,0,98,229,80,78,66,0,0,0,0,39,241,132,66,
162,69,110,65,39,241,132,66,164,240,4,66,98,39,241,132,66,162,69,57,66,129,149,107,66,123,148,102,66,129,149,63,66,150,67,124,66,108,129,149,63,66,145,45,174,66,98,172,156,106,66,57,244,184,66,10,23,132,66,63,53,207,66,10,23,132,66,84,227,232,66,98,10,
23,132,66,180,72,1,67,172,156,106,66,121,105,12,67,129,149,63,66,139,204,17,67,108,129,149,63,66,207,215,45,67,98,129,149,107,66,150,67,51,67,39,241,132,66,76,151,62,67,39,241,132,66,139,172,75,67,98,39,241,132,66,90,4,94,67,229,80,78,66,180,232,108,
67,164,240,4,66,180,232,108,67,98,162,69,110,65,180,232,108,67,0,0,0,0,90,4,94,67,0,0,0,0,139,172,75,67,98,0,0,0,0,76,151,62,67,102,102,242,64,150,67,51,67,154,153,148,65,207,215,45,67,108,154,153,148,65,139,204,17,67,98,14,45,250,64,121,105,12,67,172,
28,218,62,180,72,1,67,172,28,218,62,84,227,232,66,98,172,28,218,62,63,53,207,66,14,45,250,64,57,244,184,66,154,153,148,65,145,45,174,66,99,101,0,0 };

static const unsigned char multiIcon[] = { 110,109,154,153,148,65,150,67,124,66,98,102,102,242,64,123,148,102,66,0,0,0,0,162,69,57,66,0,0,0,0,164,240,4,66,98,0,0,0,0,162,69,110,65,162,69,110,65,0,0,0,0,164,240,4,66,0,0,0,0,98,229,80,78,66,0,0,0,0,39,241,132,66,162,69,110,65,39,241,132,66,164,
240,4,66,98,39,241,132,66,162,69,57,66,129,149,107,66,123,148,102,66,129,149,63,66,150,67,124,66,108,129,149,63,66,207,215,45,67,98,129,149,107,66,150,67,51,67,39,241,132,66,76,151,62,67,39,241,132,66,139,172,75,67,98,39,241,132,66,90,4,94,67,229,80,
78,66,180,232,108,67,164,240,4,66,180,232,108,67,98,162,69,110,65,180,232,108,67,0,0,0,0,90,4,94,67,0,0,0,0,139,172,75,67,98,0,0,0,0,76,151,62,67,102,102,242,64,150,67,51,67,154,153,148,65,207,215,45,67,108,154,153,148,65,150,67,124,66,99,109,33,144,
10,67,150,67,124,66,98,197,32,255,66,123,148,102,66,219,249,239,66,162,69,57,66,219,249,239,66,164,240,4,66,98,219,249,239,66,162,69,110,65,137,225,6,67,0,0,0,0,88,57,25,67,0,0,0,0,98,39,145,43,67,0,0,0,0,129,117,58,67,162,69,110,65,129,117,58,67,164,
240,4,66,98,129,117,58,67,162,69,57,66,78,226,50,67,123,148,102,66,78,226,39,67,150,67,124,66,108,78,226,39,67,207,215,45,67,98,78,226,50,67,150,67,51,67,129,117,58,67,76,151,62,67,129,117,58,67,139,172,75,67,98,129,117,58,67,90,4,94,67,39,145,43,67,
180,232,108,67,88,57,25,67,180,232,108,67,98,137,225,6,67,180,232,108,67,219,249,239,66,90,4,94,67,219,249,239,66,139,172,75,67,98,219,249,239,66,76,151,62,67,197,32,255,66,150,67,51,67,33,144,10,67,207,215,45,67,108,33,144,10,67,150,67,124,66,99,101,
0,0 };

static const unsigned char modIcon[] = { 110,109,53,222,10,66,127,106,101,66,98,127,234,31,66,74,12,40,66,27,175,59,66,172,28,219,65,98,16,100,66,137,65,118,65,98,178,29,128,66,57,180,224,64,16,152,147,66,240,167,70,62,188,116,170,66,111,18,3,59,98,139,236,170,66,0,0,0,0,90,100,171,66,111,18,
131,186,172,220,171,66,111,18,131,58,98,139,44,204,66,104,145,109,62,180,72,230,66,231,251,105,65,248,211,245,66,209,34,222,65,98,242,50,9,67,51,179,79,66,141,23,16,67,236,145,159,66,12,66,25,67,150,131,213,66,98,76,119,30,67,8,44,244,66,168,166,35,67,
205,140,9,67,186,233,42,67,250,222,23,67,108,184,254,42,67,240,7,24,67,98,0,96,48,67,162,69,12,67,51,243,52,67,213,56,0,67,70,118,57,67,82,56,232,66,98,33,48,53,67,84,35,220,66,170,177,50,67,23,153,205,66,170,177,50,67,195,245,189,66,98,170,177,50,67,
111,18,148,66,207,151,68,67,190,31,100,66,215,163,90,67,190,31,100,66,98,223,175,112,67,190,31,100,66,2,75,129,67,111,18,148,66,2,75,129,67,195,245,189,66,98,2,75,129,67,154,217,231,66,223,175,112,67,211,237,4,67,215,163,90,67,211,237,4,67,98,131,96,
88,67,211,237,4,67,115,40,86,67,0,192,4,67,59,255,83,67,240,103,4,67,98,39,145,80,67,119,126,13,67,94,26,77,67,252,137,22,67,125,95,73,67,133,107,31,67,98,53,190,68,67,236,113,42,67,125,191,63,67,41,92,54,67,250,254,52,67,45,178,59,67,98,127,10,46,67,
162,37,63,67,188,116,37,67,94,122,62,67,188,180,30,67,217,206,57,67,98,229,144,20,67,133,203,50,67,84,131,15,67,154,185,38,67,66,192,10,67,137,193,27,67,98,68,139,0,67,244,61,4,67,240,103,242,66,225,122,215,66,219,57,226,66,166,27,166,66,98,172,220,215,
66,201,182,134,66,225,250,204,66,123,148,77,66,164,240,185,66,147,152,24,66,98,141,215,181,66,39,49,13,66,248,147,177,66,16,88,0,66,125,63,171,66,106,188,246,65,98,125,63,171,66,106,188,246,65,186,9,166,66,231,251,1,66,51,179,162,66,59,95,9,66,98,106,
60,147,66,129,149,43,66,248,83,138,66,55,137,88,66,37,6,130,66,117,147,130,66,108,158,239,129,66,223,207,130,66,98,78,34,148,66,94,186,144,66,55,201,159,66,223,15,166,66,55,201,159,66,195,245,189,66,98,55,201,159,66,154,217,231,66,213,248,119,66,211,
237,4,67,180,200,31,66,211,237,4,67,98,39,49,143,65,211,237,4,67,0,0,0,0,154,217,231,66,0,0,0,0,195,245,189,66,98,0,0,0,0,33,112,151,66,39,49,114,65,246,40,111,66,53,222,10,66,127,106,101,66,99,101,0,0 };

static const unsigned char frameIcon[] = { 110,109,180,200,243,65,0,0,0,0,98,98,144,49,66,113,61,10,62,250,254,101,66,178,157,45,65,141,151,112,66,223,79,197,65,98,219,249,122,66,229,208,24,66,127,106,91,66,14,45,84,66,188,116,40,66,125,191,105,66,98,233,38,250,65,178,29,124,66,92,143,142,65,
82,56,114,66,82,184,20,65,76,183,81,66,98,119,190,31,61,133,107,46,66,238,124,47,192,139,108,229,65,225,122,60,64,18,131,136,65,98,178,157,251,64,8,172,224,64,41,92,148,65,162,69,54,62,123,20,239,65,111,18,3,59,98,227,165,240,65,0,0,0,0,76,55,242,65,
0,0,0,0,180,200,243,65,0,0,0,0,99,109,111,18,241,65,248,83,5,65,98,219,249,160,65,72,225,6,65,229,208,42,65,248,83,128,65,186,73,10,65,82,184,207,65,98,168,198,211,64,66,96,15,66,150,67,69,65,63,53,59,66,250,126,173,65,80,13,75,66,98,246,168,7,66,131,
192,95,66,115,104,72,66,193,202,61,66,0,128,80,66,117,147,6,66,98,57,180,87,66,193,202,170,65,197,32,47,66,215,163,8,65,6,129,244,65,41,92,5,65,98,41,92,243,65,248,83,5,65,76,55,242,65,248,83,5,65,111,18,241,65,248,83,5,65,99,109,80,13,17,66,74,12,72,
65,108,80,13,17,66,135,150,63,66,108,92,143,232,65,135,150,63,66,108,92,143,232,65,76,55,167,65,108,143,194,175,65,63,53,217,65,108,143,194,141,65,25,4,178,65,108,233,38,234,65,74,12,72,65,108,80,13,17,66,74,12,72,65,99,101,0,0 };
}

}

