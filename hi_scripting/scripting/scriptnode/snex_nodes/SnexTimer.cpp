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

		c.addComment("Calculate a new timer value here", snex::cppgen::Base::CommentType::Raw);
		c << "double getTimerValue()\n";
		c << "{\n    return 0.0;\n}\n";

		String pf;
		c.addEmptyLine();
		addDefaultParameterFunction(pf);
		c << pf;

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
		meter(updater)
	{
		t->addCompileListener(this);

		addAndMakeVisible(menuBar);
		this->addAndMakeVisible(dragger);

		meter.setModValue(t->lastValue);

		addAndMakeVisible(meter);

		this->setSize(512, 90);
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

		b.removeFromTop(5);

		auto r = b.removeFromTop(20);

		flashDot = r.removeFromLeft(20).toFloat();

		r.removeFromLeft(3);

		meter.setBounds(r);

		b.removeFromTop(5);

		dragger.setBounds(b);
	}

	void snex_timer::editor::paint(Graphics& g)
	{
		auto b = this->getLocalBounds().removeFromTop(28);

		auto ledArea = b.removeFromLeft(24).removeFromTop(24);

		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawRect(flashDot, 1.0f);

		g.setColour(Colours::white.withAlpha(alpha));
		g.fillRect(flashDot.reduced(2.0f));
	}

	void snex_timer::editor::timerCallback()
	{
		if (getObject() == nullptr)
		{
			stop();
			return;
		}

		float lastAlpha = alpha;

		auto& ui_led = getObject()->lastValue.changed;

		if (ui_led)
		{
			alpha = 1.0f;
		}
		else
			alpha = jmax(0.0f, alpha - 0.1f);

		if (lastAlpha != alpha)
			repaint();
	}

}

}

