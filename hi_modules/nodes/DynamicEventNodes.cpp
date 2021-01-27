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


class MidiDisplay : public ScriptnodeExtraComponent<DynamicMidiEventProcessor>,
					public SnexPopupEditor::Parent,
					public Value::Listener
{
public:

	using ObjectType = core::midi<DynamicMidiEventProcessor>;

	MidiDisplay(DynamicMidiEventProcessor* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<DynamicMidiEventProcessor>(t, updater),
		SnexPopupEditor::Parent(t),
		dragger(updater),
		editCodeButton("snex", this, f),
		modeSelector("Mode")
	{
		

		meter.setColour(VuMeter::backgroundColour, Colour(0xFF333333));
		meter.setColour(VuMeter::outlineColour, Colour(0x45ffffff));
		meter.setType(VuMeter::MonoHorizontal);
		meter.setColour(VuMeter::ledColour, Colours::grey);

		modeSelector.setColour(HiseColourScheme::ComponentBackgroundColour, Colours::transparentBlack);
		modeSelector.setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
		modeSelector.setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
		modeSelector.setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
		modeSelector.setColour(HiseColourScheme::ComponentTextColourId, Colours::white);

		modeSelector.onChange = [this]()
		{
			getObject()->modeValue.setValue(modeSelector.getText());
		};

		addAndMakeVisible(modeSelector);
		addAndMakeVisible(editCodeButton);

		t->modeValue.addListener(this);
		valueChanged(t->modeValue);

		editCodeButton.setLookAndFeel(&blaf);
		modeSelector.setLookAndFeel(&claf);

		modeSelector.addItemList(DynamicMidiEventProcessor::getModes(), 1);

		editCodeButton.addListener(this);

		this->addAndMakeVisible(meter);
		this->addAndMakeVisible(dragger);
		this->setSize(256, 128);
	}

	~MidiDisplay()
	{
		getObject()->modeValue.removeListener(this);
	}

	void valueChanged(Value& value) override
	{
		auto s = value.getValue().toString();

		if (DynamicMidiEventProcessor::getModes().contains(s))
		{
			modeSelector.setText(s, dontSendNotification);
		}

		editCodeButton.setVisible(s == "Custom");
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds();

		g.setColour(Colours::white.withAlpha(0.6f));
		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText("Normalised MIDI Value", b.removeFromTop(18).toFloat(), Justification::left);
		b.removeFromTop(meter.getHeight());

		auto row = b.removeFromTop(18).toFloat();

		g.drawText("Mode", row.removeFromLeft(128.0f), Justification::left);
	}

	

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new MidiDisplay(&static_cast<ObjectType*>(obj)->mType, updater);
	}

	void resized() override
	{
		auto b = this->getLocalBounds();
		b.removeFromTop(18);

		meter.setBounds(b.removeFromTop(24));
		b.removeFromTop(18);
		auto r = b.removeFromTop(28);
		editCodeButton.setBounds(r.removeFromRight(60).reduced(4));
		modeSelector.setBounds(r);
		b.removeFromTop(10);
		dragger.setBounds(b);
	}

	void timerCallback() override
	{
		if (this->getObject() == nullptr)
			return;

		meter.setPeak(this->getObject()->lastValue);
	}

	SnexPathFactory f;
	BlackTextButtonLookAndFeel blaf;
	GlobalHiseLookAndFeel claf;
	ComboBox modeSelector;
	HiseShapeButton editCodeButton;
	ModulationSourceBaseComponent dragger;
	VuMeter meter;
};





class TimerDisplay : public ScriptnodeExtraComponent<SnexEventTimer>,
					 public SnexPopupEditor::Parent
{
public:

	using ObjectType = timer_base<SnexEventTimer>;

	TimerDisplay(SnexEventTimer* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<SnexEventTimer>(t, updater),
		SnexPopupEditor::Parent(t),
		dragger(updater),
		editCodeButton("snex", this, f)
	{
		addAndMakeVisible(editCodeButton);
		editCodeButton.addListener(this);
		this->addAndMakeVisible(dragger);
		this->setSize(256, 50);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new TimerDisplay(&static_cast<ObjectType*>(obj)->tType, updater);
	}

	void resized() override
	{
		auto b = this->getLocalBounds();
		auto t = b.removeFromTop(28);
		editCodeButton.setBounds(t.removeFromRight(60).reduced(4));
		dragger.setBounds(b);
	}

	void paint(Graphics& g) override
	{
		auto b = this->getLocalBounds().removeFromTop(28);

		auto ledArea = b.removeFromLeft(24).removeFromTop(24);

		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawRect(ledArea, 1.0f);

		g.setColour(Colours::white.withAlpha(alpha));
		g.fillRect(ledArea.reduced(2.0f));
	}

	void timerCallback() override
	{
		if (getObject() == nullptr)
		{
			stop();
			return;
		}

		float lastAlpha = alpha;

		auto& ui_led = getObject()->ui_led;

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

	SnexPathFactory f;
	HiseShapeButton editCodeButton;
	
	float alpha = 0.0f;
	ModulationSourceBaseComponent dragger;
};

void DynamicMidiEventProcessor::prepare(PrepareSpecs ps)
{
	if (parentNode != nullptr)
	{
		auto pp = parentNode->getParentNode();
		bool found = true;

		bool isInMidiChain = false;

		while (pp != nullptr)
		{
			isInMidiChain |= pp->getValueTree()[PropertyIds::FactoryPath].toString().contains("midichain");
			pp = pp->getParentNode();
		}

		if (!isInMidiChain)
		{
			Error e;
			e.error = Error::NoMatchingParent;
			throw e;
		}
	}

	if(currentMode == Mode::Custom)
		SnexSource::prepare(ps);
}


struct SnexOscillatorDisplay : public ScriptnodeExtraComponent<SnexOscillator>,
							   public SnexPopupEditor::Parent,
							   public Value::Listener
{
	using ObjectType = snex_osc_base<SnexOscillator>;

	SnexOscillatorDisplay(SnexOscillator* o, PooledUIUpdater* u):
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

	~SnexOscillatorDisplay()
	{
		codeValue.removeListener(this);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u)
	{
		auto typed = reinterpret_cast<ObjectType*>(obj);
		return new SnexOscillatorDisplay(&typed->oscType, u);
	}

	void valueChanged(Value& v) override
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

	void timerCallback() override {};

	void resized() override
	{
		auto b = getLocalBounds();

		auto buttonBounds = b.removeFromRight(60).reduced(4);
		editCodeButton.setBounds(buttonBounds);

		pathBounds = b.toFloat();
	}

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

}
    
}

