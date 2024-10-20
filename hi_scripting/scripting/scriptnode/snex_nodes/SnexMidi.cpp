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

namespace midi_logic
{

dynamic::dynamic() :
	mode(PropertyIds::Mode, "Gate")
{}

void dynamic::prepare(PrepareSpecs ps)
{
    if(parentNode != nullptr)
        ScriptnodeExceptionHandler::validateMidiProcessingContext(parentNode);
}


void dynamic::initialise(NodeBase* n)
{
    parentNode = n;
	mode.initialise(n);
	mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::setMode), true);
}

void dynamic::setMode(Identifier id, var newValue)
{
	if (id == PropertyIds::Value)
	{
		auto v = newValue.toString();

		auto idx = getModes().indexOf(v);

		if (idx != -1)
			currentMode = (Mode)idx;
	}
}

bool dynamic::getMidiValue(HiseEvent& e, double& v)
{
	if (getMidiValueWrapped(e, v))
	{
		lastValue.setModValueIfChanged(v);
		return true;
	}

	return false;
}

bool dynamic::getMidiValueWrapped(HiseEvent& e, double& v)
{
    switch (currentMode)
    {
        case Mode::Gate:
            return gate<0>().getMidiValue(e, v);
        case Mode::Velocity:
            return velocity<0>().getMidiValue(e, v);
        case Mode::NoteNumber:
            return notenumber<0>().getMidiValue(e, v);
        case Mode::Frequency:
            return frequency<0>().getMidiValue(e, v);
        case Mode::Random:
            return random<0>().getMidiValue(e, v);
    }
    
    return false;
}

    
dynamic::editor::editor(dynamic* t, PooledUIUpdater* updater) :
	ScriptnodeExtraComponent<dynamic>(t, updater),
	dragger(updater),
	midiMode("Gate"),
	meter(updater)
{
	midiMode.initModes(dynamic::getModes(), t->parentNode);

	meter.setModValue(t->lastValue);

	addAndMakeVisible(midiMode);

	midiMode.mode.asJuceValue().addListener(this);
    
    auto v = midiMode.mode.asJuceValue();
    
	valueChanged(v);

	this->addAndMakeVisible(meter);
	this->addAndMakeVisible(dragger);
	this->setSize(256, 100 - 24 - 18);
}

void dynamic::editor::valueChanged(Value& value)
{
	
}

void dynamic::editor::paint(Graphics& g)
{
	auto b = getLocalBounds();

	g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText("Normalised MIDI Value", b.removeFromTop(18).toFloat(), Justification::left);
	b.removeFromTop(meter.getHeight());

	auto row = b.removeFromTop(18).toFloat();

	g.drawText("Mode", row.removeFromLeft(128.0f), Justification::left);
}

void dynamic::editor::resized()
{
    auto b = getLocalBounds();
	meter.setBounds(b.removeFromTop(8));
	b.removeFromTop(18);
	auto r = b.removeFromTop(24);

	midiMode.setBounds(r.removeFromLeft(100));
	r.removeFromLeft(UIValues::NodeMargin);
	dragger.setBounds(r);
	b.removeFromTop(10);
}

void dynamic::editor::timerCallback()
{
	if (auto c = findParentComponentOfClass<NodeComponent>())
	{
		auto c2 = c->header.colour;

		if (c2 == Colours::transparentBlack)
			c2 = Colours::white;

		meter.setColour(VuMeter::ledColour, c2);
	}
}



}

}

