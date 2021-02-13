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

namespace core
{

struct SnexOscillator : public SnexSource
{
	SnexOscillator()
	{

	}

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("snex_osc"); };

	bool setupCallbacks(snex::JitObject& obj) override
	{
		tickFunction = obj["tick"];
		processFunction = obj["process"];

		auto tickMatches = tickFunction.returnType == Types::ID::Float && tickFunction.args[0].typeInfo.getType() == Types::ID::Double;
		auto processMatches = processFunction.returnType == Types::ID::Void;

		return processFunction.function != nullptr && tickMatches && processMatches && processFunction.function != nullptr;
	}

	String getEmptyText(const Identifier& id) const override
	{
		String s;
		s << "float tick(double uptime)\n";
		s << "{\n    return 0.0f;\n}\n";

		s << "void process(OscProcessData& d)\n";
		s << "{\n";
		s << "    for (auto& s : d.data)\n";
		s << "    {\n";
		s << "        s = tick(d.uptime);\n";
		s << "        d.uptime += d.delta;\n";
		s << "    }\n";
		s << "}\n";

		addDefaultParameterFunction(s);

		return s;
	}

	void initialise(NodeBase* n)
	{
		SnexSource::initialise(n);
	}

	float tick(double uptime)
	{
		jassert(isReady());
		auto s = tickFunction.call<float>(uptime);
		return s;
	}

	void process(OscProcessData& d)
	{
		jassert(isReady());
		processFunction.callVoid(&d);
	}

	FunctionData tickFunction;
	FunctionData processFunction;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexOscillator);
};

template <typename T> struct snex_osc_base
{
	void initialise(NodeBase* n)
	{
		oscType.initialise(n);
	}

	T oscType;
};

template <int NV, typename T> struct snex_osc_impl: snex_osc_base<T>
{
	enum class Parameters
	{
		Frequency,
		PitchMultiplier
	};

	static constexpr int NumVoices = NV;

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, snex_osc_impl);
		DEF_PARAMETER(PitchMultiplier, snex_osc_impl);
	}

	SET_HISE_POLY_NODE_ID("snex_osc");
	SN_GET_SELF_AS_OBJECT(snex_osc_impl);

	void prepare(PrepareSpecs ps)
	{
		sampleRate = ps.sampleRate;
		voiceIndex = ps.voiceIndex;
		oscData.prepare(ps);
		reset();
	}

	void reset()
	{
		for (auto& o : oscData)
			o.reset();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (this->oscType.isReady())
		{
			auto& thisData = oscData.get();
			auto uptime = thisData.tick();
			data[0] += this->oscType.tick(thisData.tick());
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (this->oscType.isReady())
		{
			auto& thisData = oscData.get();

			OscProcessData op;

			op.data.referToRawData(data.getRawDataPointers()[0], data.getNumSamples());
			op.uptime = thisData.uptime;
			op.delta = thisData.uptimeDelta * thisData.multiplier;
			op.voiceIndex = voiceIndex->getVoiceIndex();
			
			this->oscType.process(op);
			thisData.uptime += op.delta * (double)data.getNumSamples();
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			setFrequency(e.getFrequency());
	}

	void setFrequency(double newValue)
	{
		if (sampleRate > 0.0)
		{
			auto cyclesPerSecond = newValue;
			auto cyclesPerSample = cyclesPerSecond / sampleRate;

			for (auto& o : oscData)
				o.uptimeDelta = cyclesPerSample;
		}
	}

	void setPitchMultiplier(double newMultiplier)
	{
		newMultiplier = jlimit(0.01, 100.0, newMultiplier);

		for (auto& o : oscData)
			o.multiplier = newMultiplier;
	}

	double sampleRate = 0.0;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(snex_osc_impl, Frequency);
			p.setRange({ 20.0, 20000.0, 0.1 });
			p.setSkewForCentre(1000.0);
			p.setDefaultValue(220.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(snex_osc_impl, PitchMultiplier);
			p.setRange({ 1.0, 16.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
	}

	PolyHandler* voiceIndex = nullptr;
	PolyData<OscData, NumVoices> oscData;
};

template <typename OscType> using snex_osc = snex_osc_impl<1, OscType>;
template <typename OscType> using snex_osc_poly = snex_osc_impl<NUM_POLYPHONIC_VOICES, OscType>;

struct SnexComplexDataDisplay : public Component,
								public SnexSource::SnexSourceListener
{
	SnexComplexDataDisplay(SnexSource* s):
		source(s)
	{
		setName("Complex Data Editor");
		source->addCompileListener(this);
		rebuildEditors();
	}

	~SnexComplexDataDisplay()
	{
		source->removeCompileListener(this);
	}

	void wasCompiled(bool ok) {};

	void complexDataAdded(snex::ExternalData::DataType , int )
	{
		rebuildEditors();
	}

	void rebuildEditors();

	void resized()
	{
		auto b = getLocalBounds();

		for (auto e : editors)
		{
			e->setBounds(b.removeFromTop(100));
		}
	}

	OwnedArray<Component> editors;

	WeakReference<SnexSource> source;
};

struct SnexMenuBar : public Component,
					 public ButtonListener,
					 public ComboBox::Listener,
					 public SnexSource::SnexSourceListener,
					 public snex::ui::WorkbenchManager::WorkbenchChangeListener
{
	struct Factory : public PathFactory
	{
		Path createPath(const String& p) const override;

		String getId() const override { return {}; }
	} f;

	struct ComplexDataPopupButton : public Button
	{
		ComplexDataPopupButton(SnexSource* s);

		String getText()
		{
			bool containsSomething = false;

			String s;

			ExternalData::forEachType([this, &s, &containsSomething](ExternalData::DataType t)
			{
				auto numObjects = source->getComplexDataHandler().getNumDataObjects(t);
				containsSomething |= numObjects > 0;
				s << ExternalData::getDataTypeName(t).substring(0, 1);
				s << ":" << String(numObjects);
				s << ", ";
			});

			setEnabled(containsSomething);

			return s.upToLastOccurrenceOf(", ", false, false);
		}

		void update(ValueTree, bool)
		{
			text = getText();
			repaint();
		}

		void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
		{
			g.setColour(Colours::white);
			g.drawText(text, getLocalBounds().toFloat(), Justification::centred);
		}

		String text;
		SnexSource* source;
		ValueTree t;
		valuetree::RecursiveTypedChildListener l;
	};

	HiseShapeButton newButton;
	ComboBox classSelector;
	HiseShapeButton popupButton;
	HiseShapeButton editButton;
	HiseShapeButton addButton;
	HiseShapeButton deleteButton;
	ComplexDataPopupButton cdp;

	SnexMenuBar(SnexSource* s) :
		newButton("new", this, f),
		popupButton("popup", this, f),
		editButton("edit", this, f),
		addButton("add", this, f),
		deleteButton("delete", this, f),
		source(s),
		cdp(s)
	{
		s->addCompileListener(this);

		addAndMakeVisible(newButton);
		addAndMakeVisible(classSelector);
		addAndMakeVisible(popupButton);
		addAndMakeVisible(editButton);
		classSelector.setLookAndFeel(&plaf);
		classSelector.addListener(this);

		addAndMakeVisible(addButton);
		addAndMakeVisible(deleteButton);
		addAndMakeVisible(cdp);

		editButton.setToggleModeWithColourChange(true);
		
		rebuildComboBoxItems();
		refreshButtonState();

		auto wb = static_cast<snex::ui::WorkbenchManager*>(source->getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());
		wb->addListener(this);
		workbenchChanged(wb->getCurrentWorkbench());

		GlobalHiseLookAndFeel::setDefaultColours(classSelector);
	}

	void wasCompiled(bool ok) override
	{
		iconColour = ok ? Colours::green : Colours::red;
		iconColour = iconColour.withSaturation(0.2f).withAlpha(0.8f);
		repaint();
	}

	~SnexMenuBar()
	{
		auto wb = static_cast<snex::ui::WorkbenchManager*>(source->getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());
		wb->removeListener(this);

		source->removeCompileListener(this);
	}
	
	void workbenchChanged(snex::ui::WorkbenchData::Ptr newWb)
	{
		editButton.setToggleStateAndUpdateIcon(source->getWorkbench() == newWb && newWb != nullptr, true);
	}

	void complexDataAdded(snex::ExternalData::DataType t, int index) override
	{

	}

	void rebuildComboBoxItems()
	{
		classSelector.clear(dontSendNotification);
		classSelector.addItemList(source->getAvailableClassIds(), 1);

		if (auto w = source->getWorkbench())
			classSelector.setText(w->getInstanceId().toString(), dontSendNotification);
	}

	void refreshButtonState()
	{
		bool shouldBeEnabled = source->getWorkbench() != nullptr;
		
		addButton.setEnabled(shouldBeEnabled);
		editButton.setEnabled(shouldBeEnabled);
		deleteButton.setEnabled(shouldBeEnabled);
		popupButton.setEnabled(shouldBeEnabled);
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		source->setClass(classSelector.getText());
		refreshButtonState();
	}

	void buttonClicked(Button* b);

	void paint(Graphics& g) override
	{
		g.setColour(Colours::black.withAlpha(0.2f));
		g.fillRoundedRectangle(getLocalBounds().toFloat(), 1.0f);
		g.setColour(iconColour);
		g.fillPath(snexIcon);
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(1);
		auto h = getHeight();

		newButton.setBounds(b.removeFromLeft(h));
		classSelector.setBounds(b.removeFromLeft(128));
		popupButton.setBounds(b.removeFromLeft(h));
		editButton.setBounds(b.removeFromLeft(h));

		b.removeFromLeft(10);
		addButton.setBounds(b.removeFromLeft(h));
		deleteButton.setBounds(b.removeFromLeft(h));
		cdp.setBounds(b.removeFromLeft(90));

		snexIcon = f.createPath("snex");
		f.scalePath(snexIcon, getLocalBounds().removeFromRight(80).toFloat().reduced(2.0f));
	};

	PopupLookAndFeel plaf;
	Path snexIcon;
	Colour iconColour = Colours::white.withAlpha(0.2f);

	WeakReference<SnexSource> source;

	snex::ui::WorkbenchData::WeakPtr rootBench;

};

struct NewSnexOscillatorDisplay : public ScriptnodeExtraComponent<SnexOscillator>,
								  public SnexSource::SnexSourceListener
{
	NewSnexOscillatorDisplay(SnexOscillator* osc, PooledUIUpdater* updater):
		ScriptnodeExtraComponent<SnexOscillator>(osc, updater),
		menuBar(osc)
	{
		addAndMakeVisible(menuBar);
		setSize(512, 144);
		getObject()->addCompileListener(this);
		stop();
	}

	~NewSnexOscillatorDisplay()
	{
		getObject()->removeCompileListener(this);
	}

	void complexDataAdded(snex::ExternalData::DataType t, int index) override
	{
		
	}

	void wasCompiled(bool ok)
	{
		if (ok)
		{
			errorMessage = {};

			heap<float> buffer;
			buffer.setSize(200);
			dyn<float> d(buffer);

			for (auto& s : buffer)
				s = 0.0f;

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

			p.lineTo(i-1.0f, 0.0f);
			p.closeSubPath();

			if (p.getBounds().getHeight() > 0.0f && p.getBounds().getWidth() > 0.0f)
			{
				p.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), false);
			}

			repaint();
		}
		else
		{
			p = {};
			errorMessage = getObject()->getWorkbench()->getLastResult().compileResult.getErrorMessage();
			repaint();
		}
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRect(pathBounds, 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("SNEX Oscillator", pathBounds.reduced(3.0f), Justification::topRight);

		if (errorMessage.isNotEmpty())
		{
			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(errorMessage, pathBounds, Justification::centred);
		}
		else if (!p.getBounds().isEmpty())
		{
			
			GlobalHiseLookAndFeel::fillPathHiStyle(g, p, (int)pathBounds.getWidth(), (int)pathBounds.getHeight());
		}
	}

	void timerCallback() override
	{
	}

	void resized() override
	{
		auto t = getLocalBounds();
		menuBar.setBounds(t.removeFromTop(24));
		t.removeFromTop(20);

		pathBounds = t.reduced(2).toFloat();
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u)
	{
		return new NewSnexOscillatorDisplay(static_cast<SnexOscillator*>(obj), u);
	}

	SnexMenuBar menuBar;
	Path p;
	String errorMessage;
	Rectangle<float> pathBounds;
};


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

}




