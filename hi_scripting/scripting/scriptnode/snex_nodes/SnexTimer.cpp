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
#if HISE_INCLUDE_SNEX
		cppgen::Base c(cppgen::Base::OutputType::AddTabs);

		cppgen::Struct s(c, id, {}, { TemplateParameter(NamespacedIdentifier("NumVoices"), 0, false) });

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
#else
		return {};
#endif
	}

	double snex_timer::getTimerValue()
	{
		double v;

		switch (currentMode)
		{
		case TimerMode::Ping:   v = pingTimer.getTimerValue(); break;

		case TimerMode::Custom: 
#if HISE_INCLUDE_SNEX
			v = callbacks.getTimerValue(); 
#else
			jassertfalse;
            v = 0.0;
#endif
			break;
		case TimerMode::Toggle: v = toggleTimer.getTimerValue(); break;
		case TimerMode::Random: v = randomTimer.getTimerValue(); break;
        default:                v = 0.0f; break;
		}

		lastValue.setModValue(v);
		return v;
	}

#if HISE_INCLUDE_SNEX
	snex_timer::editor::editor(snex_timer* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<snex_timer>(t, updater),
		menuBar(t),
		dragger(updater),
		modKnob(updater, &t->lastValue),
		modeSelector("toggle")
	{
		modeSelector.initModes(snex_timer::getModes(), t->getParentNode());
		t->addCompileListener(this);

		addAndMakeVisible(modKnob);
		addAndMakeVisible(menuBar);
		addAndMakeVisible(modeSelector);
		this->addAndMakeVisible(dragger);

		this->setSize(200, 110);
	}

	juce::Component* snex_timer::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto mn = static_cast<mothernode*>(obj);
		auto t = dynamic_cast<timer_base<snex_timer>*>(mn);
		return new editor(&t->tType, updater);
	}

	void snex_timer::editor::resized()
	{
		auto b = this->getLocalBounds();

		auto top = b.removeFromTop(24);
		
		menuBar.setBounds(top);

		b.removeFromTop(UIValues::NodeMargin);
		b.removeFromBottom(UIValues::NodeMargin);
		
		auto r = b.removeFromRight(b.getHeight());
		auto l = b;
		
		modeSelector.setBounds(l.removeFromTop(24));
		dragger.setBounds(l.removeFromBottom(28));

		modKnob.setBounds(r.reduced(UIValues::NodeMargin));

	}

	void snex_timer::editor::paint(Graphics& g)
	{
		
	}

	void snex_timer::editor::timerCallback()
	{
		auto snexEnabled = getObject()->currentMode == snex_timer::TimerMode::Custom;
		menuBar.setAlpha(snexEnabled ? 1.0f : 0.1f);

		if (getObject() == nullptr)
		{
			stop();
			return;
		}
	}
#endif
}

void FlashingModKnob::paint(Graphics& g)
{
	auto v = modValue->getModValue();

	auto maxSize = (float)jmin(getWidth(), getHeight()) - 2.0f;

	auto c = Colour(0xFFDADADA);

	auto flashDot = getLocalBounds().toFloat().withSizeKeepingCentre(maxSize, maxSize);

	Path p;

	const float s = 2.4f;

	p.addPieSegment(flashDot.reduced(maxSize * 0.02f), -s, s, JUCE_LIVE_CONSTANT_OFF(0.93f));
	p.addPieSegment(flashDot, -s, -s + s * 2.0f * (float)v, JUCE_LIVE_CONSTANT_OFF(0.85f));

	g.setColour(c);

	g.fillPath(p);

	g.drawEllipse(flashDot.reduced(maxSize * 0.2f), 1.0f);

	if(on)
		g.fillEllipse(flashDot.reduced(maxSize * 0.25f));
}

}

