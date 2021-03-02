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

namespace midi_logic
{



struct dynamic : public SnexSource
{
	using NodeType = control::midi<dynamic>;

	enum class Mode
	{
		Gate = 0,
		Velocity,
		NoteNumber,
		Frequency,
		Custom
	};

	static StringArray getModes()
	{
		return { "Gate", "Velocity", "NoteNumber", "Frequency", "Custom" };
	}

	String getEmptyText() const override;

	dynamic();;

	class editor : public ScriptnodeExtraComponent<dynamic>,
		public SnexPopupEditor::Parent,
		public Value::Listener
	{
	public:

		editor(dynamic* t, PooledUIUpdater* updater);

		~editor()
		{
			midiMode.mode.asJuceValue().removeListener(this);
		}

		void valueChanged(Value& value) override;

		void paint(Graphics& g) override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<NodeType*>(obj);
			return new editor(&typed->mType, updater);
		}

		void resized() override;

		void timerCallback() override
		{
			jassertfalse;
		}

		SnexPathFactory f;
		BlackTextButtonLookAndFeel blaf;
		GlobalHiseLookAndFeel claf;

		ComboBoxWithModeProperty midiMode;

		HiseShapeButton editCodeButton;
		ModulationSourceBaseComponent dragger;
		VuMeterWithModValue meter;
	};

	void prepare(PrepareSpecs ps);

	void initialise(NodeBase* n);

	void codeCompiled() override;

	void setMode(Identifier id, var newValue);

	bool getMidiValue(HiseEvent& e, double& v);

	bool getMidiValueWrapped(HiseEvent& e, double& v);

	ModValue lastValue;
	Mode currentMode = Mode::Gate;
	NodePropertyT<String> mode;
	snex::jit::FunctionData f;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};


}

namespace control
{



struct snex_timer: public SnexSource
{
	using NodeType = control::timer_impl<1, snex_timer>;

	snex_timer():
		SnexSource()
	{}

	void codeCompiled() override;

	String getEmptyText() const override;

	double getTimerValue();

	ModValue lastValue;
	snex::jit::FunctionData f;

	class editor : public ScriptnodeExtraComponent<snex_timer>,
		public SnexPopupEditor::Parent
	{
	public:

		editor(snex_timer* t, PooledUIUpdater* updater);

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater);

		void resized() override;

		void paint(Graphics& g) override;

		void timerCallback() override;

		VuMeterWithModValue meter;
		SnexPathFactory f;
		HiseShapeButton editCodeButton;

		float alpha = 0.0f;
		ModulationSourceBaseComponent dragger;
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(snex_timer);
};


}


namespace waveshapers
{


struct dynamic : public SnexSource
{
	SN_GET_SELF_AS_OBJECT(dynamic);

	using NodeType = core::waveshaper<dynamic>;

	dynamic()
	{

	}

	void initialise(NodeBase* n) override
	{
		SnexSource::initialise(n);
	}

	void codeCompiled() override;

	template <int P> void setParameter(double v)
	{
		pValues[P].setModValueIfChanged(v);

		if (ok)
			parameters[P].callVoidUnchecked(v);
	}

	void process(ProcessDataDyn& data)
	{
		if (ok)
		{
			processFunction.callVoid(&data);
			FloatSanitizers::sanitizeArray(data[0]);
		}
			
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		if (ok)
		{
			processFrameFunction.callVoid(d.begin());
			FloatSanitizers::sanitizeArray(d);
		}
	}

	bool preprocessCode(String& c) override;

	String getEmptyText() const override;

	class editor : public ScriptnodeExtraComponent<dynamic>,
		public SnexPopupEditor::Parent,
		public WaveformComponent::Broadcaster
	{
	public:

		editor(dynamic* t, PooledUIUpdater* updater);

		~editor();

		snex::Types::span<float, 128> tData;

		void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;

		void timerCallback() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater);

		void resized() override;

		SnexPathFactory f;
		BlackTextButtonLookAndFeel blaf;
		GlobalHiseLookAndFeel claf;

		WaveformComponent waveform;

		HiseShapeButton editCodeButton;
	};

	bool ok = false;
	FunctionData processFunction;
	FunctionData processFrameFunction;
	FunctionData parameters[2];

	ModValue pValues[2];

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};


}

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






