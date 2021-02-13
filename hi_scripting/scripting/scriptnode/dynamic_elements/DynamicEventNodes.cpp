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

namespace midi_logic
{

	String dynamic::getEmptyText() const
	{
		String c;

		c << "void prepare(PrepareSpecs ps)\n";
		c << "{\n\n    \n}\n\n";
		c << "int getMidiValue(HiseEvent& e, double& value)\n";
		c << "{\n    return 0;\n}\n";

		return c;
	}

	dynamic::dynamic() :
		SnexSource(),
		mode(PropertyIds::Mode, "Gate")
	{

	}

void dynamic::prepare(PrepareSpecs ps)
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

	if (currentMode == Mode::Custom)
		SnexSource::prepare(ps);
}


void dynamic::initialise(NodeBase* n)
{
	SnexSource::initialise(n);

	mode.initialise(n);
	mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::setMode), true);
}

void dynamic::codeCompiled()
{
	f = obj["getMidiValue"];
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
		return gate().getMidiValue(e, v);
	case Mode::Velocity:
		return velocity().getMidiValue(e, v);
	case Mode::NoteNumber:
		return notenumber().getMidiValue(e, v);
	case Mode::Frequency:
		return frequency().getMidiValue(e, v);
	case Mode::Custom:
	{
		HiseEvent* eptr = &e;
		double* s = &v;

		if (f.function != nullptr)
		{
			auto ok = (bool)f.call<int>(eptr, s);
			return ok;
		}

		break;
	}
	}

	return false;
}




dynamic::editor::editor(dynamic* t, PooledUIUpdater* updater) :
	ScriptnodeExtraComponent<dynamic>(t, updater),
	SnexPopupEditor::Parent(t),
	dragger(updater),
	editCodeButton("snex", this, f),
	midiMode("Gate"),
	meter(updater)
{
	midiMode.initModes(dynamic::getModes(), t->parentNode);

	meter.setModValue(t->lastValue);

	addAndMakeVisible(midiMode);
	addAndMakeVisible(editCodeButton);

	midiMode.mode.asJuceValue().addListener(this);
	valueChanged(midiMode.mode.asJuceValue());

	editCodeButton.setLookAndFeel(&blaf);
	editCodeButton.addListener(this);

	this->addAndMakeVisible(meter);
	this->addAndMakeVisible(dragger);
	this->setSize(256, 128);
	stop();
}

void dynamic::editor::valueChanged(Value& value)
{
	auto s = value.getValue().toString();
	editCodeButton.setVisible(s == "Custom");
}

void dynamic::editor::paint(Graphics& g)
{
	auto b = getLocalBounds();

	g.setColour(Colours::white.withAlpha(0.6f));
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText("Normalised MIDI Value", b.removeFromTop(18).toFloat(), Justification::left);
	b.removeFromTop(meter.getHeight());

	auto row = b.removeFromTop(18).toFloat();

	g.drawText("Mode", row.removeFromLeft(128.0f), Justification::left);
}

void dynamic::editor::resized()
{
	auto b = this->getLocalBounds();
	b.removeFromTop(18);

	meter.setBounds(b.removeFromTop(24));
	b.removeFromTop(18);
	auto r = b.removeFromTop(28);
	editCodeButton.setBounds(r.removeFromRight(60).reduced(4));
	midiMode.setBounds(r);
	b.removeFromTop(10);
	dragger.setBounds(b);
}

}

namespace waveshapers
{


void dynamic::codeCompiled()
{
	if (parentNode != nullptr)
	{
		int nc = parentNode->getNumChannelsToProcess();

		ok = false;

		processFunction = obj["process"];
		processFrameFunction = obj["processFrame"];

		parameters[0] = obj["setParam1"];
		parameters[1] = obj["setParam2"];

		ok = processFrameFunction.function != nullptr &&
			processFunction.function != nullptr &&
			parameters[0].function != nullptr &&
			parameters[1].function != nullptr;
	}
}

bool dynamic::preprocessCode(String& c)
{
	if (parentNode != nullptr)
	{
		int nc = parentNode->getNumChannelsToProcess();

		using namespace snex::cppgen;

		Base b(Base::OutputType::AddTabs);

		String def1, def2, def3, def4;
		def1 << "void process(ProcessData<" << String(nc) << ">& data)";
		def2 << "void processFrame(span<float, " << String(nc) << ">& data)";

		def3 << "void setParam1(double v)";
		def4 << "void setParam2(double v)";

		b << def1;
		{
			StatementBlock fb(b);
			b << "for(auto& ch: data)";
			{
				StatementBlock lb(b);
				b << "for(auto& s: data.toChannelData(ch))";
				b << "s = getSample(s);";
			}
		}

		b << def2;
		{
			StatementBlock lb(b);
			b << "for(auto& s: data)";
			b << "s = getSample(s);";
		}

		b << def3;
		{
			StatementBlock fb(b);
			b << "setParameter<0>(v);";
		}

		b << def4;
		{
			StatementBlock fb(b);
			b << "setParameter<1>(v);";
		}

		c << b.toString();

		DBG(c);
	}

	return true;
}

String dynamic::getEmptyText() const
{
	using namespace snex::cppgen;

	Base b(Base::OutputType::AddTabs);

	b.addComment("Implement the Waveshaper here...", snex::cppgen::Base::CommentType::RawWithNewLine);
	b << "float getSample(float input)";

	{
		StatementBlock body(b);
		b << "return input;";
	}

	b.addComment("Use this function to setup the parameters", snex::cppgen::Base::CommentType::RawWithNewLine);
	b << "template <int P> void setParameter(double value)";

	{
		StatementBlock body(b);
		b << "if(P == 1)";
		{
			StatementBlock tb(b);
			b.addComment("Insert the logic for the first parameter", snex::cppgen::Base::CommentType::Raw);
		}
		b << "else";
		{
			StatementBlock fb(b);
			b.addComment("Insert the logic for the second parameter", snex::cppgen::Base::CommentType::Raw);
		}
	}

	return b.toString();
}

dynamic::editor::editor(dynamic* t, PooledUIUpdater* updater) :
	ScriptnodeExtraComponent<dynamic>(t, updater),
	SnexPopupEditor::Parent(t),
	editCodeButton("snex", this, f),
	waveform(nullptr, 0)
{
	addAndMakeVisible(editCodeButton);

	addAndMakeVisible(waveform);
	addWaveformListener(&waveform);

	editCodeButton.setLookAndFeel(&blaf);
	editCodeButton.addListener(this);

	this->setSize(256, 128);
}

dynamic::editor::~editor()
{

}

void dynamic::editor::getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue)
{
	for (int i = 0; i < 128; i++)
		tData[i] = 2.0f * (float)i / 127.0f - 1.0f;


	float* x = tData.begin();

	ProcessDataDyn pd(&x, 128, 1);

	getObject()->process(pd);

	*tableValues = tData.begin();
	numValues = 128;
	normalizeValue = 1.0f;
}

void dynamic::editor::timerCallback()
{
	double v1, v2 = 0.0;

	if (getObject()->pValues[0].getChangedValue(v1) || getObject()->pValues[1].getChangedValue(v2))
		Broadcaster::updateData();
}

juce::Component* dynamic::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
{
	auto typed = static_cast<NodeType*>(obj);
	return new editor(&typed->shaper, updater);
}

void dynamic::editor::resized()
{
	auto b = this->getLocalBounds();

	editCodeButton.setBounds(b.removeFromRight(60).reduced(4));
	waveform.setBounds(b);
}

}

namespace control
{












void snex_timer::codeCompiled()
{
	f = obj["getTimerValue"];
}

String snex_timer::getEmptyText() const
{
	String s;

	s << "double getTimerValue()\n";
	s << "{\n    return 0.0;\n}\n";
	return s;
}

double snex_timer::getTimerValue()
{
	if (f.function != nullptr)
	{
		auto v = f.call<double>();
		lastValue.setModValue(v);
		return v;
	}

	return 0.0;
}

snex_timer::editor::editor(snex_timer* t, PooledUIUpdater* updater) :
	ScriptnodeExtraComponent<snex_timer>(t, updater),
	SnexPopupEditor::Parent(t),
	dragger(updater),
	editCodeButton("snex", this, f),
	meter(updater)
{
	addAndMakeVisible(editCodeButton);
	editCodeButton.addListener(this);
	this->addAndMakeVisible(dragger);

	meter.setModValue(t->lastValue);

	addAndMakeVisible(meter);

	this->setSize(256, 60);
}

juce::Component* snex_timer::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
{
	return new editor(&static_cast<NodeType*>(obj)->tType, updater);
}

void snex_timer::editor::resized()
{
	auto b = this->getLocalBounds();
	auto t = b.removeFromTop(28);
	editCodeButton.setBounds(t.removeFromRight(60).reduced(4));

	meter.setBounds(b.removeFromTop(15));

	dragger.setBounds(b);
}

void snex_timer::editor::paint(Graphics& g)
{
	auto b = this->getLocalBounds().removeFromTop(28);

	auto ledArea = b.removeFromLeft(24).removeFromTop(24);

	g.setColour(Colours::white.withAlpha(0.7f));
	g.drawRect(ledArea, 1.0f);

	g.setColour(Colours::white.withAlpha(alpha));
	g.fillRect(ledArea.reduced(2.0f));
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

#endif

namespace core
{
juce::Path SnexMenuBar::Factory::createPath(const String& url) const
{
	if (url == "snex")
	{
		snex::ui::SnexPathFactory f;
		return f.createPath(url);
	}

	Path p;

	LOAD_PATH_IF_URL("new", SampleMapIcons::newSampleMap);
	LOAD_PATH_IF_URL("edit", HiBinaryData::SpecialSymbols::scriptProcessor);
	LOAD_PATH_IF_URL("popup", HiBinaryData::ProcessorEditorHeaderIcons::popupShape);
	LOAD_PATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_PATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_PATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	//LOAD_PATH_IF_URL("asm", SnexIcons::asmIcon);
	//LOAD_PATH_IF_URL("optimize", SnexIcons::optimizeIcon);

	return p;
}




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

void SnexMenuBar::buttonClicked(Button* b)
{
	if (b == &newButton)
	{
		auto name = PresetHandler::getCustomName(source->getId(), "Enter the name for the SNEX class file");

		if (name.isNotEmpty())
		{
			source->setClass(name);
			rebuildComboBoxItems();
			refreshButtonState();
		}
	}

	if (b == &popupButton)
	{
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Show popup editor", source->getWorkbench() != nullptr);
		m.addItem(2, "Show complex data", source->getComplexDataHandler().hasComplexData());

		if (auto r = m.show())
		{
			Component* c;

			if (r == 1)
			{
				c = new snex::jit::SnexPlayground(source->getWorkbench());
				c->setSize(900, 800);
			}
			else
			{
				c = new SnexComplexDataDisplay(source);
			}

			auto sp = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();
			auto area = sp->getLocalArea(this, getLocalBounds());
			sp->setCurrentModalWindow(c, area);

			//auto b = ft->getLocalArea(&popupButton, popupButton.getLocalBounds());

			//ft->showComponentInRootPopup(c, &popupButton, b.getCentre());
		}
	}

	if (b == &addButton)
	{
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Add Parameter");
		
		ExternalData::forEachType([&m](ExternalData::DataType t)
		{
			m.addItem(12 + (int)t, "Add " + ExternalData::getDataTypeName(t));
		});

		if (auto r = m.show())
		{
			if (r == 1)
			{
				auto n = PresetHandler::getCustomName("Parameter");

				if (n.isNotEmpty())
					source->getParameterHandler().addNewParameter(parameter::data(n, { 0.0, 1.0 }));
			}
			else
			{
				auto t = (ExternalData::DataType)(r - 12);
				source->getComplexDataHandler().addOrRemoveDataFromUI(t, true);
			}
		}
	}
	if (b == &deleteButton)
	{
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Remove last Parameter");

		ExternalData::forEachType([&m](ExternalData::DataType t)
		{
			m.addItem(12 + (int)t, "Remove " + ExternalData::getDataTypeName(t));
		});

		if (auto r = m.show())
		{
			if(r == 1)
				source->getParameterHandler().removeLastParameter();
			else
			{
				auto t = (ExternalData::DataType)(r - 12); 
				source->getComplexDataHandler().addOrRemoveDataFromUI(t, false);
			}
		}
	}

	if (b == &editButton)
	{
		auto& manager = dynamic_cast<BackendProcessor*>(findParentComponentOfClass<FloatingTile>()->getMainController())->workbenches;

		if (b->getToggleState())
			manager.setCurrentWorkbench(source->getWorkbench(), false);
		else
			manager.resetToRoot();
	}

	
}

void SnexComplexDataDisplay::rebuildEditors()
{
	auto updater = source->getParentNode()->getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
	auto undoManager = source->getParentNode()->getScriptProcessor()->getMainController_()->getControlUndoManager();

	auto& dataHandler = source->getComplexDataHandler();

	auto t = snex::ExternalData::DataType::Table;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::table_editor_without_mod(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::SliderPack;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::sliderpack_editor_without_mod(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::AudioFile;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		data::pimpl::dynamic_base* d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::audiofile_editor(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	setSize(512, editors.size() * 100);
}

SnexMenuBar::ComplexDataPopupButton::ComplexDataPopupButton(SnexSource* s) :
	Button("TSA"),
	source(s)
{
	t = s->getComplexDataHandler().getDataRoot();
	l.setTypesToWatch({ Identifier("Tables"), Identifier("SliderPacks"), Identifier("AudioFiles") });
	l.setCallback(t, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(ComplexDataPopupButton::update));

	setClickingTogglesState(true);

	onClick = [this]()
	{
		auto ft = findParentComponentOfClass<FloatingTile>();

		if (getToggleState())
		{
			ft->showComponentInRootPopup(nullptr, nullptr, {});
		}
		else
		{
			Component* c = new SnexComplexDataDisplay(source);

			auto nc = findParentComponentOfClass<NodeComponent>();
			auto lb = nc->getLocalBounds();
			auto fBounds = ft->getLocalBounds();

			auto la = ft->getLocalArea(nc, lb);

			auto showAbove = la.getY() > fBounds.getHeight() / 2;

			ft->showComponentInRootPopup(c, nc, { lb.getCentreX(), showAbove ? 0 : lb.getBottom() });
		}
	};
}

}

VuMeterWithModValue::VuMeterWithModValue(PooledUIUpdater* updater) :
	SimpleTimer(updater)
{
	setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	setColour(VuMeter::outlineColour, Colour(0x45ffffff));
	setType(VuMeter::MonoHorizontal);
	setColour(VuMeter::ledColour, Colours::grey);
}

}

