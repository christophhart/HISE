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

#if REWRITE_SNEX_SOURCE_STUFF


#endif

#if HISE_INCLUDE_SNEX

namespace core
{






#if REWRITE_SNEX_SOURCE_STUFF
SnexOscillatorDisplay::SnexOscillatorDisplay(SnexOscillator* o, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<SnexOscillator>(o, u),
	Parent(o),
	editCodeButton("snex", this, f)
{
	codeValue.referTo(o->expression.asJuceValue());
	addAndMakeVisible(editCodeButton);
	codeValue.addListener(this);
	setSize(256, 60);
	valueChanged(codeValue);
}

SnexOscillatorDisplay::~SnexOscillatorDisplay()
{
	codeValue.removeListener(this);
}

juce::Component* SnexOscillatorDisplay::createExtraComponent(void* obj, PooledUIUpdater* u)
{
	auto typed = reinterpret_cast<ObjectType*>(obj);
	return new SnexOscillatorDisplay(&typed->oscType, u);
}

void SnexOscillatorDisplay::valueChanged(Value& v)
{
	heap<float> buffer;
	buffer.setSize(200);
	dyn<float> d(buffer);

	for (auto& s : buffer)
		s = 0.0f;

	if (getObject()->isReady())
	{
		OscProcessData od;
		od.data.referTo(d);
		od.uptime = 0.0;
		od.delta = 1.0 / (double)buffer.size();
		od.voiceIndex = 0;

		getObject()->process(od);

		p.clear();
		p.startNewSubPath(0.0f, 0.0f);

		float i = 0.0f;

		for (auto& s : buffer)
		{
			FloatSanitizers::sanitizeFloatNumber(s);
			jlimit(-10.0f, 10.0f, s);
			p.lineTo(i, -1.0f * s);
			i += 1.0f;
		}

		p.lineTo(i, 0.0f);
		p.closeSubPath();

		if (p.getBounds().getHeight() > 0.0f && p.getBounds().getWidth() > 0.0f)
		{
			p.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), false);
		}
	}

	repaint();
}

void SnexOscillatorDisplay::resized()
{
	auto b = getLocalBounds();

	auto buttonBounds = b.removeFromRight(60).reduced(4);
	editCodeButton.setBounds(buttonBounds);

	pathBounds = b.toFloat();
}
#endif






}

#endif

VuMeterWithModValue::VuMeterWithModValue(PooledUIUpdater* updater) :
	SimpleTimer(updater)
{
	setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	setColour(VuMeter::outlineColour, Colour(0x45ffffff));
	setType(VuMeter::MonoHorizontal);
	setColour(VuMeter::ledColour, Colours::grey);
}

}

