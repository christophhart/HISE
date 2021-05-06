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


namespace control
{
	String snex_timer::getEmptyText(const Identifier& id) const
	{
		cppgen::Base c(cppgen::Base::OutputType::AddTabs);

		cppgen::Struct s(c, id, {}, {});

		addSnexNodeId(c, id);

		c.addComment("Calculate a new timer value here", snex::cppgen::Base::CommentType::Raw);
		c << "double getTimerValue()\n";
		c << "{\n    return 0.0;\n}\n";

		c.addComment("Reset any state here", snex::cppgen::Base::CommentType::Raw);
		c << "void reset()\n";
		c << "{\n    \n}\n";

		c.addComment("Initialise the processing", snex::cppgen::Base::CommentType::Raw);
		c << "void prepare(PrepareSpecs ps)\n";
		c << "{\n    \n}\n";

		String pf;
		c.addEmptyLine();
		addDefaultParameterFunction(pf);
		c << pf;

		s.flushIfNot();

		return c.toString();
	}

	double snex_timer::getTimerValue()
	{
		auto v = callbacks.getTimerValue();
		lastValue.setModValue(v);
		return v;
	}

	snex_timer::editor::editor(snex_timer* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<snex_timer>(t, updater),
		menuBar(t),
		dragger(updater),
		modKnob(updater, &t->lastValue)
	{
		t->addCompileListener(this);

		addAndMakeVisible(modKnob);
		addAndMakeVisible(menuBar);
		this->addAndMakeVisible(dragger);

		this->setSize(200, 140);
	}

	juce::Component* snex_timer::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new editor(&static_cast<NodeType*>(obj)->tType, updater);
	}

	void snex_timer::editor::resized()
	{
		auto b = this->getLocalBounds();

		auto top = b.removeFromTop(24);
		
		menuBar.setBounds(top);

		b.removeFromTop(UIValues::NodeMargin);
		b.removeFromBottom(UIValues::NodeMargin);
		

		dragger.setBounds(b.removeFromBottom(28));

		modKnob.setBounds(b);

	}

	void snex_timer::editor::paint(Graphics& g)
	{
		
	}

	void snex_timer::editor::timerCallback()
	{
		if (getObject() == nullptr)
		{
			stop();
			return;
		}

		
	}

}

void FlashingModKnob::paint(Graphics& g)
{
	auto v = modValue->getModValue();

	auto maxSize = (float)jmin(getWidth(), getHeight()) - 2.0f;

	auto c = Colour(0xFFDADADA);

	auto flashDot = getLocalBounds().toFloat().withSizeKeepingCentre(maxSize, maxSize);

	Path p;

	const double s = 2.4;

	p.addPieSegment(flashDot.reduced(maxSize * 0.02f), -s, s, JUCE_LIVE_CONSTANT_OFF(0.93f));
	p.addPieSegment(flashDot, -s, -s + s * 2.0 * v, JUCE_LIVE_CONSTANT_OFF(0.85f));

	g.setColour(c);

	g.fillPath(p);

	g.drawEllipse(flashDot.reduced(maxSize * 0.2f), 1.0f);

	if(on)
		g.fillEllipse(flashDot.reduced(maxSize * 0.25f));
}

}

