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

using namespace snex;

String dynamic::getEmptyText(const Identifier& id) const
{
#if HISE_INCLUDE_SNEX
	cppgen::Base c(cppgen::Base::OutputType::AddTabs);

	

	cppgen::Struct s(c, id, {}, {TemplateParameter(NamespacedIdentifier("NumVoices"), 0, false)});

	addSnexNodeId(c, id);

	c << "void prepare(PrepareSpecs ps)";

	{
		cppgen::StatementBlock sb(c);
	}
	
	c.addComment("Return 1 and set value if you want to process this event", snex::cppgen::Base::CommentType::Raw);
	c << "int getMidiValue(HiseEvent& e, double& value)";
	{
		cppgen::StatementBlock sb(c);
		c << "return 0;";
	}

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

dynamic::dynamic() :
	OptionalSnexSource(),
	callbacks(*this, object),
	mode(PropertyIds::Mode, "Gate")
{
	setCallbackHandler(&callbacks);
}



void dynamic::prepare(PrepareSpecs ps)
{
	ScriptnodeExceptionHandler::validateMidiProcessingContext(getParentNode());

	if (currentMode == Mode::Custom)
		callbacks.prepare(ps);
}


void dynamic::initialise(NodeBase* n)
{
	OptionalSnexSource::initialise(n);

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

	case Mode::Custom:
	{
#if HISE_INCLUDE_SNEX
		HiseEvent* eptr = &e;
		double* s = &v;
		return callbacks.getMidiValue(eptr, s);
#else
		jassertfalse;
		return 1.0f;
#endif
	}
	}

	return false;
}



#if HISE_INCLUDE_SNEX
dynamic::editor::editor(dynamic* t, PooledUIUpdater* updater) :
	ScriptnodeExtraComponent<dynamic>(t, updater),
	menuBar(t),
	dragger(updater),
	midiMode("Gate"),
	meter(updater)
{
	t->addCompileListener(this);

	midiMode.initModes(dynamic::getModes(), t->getParentNode());

	meter.setModValue(t->lastValue);

	
	addAndMakeVisible(midiMode);

	midiMode.mode.asJuceValue().addListener(this);
    
    auto v = midiMode.mode.asJuceValue();
    
	valueChanged(v);

	addAndMakeVisible(menuBar);
	this->addAndMakeVisible(meter);
	this->addAndMakeVisible(dragger);
	this->setSize(256, 100);
}

void dynamic::editor::valueChanged(Value& value)
{
	
}

void dynamic::editor::paint(Graphics& g)
{
	auto b = getLocalBounds();

	g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_BOLD_FONT());

	b.removeFromTop(menuBar.getHeight());

	g.drawText("Normalised MIDI Value", b.removeFromTop(18).toFloat(), Justification::left);
	b.removeFromTop(meter.getHeight());

	auto row = b.removeFromTop(18).toFloat();

	g.drawText("Mode", row.removeFromLeft(128.0f), Justification::left);
}

void dynamic::editor::resized()
{
	auto b = this->getLocalBounds();
	menuBar.setBounds(b.removeFromTop(24));

	b.removeFromTop(18);

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

	auto snexEnabled = getObject()->currentMode == Mode::Custom;
	menuBar.setAlpha(snexEnabled ? 1.0f : 0.1f);
}


void dynamic::CustomMidiCallback::reset()
{
	SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

	ok = false;
	prepareFunction = {};
	midiFunction = {};
}

Result dynamic::CustomMidiCallback::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
{
	auto newGetMidiFunc = getFunctionAsObjectCallback("getMidiValue");
	auto newPrepareFunc = getFunctionAsObjectCallback("prepare");

	auto r = newGetMidiFunc.validateWithArgs("int", {"HiseEvent&", "double&"});

	if (r.wasOk())
		r = newPrepareFunc.validateWithArgs("void", {"PrepareSpecs"});

	{
		SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
		ok = r.wasOk();
		std::swap(midiFunction, newGetMidiFunc);
		std::swap(prepareFunction, newPrepareFunc);
	}

	return r;
}
#endif

}

}

