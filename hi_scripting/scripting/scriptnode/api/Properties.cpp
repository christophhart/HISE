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
                setLookAndFeel(&laf);
				
                selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
                selector.setColour(ColourSelector::ColourIds::labelTextColourId, Colours::white);
                selector.setColour(ColourSelector::ColourIds::labelTextColourId, Colours::white);

                juce::Component::callRecursive<Component>(&selector, [](Component* s)
                {
                    s->setColour(Slider::ColourIds::textBoxTextColourId, Colours::white.withAlpha(0.8f));
                    s->setColour(Slider::ColourIds::backgroundColourId, Colours::black.withAlpha(0.3f));
                    s->setColour(Slider::ColourIds::thumbColourId, Colours::white.withAlpha(0.8f));
                    s->setColour(Slider::ColourIds::trackColourId, Colours::white.withAlpha(0.5f));
                    return false;
                });
                
                
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
            LookAndFeel_V4 laf;
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

		void mouseDown(const MouseEvent& )
		{
			auto p = new Popup(this);

			Component* root = findParentComponentOfClass<ZoomableViewport>();

			if (root == nullptr)
				root = findParentComponentOfClass<PropertyPanel>();

			CallOutBox::launchAsynchronously(std::unique_ptr<Component>(p), root->getLocalArea(this, getLocalBounds()), root);
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

		void labelTextChanged(Label* ) override
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

		DBG(data.createXml()->createDocument(""));

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

		auto min = (double)jmin(0.0, (double)v, (double)data.getProperty(MinValue, 0.0));
		auto max = (double)jmax(v, data.getProperty(MaxValue, 1.0));
		auto stepSize = data.getProperty(StepSize, 0.01);
		
		if (min > max)
			std::swap(min, max);

		c.setScrollWheelEnabled(false);
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
				w = (float)range.convertTo0to1(value) * b.getWidth();
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
			setColour(juce::Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
			setColour(juce::Slider::textBoxHighlightColourId, Colour(SIGNAL_COLOUR));
		}

		String getTextFromValue(double v) override
		{
			return snex::Types::Helpers::getCppValueString(var(v), snex::Types::ID::Double);
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

#if HISE_INCLUDE_SNEX
struct ExpressionPropertyComponent : public PropertyComponent
{
	ExpressionPropertyComponent(ValueTree data, const Identifier& id, UndoManager* um):
		PropertyComponent(id.toString()),
		comp(data.getPropertyAsValue(id, um, true))
	{
		addAndMakeVisible(comp);

		

		
		setPreferredHeight(32);
		
	}

	void refresh() override {};

	struct Comp : public Component,
				  public KeyListener
	{
		struct Display : public Component,
						 public Value::Listener
		{
			Display(const Value& v, bool smallMode_) :
				smallMode(smallMode_),
				value(v)
			{
				value.addListener(this);
				setRepaintsOnMouseActivity(true);
				
				if (!smallMode)
				{
					addAndMakeVisible(label);
					label.setInterceptsMouseClicks(false, false);
				}

			};

			void resized()
			{
				rebuild();

				if (!smallMode)
					label.setBounds(getLocalBounds().removeFromBottom(32));
			}

			~Display()
			{
				value.removeListener(this);
			}

			Value value;

			void mouseDown(const MouseEvent& event) override;

			Label label;

			bool smallMode;
			snex::jit::JitObject expression;
			snex::jit::GlobalScope gs;
			bool empty = false;
			bool ok = false;
			Path p;

			Point<float> hoverPos;

			HeapBlock<double> data;
			int numToUse = 0;

			Range<double> range;

			void mouseMove(const MouseEvent& e)
			{
				auto x = (float)e.getPosition().getX() / (float)getWidth();
				auto y = (float)e.getPosition().getY() / (float)getHeight();
				hoverPos = {x , y};

				
				repaint();
			}

			void mouseExit(const MouseEvent& )
			{
				hoverPos = {};
				repaint();
			}

			void paint(Graphics& g) override
			{
				auto b = getLocalBounds();

				
				auto c = ok ? Colours::green : Colours::red;
				c = c.withSaturation(0.3f);
				c = c.withBrightness(0.4f);

				g.setColour(c);

				g.fillRect(b);
				g.setColour(Colours::white.withAlpha(0.6f));
				g.fillPath(p);

				if (ok && !smallMode)
				{
					
					auto f = GLOBAL_BOLD_FONT();

					auto s = snex::Types::Helpers::getCppValueString(var(range.getStart()), snex::Types::ID::Double);
					auto e = snex::Types::Helpers::getCppValueString(var(range.getEnd()), snex::Types::ID::Double);

					auto sw = f.getStringWidthFloat(s) + 15.0f;
					auto ew = f.getStringWidthFloat(e) + 15.0f;

					g.setFont(f);
					
					b = getLocalBounds();
					auto left = b.removeFromBottom(26).toFloat().removeFromLeft(sw);
					auto right = b.removeFromTop(26).toFloat().removeFromRight(ew);

					

					g.setColour(Colours::black.withAlpha(0.3f));
					g.fillRect(left);
					g.fillRect(right);

					g.setColour(Colours::white.withAlpha(0.8f));

					g.drawText(s, left.reduced(3.0f), Justification::left);
					g.drawText(e, right.reduced(3.0f), Justification::right);

					if (!hoverPos.isOrigin())
					{
						int index = roundToInt(hoverPos.getX() * (float)numToUse);

						index = jlimit(0, numToUse - 1, index);

						auto va = data[index];

						auto xPos = roundToInt(hoverPos.getX() * (float)getWidth());

						auto yNormalised = (va - range.getStart()) / range.getLength();

						auto yPos = roundToInt((1.0f - yNormalised) * (float)getHeight());

						Rectangle<float> circle((float)xPos, (float)yPos, 0.0f, 0.0f);

						g.setColour(Colours::red);
						g.fillEllipse(circle.withSizeKeepingCentre(10.0f, 10.0f));

						String posText;

						posText << String(hoverPos.getX(), 2);
						posText << " | ";
						posText << snex::Types::Helpers::getCppValueString(var(va), snex::Types::ID::Double); 

						auto w = f.getStringWidthFloat(posText) + 10.0f;

						auto bf = circle.withSizeKeepingCentre(w, 20.0f);
						if (yNormalised < 0.5)
							bf = bf.translated(0.0f, -30.0f);
						else
							bf = bf.translated(0.0f, 30.0f);

						if(bf.getRight() > (float)getWidth())
							bf = bf.withX((float)getWidth() - bf.getWidth());

						if (bf.getX() < 0.0f)
							bf = bf.withX(0.0f);

						g.setColour(Colours::black.withAlpha(0.5f));
						g.fillRoundedRectangle(bf, 2.0f);
						g.setColour(Colours::white.withAlpha(0.8f));
						g.drawText(posText, bf, Justification::centred);
					}
				}
			}

			void rebuild()
			{
				empty = value.toString().isEmpty();

				numToUse = 0;
				data.free();

				if (!empty)
				{
					String s = "double get(double input){ return ";
					s << value.toString() << "; }";

					snex::jit::Compiler compiler(gs);
					expression = compiler.compileJitObject(s);

					ok = compiler.getCompileResult().wasOk();

					if (!ok)
						label.setText(compiler.getCompileResult().getErrorMessage(), dontSendNotification);

					if (auto f = expression["get"])
					{
						numToUse = smallMode ? 32 : 128;

						data.calloc(numToUse);

						double maxValue = -5000000.0;
						double minValue = 5000000.0;

						for (int i = 0; i < numToUse; i++)
						{
							auto v = (double)i / (double)(numToUse-1);
							v = f.call<double>(v);

							minValue = jmin(minValue, v);
							maxValue = jmax(maxValue, v);
							data[i] = v;
						}

						Path newPath;
						newPath.startNewSubPath(0.0f, (float)1.0f);

						for (int i = 0; i < numToUse; i++)
						{
							auto va = data[i];

							if (std::isnan(va) || std::isinf(va))
							{
								ok = false;
								p = {};
								label.setText("Invalid values", dontSendNotification);
								break;
							}

							auto yNormalised = (float)((data[i] - minValue) / (maxValue - minValue));

							newPath.lineTo(((float)(i) / (float)(numToUse - 1)), 1.0f - yNormalised);
						}


						newPath.lineTo(1.0f, 1.0f);
						newPath.closeSubPath();

						auto b = getLocalBounds().toFloat();

						if (newPath.getBounds().getHeight() > 0.0f &&
							newPath.getBounds().getWidth() > 0.0f)
						{
							newPath.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);
							p = newPath;

							range = { minValue, maxValue };
						}
						else
							p = {};

					}
				}
				else
				{
					Path newPath;
					newPath.startNewSubPath(0.0f, 1.0f);
					newPath.lineTo(1.0f, 0.0f);
					newPath.lineTo(1.0f, 1.0f);
					newPath.closeSubPath();

					p = newPath;
				}

				repaint();
			}

			void valueChanged(Value& ) override
			{
				rebuild();
			}
		};

		Comp(const Value& value):
			display(value, true)
		{
			

			addAndMakeVisible(editor);
			editor.setFont(GLOBAL_MONOSPACE_FONT());
			editor.setText(value.toString(), dontSendNotification);
			editor.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.7f));
			editor.setSelectAllWhenFocused(true);
			editor.addKeyListener(this);

			addAndMakeVisible(display);
		}

		bool keyPressed(const KeyPress& key, Component*) override
		{
			if (key == KeyPress::returnKey)
			{
				display.value.setValue(editor.getText());
				return true;
			}

			return false;
		}

		void resized() override
		{
			auto b = getLocalBounds();
			display.setBounds(b.removeFromRight(getHeight() + 3));
			b.removeFromBottom(3);
			editor.setBounds(b);	
		}

		TextEditor editor;
		Display display;
		
	} comp;
};

void ExpressionPropertyComponent::Comp::Display::mouseDown(const MouseEvent& )
{
	Display* bigOne = new Display(value, false);
	bigOne->setSize(300, 300);

	auto pc = findParentComponentOfClass<ZoomableViewport>();

	auto b = pc->getLocalArea(this, getLocalBounds());

	CallOutBox::launchAsynchronously(std::unique_ptr<Component>(bigOne), b, pc);
}
#endif

	Colour PropertyHelpers::getColour(ValueTree data)
	{
		while (data.getParent().isValid())
		{
			if (data.hasProperty(PropertyIds::NodeColour))
			{
				auto c = getColourFromVar(data[PropertyIds::NodeColour]);

				if (!c.isTransparent())
					return c;
			}

			data = data.getParent();
		}

		return Colour();
	}

	Colour PropertyHelpers::getColourFromVar(const var& value)
	{
		int64 colourValue = 0;

		if (value.isInt64() || value.isInt())
			colourValue = (int64)value;
		else if (value.isString())
		{
			auto string = value.toString();

			if (string.startsWith("0x"))
				colourValue = string.getHexValue64();
			else
				colourValue = string.getLargeIntValue();
		}

		return Colour((uint32)colourValue);
	}

	juce::PropertyComponent* PropertyHelpers::createPropertyComponent(ProcessorWithScriptingContent* s, ValueTree& d, const Identifier& id, UndoManager* um)
{
	using namespace PropertyIds;

	auto value = d.getPropertyAsValue(id, um, true);
	auto name = id.toString();

	Identifier propId = Identifier(name.fromLastOccurrenceOf(".", false, false));

	if (id == NodeColour)
		return new ColourSelectorPropertyComponent(d, id, um);

	if (id == MinValue || id == MaxValue)
		return new SliderWithLimit(d, id, um);

	if (propId == SplitSignal ||
		propId == AllowCompilation ||
		propId == HasTail ||
		propId == SuspendOnSilence ||
		propId == AllowPolyphonic)
		return new ToggleButtonPropertyComponent(d, id, um);
    
#if HISE_INCLUDE_SNEX
	if (propId == Expression)
		return new ExpressionPropertyComponent(d, id, um);
#endif

	bool isComment = id == PropertyIds::Comment;

	return new TextPropertyComponent(value, name, isComment ? 2048 : 256, isComment);
}

}

