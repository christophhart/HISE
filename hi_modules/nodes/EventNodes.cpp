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


namespace core
{

class MidiDisplay : public ScriptnodeExtraComponent<MidiDisplayProviderBase>
{
public:

	MidiDisplay(MidiDisplayProviderBase* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<MidiDisplayProviderBase>(t, updater),
		dragger(updater)
	{
		meter.setColour(VuMeter::backgroundColour, Colour(0xFF333333));
		meter.setColour(VuMeter::outlineColour, Colour(0x45ffffff));
		meter.setType(VuMeter::MonoHorizontal);
		meter.setColour(VuMeter::ledColour, Colours::grey);

		this->addAndMakeVisible(meter);
		this->addAndMakeVisible(dragger);

		this->setSize(256, 40);
	}

	static Component* createExtraComponent(ObjectType* obj, PooledUIUpdater* updater)
	{
		return new MidiDisplay(obj, updater);
	}

	void resized() override
	{
		auto b = this->getLocalBounds();
		meter.setBounds(b.removeFromTop(24));
		dragger.setBounds(b);
	}

	void timerCallback() override
	{
		if (this->getObject() == nullptr)
			return;

		meter.setPeak(this->getObject()->getDisplayValue());
	}

	ModulationSourceBaseComponent dragger;
	VuMeter meter;
};

template <int V>
float scriptnode::core::MidiSourceNode<V>::getDisplayValue() const
{
	return (float)modValue.getCurrentOrFirst().getModValue();
}


template <int V>
void MidiSourceNode<V>::createParameters(Array<ParameterData>& data)
{
	ParameterData d("Mode");

	d.setParameterValueNames({ "Gate", "Velocity", "NoteNumber", "Frequency" });
	d.defaultValue = 0.0;
	d.db = BIND_MEMBER_FUNCTION_1(MidiSourceNode::setMode);

	data.add(std::move(d));

	scriptFunction.init(nullptr, nullptr);
}

template <int V>
bool MidiSourceNode<V>::handleModulation(double& value)
{
	return modValue.get().getChangedValue(value);
}

template <int V>
void MidiSourceNode<V>::handleHiseEvent(HiseEvent& e)
{
	bool thisChanged = false;
	double thisModValue = 0.0;

	switch (currentMode)
	{
	case Mode::Gate:
	{
		if (e.isNoteOnOrOff())
		{
			thisModValue = e.isNoteOn() ? 1.0 : 0.0;
			thisChanged = true;
		}

		break;
	}
	case Mode::Velocity:
	{
		if (e.isNoteOn())
		{
			thisModValue = (double)e.getVelocity() / 127.0;
			thisChanged = true;
		}

		break;
	}
	case Mode::NoteNumber:
	{
		if (e.isNoteOn())
		{
			thisModValue = (double)e.getNoteNumber() / 127.0;
			thisChanged = true;
		}

		break;
	}
	case Mode::Frequency:
	{
		if (e.isNoteOn())
		{
			thisModValue = (e.getFrequency() - 20.0) / 19980.0;
			thisChanged = true;
		}

		break;
	}
	}

	if (thisChanged)
	{
		
		modValue.get().setModValue(scriptFunction.callWithDouble(thisModValue));
	}
		
}

template <int V>
void MidiSourceNode<V>::prepare(PrepareSpecs sp)
{
	modValue.prepare(sp);
}


template <int V>
void MidiSourceNode<V>::initialise(NodeBase* n)
{
	scriptFunction.init(n, this);
}

DEFINE_EXTERN_NODE_TEMPIMPL(MidiSourceNode);


class TimerDisplay : public ScriptnodeExtraComponent<TimerDisplayProviderBase>
{
public:

	TimerDisplay(TimerDisplayProviderBase* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<TimerDisplayProviderBase>(t, updater),
		dragger(updater)
	{
		this->addAndMakeVisible(dragger);

		this->setSize(256, 40);
	}

	static Component* createExtraComponent(ObjectType* obj, PooledUIUpdater* updater)
	{
		return new TimerDisplay(obj, updater);
	}

	void resized() override
	{
		auto b = this->getLocalBounds();
		b.removeFromTop(24);
		dragger.setBounds(b);
	}

	void paint(Graphics& g) override
	{
		auto b = this->getLocalBounds().removeFromTop(24);

		g.setColour(Colours::white.withAlpha(alpha));
		g.fillEllipse(b.withSizeKeepingCentre(20, 20).toFloat());
	}

	void timerCallback() override
	{
		if (getObject() == nullptr)
		{
			stop();
			return;
		}

		float lastAlpha = alpha;

		auto& ui_led = getObject()->getActiveFlag();

		if (ui_led)
		{
			alpha = 1.0f;
			ui_led = false;
		}
		else
			alpha = jmax(0.0f, alpha - 0.1f);

		if (lastAlpha != alpha)
			repaint();
	}

	float alpha = 0.0f;
	ModulationSourceBaseComponent dragger;
};

template <int NV>
TimerNode<NV>::TimerNode():
	fillMode(PropertyIds::FillMode, true)
{

}


template <int NV>
bool& TimerNode<NV>::getActiveFlag()
{
	return ui_led;
}


template <int NV>
void TimerNode<NV>::setInterval(double timeMs)
{
	auto newTime = roundToInt(timeMs * 0.001 * sr);

	if (t.isMonophonicOrInsideVoiceRendering())
	{
		t.get().samplesBetweenCallbacks = newTime;
	}
	else
	{
		for(auto& ti: t)
			ti.samplesBetweenCallbacks = newTime;
	}
}

template <int NV>
void TimerNode<NV>::setActive(double value)
{
	bool thisActive = value > 0.5;

	if (t.isMonophonicOrInsideVoiceRendering())
	{
		auto& thisInfo = t.get();

		if (thisInfo.active != thisActive)
		{
			thisInfo.active = thisActive;
			thisInfo.reset();
		}
	}
	else
	{
		for (auto& ti : t)
		{
			ti.active = thisActive;
			ti.reset();
		}
	}

}

template <int NV>
void TimerNode<NV>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData d("Interval");

		d.range = { 0.0, 2000.0, 0.1 };
		d.defaultValue = 500.0;
		d.db = BIND_MEMBER_FUNCTION_1(TimerNode::setInterval);

		data.add(std::move(d));
	}

	{
		ParameterData d("Active");

		d.range = { 0.0, 1.0, 1.0 };
		d.defaultValue = 500.0;
		d.db = BIND_MEMBER_FUNCTION_1(TimerNode::setActive);

		data.add(std::move(d));
	}

	scriptFunction.init(nullptr, nullptr);
	fillMode.init(nullptr, nullptr);
}

template <int NV>
bool TimerNode<NV>::handleModulation(double& value)
{
	return modValue.get().getChangedValue(value);
}

template <int NV>
void TimerNode<NV>::processFirstChannel(snex::Types::ProcessDataFix<1>& d)
{
	auto& thisInfo = t.get();

	if (!thisInfo.active)
		return;

	const int numSamples = d.getNumSamples();

	if (numSamples < thisInfo.samplesLeft)
	{
		thisInfo.samplesLeft -= numSamples;

		if (fillMode.getValue())
		{
			for (auto ch : d)
				FloatVectorOperations::fill(ch.getRawWritePointer(), (float)modValue.get().getModValue(), numSamples);
		}
	}
	else
	{
		const int numRemaining = numSamples - thisInfo.samplesLeft;

		if (fillMode.getValue())
		{
			for (auto ch : d)
				FloatVectorOperations::fill(ch.getRawWritePointer(), (float)modValue.get().getModValue(), numRemaining);
		}

		auto newValue = scriptFunction.callWithDouble(0.0);
		modValue.get().setModValue(newValue);

		ui_led = true;

		const int numAfter = numSamples - numRemaining;

		if (fillMode.getValue())
		{
			for (auto ch : d)
				FloatVectorOperations::fill(ch.getRawWritePointer() + numRemaining, (float)newValue, numAfter);
		}
		
		thisInfo.samplesLeft = thisInfo.samplesBetweenCallbacks + numRemaining;
	}
}

template <int NV>
void TimerNode<NV>::reset()
{
	t.get().reset();
}

template <int NV>
void TimerNode<NV>::prepare(PrepareSpecs ps)
{
	sr = ps.sampleRate;

	t.prepare(ps);
	modValue.prepare(ps);
}

template <int NV>
void TimerNode<NV>::initialise(NodeBase* n)
{
	scriptFunction.init(n, this);
	fillMode.init(n, this);
}

DEFINE_EXTERN_NODE_TEMPIMPL(TimerNode);
}
    
}

