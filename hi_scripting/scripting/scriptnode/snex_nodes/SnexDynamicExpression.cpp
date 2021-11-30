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
using namespace snex;





dynamic_expression::graph::graph(PooledUIUpdater* u, dynamic_expression* e) :
	simple_visualiser(nullptr, u),
	expr(e)
{
	thickness = 3.0f;
	drawBackground = false;
}

float dynamic_expression::graph::intersectsPath(Path& path, Rectangle<float> b)
{
	PathFlatteningIterator i(path, AffineTransform(), JUCE_LIVE_CONSTANT(0.6));

	while (i.next())
	{
		Rectangle<float> pb({ i.x1, i.y1 }, { i.x2, i.y2 });

		if (b.intersectRectangle(pb))
		{
			auto midX = pb.getCentreX();
			return jmax<float>(Math.abs(midX - b.getX()), Math.abs(midX - b.getRight()));
		}
	}

	return 0.0f;
}

juce::NormalisableRange<double> dynamic_expression::graph::getXRange()
{
	if (expr != nullptr && expr->isMathNode)
		return NormalisableRange<double>(-1.0, 1.0);
	else
    {
        if(auto n = getNode())
            return RangeHelpers::getDoubleRange(n->getParameterFromIndex(0)->data).rng;
    }
    
    return {0.0, 1.0};
}

double dynamic_expression::graph::getInputValue()
{
	if (expr != nullptr && expr->isMathNode)
		return expr->lastInput;
	else if (auto n = getNode())
		return n->getParameterFromIndex(0)->getValue();
    
    return 0.0;
}

float dynamic_expression::graph::getValue(double x)
{
	if (expr->isMathNode)
		return expr->expr->getFloatValueWithInput(x, expr->lastValue);
	else
		return (float)expr->expr->getValue(x);
}

void dynamic_expression::graph::rebuildPath(Path& path)
{
    if(expr == nullptr && getNode() == nullptr)
        return;
    
	auto pRange = getXRange();

	auto v = getInputValue();

	original.clear();

	if (auto j = expr->expr)
	{
		yMin = 100000000.0;
		yMax = -100000000.0;

		auto delta = 1.0f / (float)getWidth();

		auto start = 0.0f;

		gridPath.startNewSubPath(0.0, 0.0f);
		gridPath.lineTo(0.0, 1.0f);
		gridPath.startNewSubPath(1.0, 0.0f);
		gridPath.lineTo(1.0, 1.0f);

		auto xmid = pRange.convertFrom0to1(0.5) - pRange.start / (pRange.end - pRange.start);

		gridPath.startNewSubPath(xmid, 0.0f);
		gridPath.lineTo(xmid, 1.0f);

		if (expr->isMathNode)
		{
			p.startNewSubPath(-1.0f, 0.0f);
			p.startNewSubPath(1.0f, 2.0f);
			original.startNewSubPath(-1.0f, 0.0f);
			original.startNewSubPath(1.0f, 2.0f);
		}

		for (int i = 0; i <= getWidth(); i++)
		{
			auto x = start + (double)i * delta;

			x = pRange.convertFrom0to1(x);

			auto y = getValue(x);

			FloatSanitizers::sanitizeFloatNumber(y);

			yMax = jmax(y, yMax);
			yMin = jmin(y, yMin);

			y = 1.0f - y;

			if (i == 0)
			{
				original.startNewSubPath(x, y);
				path.startNewSubPath(x, y);
			}
			else
			{
				original.lineTo(x, y);

				if (x < v)
					path.lineTo(x, y);
				else
					path.startNewSubPath(x, y);
			}
		}
	}
}

dynamic_expression::editor::editor(dynamic_expression* e, PooledUIUpdater* u, bool isMathNode) :
	ScriptnodeExtraComponent<dynamic_expression>(e, u),
	mathNode(isMathNode),
	d(u),
	eg(u, e),
	debugButton("debug", this, f)
{
	addAndMakeVisible(d);
	addAndMakeVisible(te);
	addAndMakeVisible(eg);
	addAndMakeVisible(debugButton);

	debugButton.setClickingTogglesState(true);
	debugButton.setLookAndFeel(&laf);
	debugButton.setToggleModeWithColourChange(true);
	debugButton.getToggleStateValue().referTo(getObject()->debugEnabled.asJuceValue());

	te.setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
	te.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	te.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	te.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
	te.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
	te.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	te.addListener(this);


	te.setMultiLine(true, true);

	te.setFont(GLOBAL_MONOSPACE_FONT());
	te.getTextValue().referTo(e->code.asJuceValue());
	te.setScrollToShowCursor(true);

	addAndMakeVisible(logger);
	logger.setFont(GLOBAL_MONOSPACE_FONT());
	logger.setMultiLine(true);
	logger.setReadOnly(true);

	logger.setColour(TextEditor::ColourIds::textColourId, Colours::white);
	logger.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	logger.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	logger.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
	logger.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);

	setSize(300, 180 - (mathNode ? 28 : 0));
}

void dynamic_expression::editor::textEditorReturnKeyPressed(TextEditor&)
{
	te.getTextValue().setValue(te.getText());
}

void dynamic_expression::editor::buttonClicked(Button* b)
{

}

String dynamic_expression::editor::getValueString(float v)
{
	v = Math.round(v * 10.0f) / 10.0f;

	return String(v, 1);
}

int dynamic_expression::editor::getYAxisLabelWidth() const
{
	auto minValue = getValueString(eg.yMin);
	auto maxValue = getValueString(eg.yMax);

	auto f = GLOBAL_BOLD_FONT();

	auto v = jmax(f.getStringWidthFloat(maxValue),
		f.getStringWidthFloat(minValue));

	return Math.ceil(v / 10.0f) * 10;
}

void dynamic_expression::editor::drawYAxisValues(Graphics& g)
{
	if (!eg.isVisible())
		return;

	auto minValue = getValueString(eg.yMin);
	auto maxValue = getValueString(eg.yMax);

	auto f = GLOBAL_BOLD_FONT();

	auto a = logger.getBounds();

	auto b = a.removeFromTop(18).removeFromRight(f.getStringWidthFloat(maxValue) + 5.0f).toFloat();

	g.setFont(f);

	g.setColour(Colours::white.withAlpha(0.4f));

	g.drawText(maxValue, b, Justification::left);

	b = a.removeFromBottom(18).removeFromRight(f.getStringWidthFloat(minValue) + 5.0f).toFloat();

	g.drawText(minValue, b, Justification::left);
}

void dynamic_expression::editor::resized()
{
	auto b = getLocalBounds();

	auto teb = b.removeFromTop(codeHeight + JUCE_LIVE_CONSTANT_OFF(8));
	teb.removeFromLeft(70);



	debugButton.setBounds(teb.removeFromRight(28).removeFromTop(28).reduced(6));

	if (!mathNode)
	{
		auto bottom = b.removeFromBottom(28);
		d.setBounds(bottom);
		b.removeFromBottom(UIValues::NodeMargin);
	}
	else
		d.setVisible(false);

	te.setBounds(teb);

	logger.setBounds(b.reduced(UIValues::NodeMargin));
	logger.setScrollbarsShown(false);

	auto egBounds = logger.getBounds();
	egBounds.removeFromRight(getYAxisLabelWidth());

	eg.setBounds(egBounds);
}

juce::Component* dynamic_expression::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
{
	auto t = static_cast<mothernode*>(obj);

	if (auto n = dynamic_cast<ControlNodeType*>(t))
		return new editor(&n->obj, updater, false);
	else if (auto n = dynamic_cast<MathNodeType*>(t))
		return new editor(&n->obj, updater, true);
    else
    {
        jassertfalse;
        return nullptr;
    }
}

void dynamic_expression::editor::paint(Graphics& g)
{
	auto darkBackground = getLocalBounds().toFloat().removeFromTop(logger.getBottom());

	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, darkBackground, false);

	drawYAxisValues(g);

	auto b = darkBackground.removeFromTop(jmax<float>(te.getHeight(), 24 - 3.0)).toFloat();

	auto c = Colours::green;

	if (!getObject()->warning.wasOk())
		c = Colours::yellow;

	if (!getObject()->r.wasOk())
		c = Colours::red;

	c = c.withSaturation(0.5f);

	g.setColour(c.withAlpha(0.05f));
	g.fillRect(b);

	g.setColour(Colours::white.withAlpha(0.4f));
	g.setFont(GLOBAL_MONOSPACE_FONT());

	auto ta = b.removeFromLeft(65.0f).translated(0.0f, 4.5f);

	g.drawText("output =", ta, Justification::topRight);
}

void dynamic_expression::editor::timerCallback()
{
	repaint();

	if (getObject() == nullptr)
		return;

	showError = !getObject()->r.wasOk();

	auto showLogger = debugButton.getToggleState() || showError;

	logger.setVisible(showLogger);

	logger.setText(getObject()->createMessageList(), dontSendNotification);

	eg.setVisible(!showLogger);

	auto thisHeight = te.getTextHeight();

	auto thisWidth = getYAxisLabelWidth();

	if (thisWidth != ywidth ||
		thisHeight != codeHeight)
	{
		codeHeight = thisHeight;
		ywidth = thisWidth;

		resized();
	}
}

juce::Path dynamic_expression::editor::Factory::createPath(const String& id) const
{
	Path p;
	p.loadPathFromData(SnexIcons::bugIcon, sizeof(SnexIcons::bugIcon));
	return p;
}

dynamic_expression::dynamic_expression() :
	code(PropertyIds::Code, "input"),
	debugEnabled(PropertyIds::Debug, false),
	r(Result::ok()),
	warning(Result::ok())
{

}

void dynamic_expression::initialise(NodeBase* n)
{
	isMathNode = n->getPath().getParent().getIdentifier() == Identifier("math");

	code.initialise(n);
	code.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic_expression::updateCode), true);

	debugEnabled.initialise(n);
}

void dynamic_expression::logMessage(int level, const juce::String& s)
{
	if (level > 1)
		return;

	if (level == 1)
	{
		warning = Result::fail(s);
	}

	messages[messageIndex] = s;
	messageIndex++;
}

String dynamic_expression::createMessageList() const
{
	IndexType ri((int)messageIndex);

	String s;

	if (!r.wasOk())
		s << r.getErrorMessage() << "\n";

	if (!warning.wasOk())
		s << warning.getErrorMessage() << "\n";

	if (s.isEmpty())
	{
		for (int i = 0; i < NumMessages; i++)
		{
			auto x = messages[ri];

			if (!x.isEmpty())
				s << x << "\n";

			++ri;
		}
	}

	return s;
}

void dynamic_expression::updateCode(Identifier id, var newValue)
{
	for (auto& m : messages)
		m = {};

	messageIndex = 0;

	warning = Result::ok();

	snex::JitExpression::Ptr newCode = new snex::JitExpression(newValue.toString(), this, isMathNode);

	if (newCode->isValid())
	{
		SimpleReadWriteLock::ScopedWriteLock sl(lock);
		std::swap(newCode, expr);
		r = Result::ok();
	}
	else
	{
		r = Result::fail(newCode->getErrorMessage());
		warning = Result::ok();
	}
}

double dynamic_expression::op(double input)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (expr != nullptr)
	{
		auto output = expr->getValueUnchecked(input);

		if (debugEnabled.getValue())
		{
			String s;
			s << String(input) << " -> " << String(output);
			logMessage(0, s);
		}

		return output;
	}
    
    return 0.0;
}

void dynamic_expression::updateUIValue()
{
	if (debugEnabled.getValue() && updateCounter++ > 50)
	{
		updateCounter = 0;

		String s;

		auto output = expr->getFloatValueWithInputUnchecked(lastInput, lastValue);

		s << String(lastInput) << " -> " << String(output);
		logMessage(0, s);
	}
}

}

