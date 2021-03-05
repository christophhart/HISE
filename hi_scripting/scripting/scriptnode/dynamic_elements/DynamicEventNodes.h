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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;





struct VuMeterWithModValue : public VuMeter,
	public PooledUIUpdater::SimpleTimer
{
	VuMeterWithModValue(PooledUIUpdater* updater);

	void setModValue(ModValue& v)
	{
		valueToWatch = &v;
		start();
	}

	void timerCallback()
	{
		auto v = 0.0;

		if (valueToWatch->getChangedValue(v))
			setPeak(v);
	}

	ModValue* valueToWatch = nullptr;
};

#if REWRITE_SNEX_SOURCE_STUFF






#endif




#if REWRITE_SNEX_SOURCE_STUFF
struct SnexOscillatorDisplay : public ScriptnodeExtraComponent<SnexOscillator>
{
	using ObjectType = snex_osc_base<SnexOscillator>;

	SnexOscillatorDisplay(SnexOscillator* o, PooledUIUpdater* u);

	~SnexOscillatorDisplay();

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u);

	void valueChanged(Value& v) override;

	void timerCallback() override {};

	void resized() override;

	void paint(Graphics& g) override
	{
		GlobalHiseLookAndFeel::fillPathHiStyle(g, p, pathBounds.getWidth(), pathBounds.getHeight());
	}

	Path p;

	Value codeValue;
	Rectangle<float> pathBounds;

	SnexPathFactory f;
	HiseShapeButton editCodeButton;
};
#endif

}






