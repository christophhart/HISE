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

core::SnexOscillator::OscillatorCallbacks::OscillatorCallbacks(SnexSource& p, ObjectStorageType& o) :
	CallbackHandlerBase(p, o)
{

}

void core::SnexOscillator::OscillatorCallbacks::reset()
{
	SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
	ok = false;
	//tickFunction = {};
	processFunction = {};
}

juce::Result core::SnexOscillator::OscillatorCallbacks::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
{
	auto r = Result::ok();

	auto newTickFunction = getFunctionAsObjectCallback("tick");
	auto newProcessFunction = getFunctionAsObjectCallback("process", false);
	auto newPrepareFunction = getFunctionAsObjectCallback("prepare", false);

	//r = newTickFunction.validateWithArgs(Types::ID::Float, { Types::ID::Double });

	Array<Types::ID> argTypes = { Types::ID::Pointer };

#if SNEX_MIR_BACKEND
	argTypes.add(Types::ID::Pointer);
#endif

	if (r.wasOk())
		r = newProcessFunction.validateWithArgs(Types::ID::Void, argTypes);

	if (r.wasOk() && newPrepareFunction.isResolved())
		r = newPrepareFunction.validateWithArgs(Types::ID::Void, argTypes);

	{
		SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
		ok = r.wasOk();
		//std::swap(newTickFunction, tickFunction);
		std::swap(newProcessFunction, processFunction);
		std::swap(newPrepareFunction, prepareFunction);
	}

	prepare(lastSpecs);

	return r;
}

#if 0
float core::SnexOscillator::OscillatorCallbacks::tick(double uptime)
{
	if (auto c = ScopedCallbackChecker(*this))
		return tickFunction.callUncheckedWithObj5ect<float>(uptime);

	return 0.0;
}
#endif

void core::SnexOscillator::OscillatorCallbacks::process(OscProcessData& d)
{
	lastDelta = d.delta;

	if (auto c = ScopedCallbackChecker(*this))
		processFunction.callVoidUncheckedWithObject(&d);
}

void core::SnexOscillator::OscillatorCallbacks::prepare(PrepareSpecs ps)
{
	lastSpecs = ps;

	if(auto c = ScopedCallbackChecker(*this))
		prepareFunction.callVoid(&lastSpecs);
}

core::SnexOscillator::SnexOscillator() :
	SnexSource(),
	callbacks(*this, object)
{
	setCallbackHandler(&callbacks);
}

String core::SnexOscillator::getEmptyText(const Identifier& id) const
{
	using namespace snex::cppgen;

	cppgen::Base c(cppgen::Base::OutputType::AddTabs);
	cppgen::Struct s(c, id, {}, { TemplateParameter(NamespacedIdentifier("NumVoices"), 0, false) });

	c.addComment("This macro enables C++ compilation to a snex_osc", snex::cppgen::Base::CommentType::Raw);
	addSnexNodeId(c, id);

	c.addComment("This function will be called once per sample", cppgen::Base::CommentType::Raw);
	c << "float tick(double uptime)\n";
	{
		StatementBlock sb(c);
		c << "return Math.fmod(uptime, 1.0);";
	}

	c.addEmptyLine();
	c.addComment("This function will calculate a chunk of samples", cppgen::Base::CommentType::Raw);
	c << "void process(OscProcessData& d)\n";
	{
		StatementBlock sb(c);
		c << "for (auto& s : d.data)";
		{
			StatementBlock sb2(c);
			c << "s = tick(d.uptime);";
			c << "d.uptime += d.delta;";
		}
	}

	c.addEmptyLine();
	c.addComment("This can be used to initialise the processing if required.", snex::cppgen::Base::CommentType::Raw);
	c << "void prepare(PrepareSpecs ps)\n";
	{
		StatementBlock sb2(c);
	}
	

	String pf;

	c.addEmptyLine();
	addDefaultParameterFunction(pf);

	c << pf;

	s.flushIfNot();

	auto code = c.toString();

	return code;
}

void core::SnexOscillator::initialise(NodeBase* n)
{
	SnexSource::initialise(n);
}


float core::SnexOscillator::tick(double uptime)
{
	float v = 0.0f;
	
	OscProcessData d;
	d.uptime = uptime;
	d.delta = 0.0;
	d.data.referToRawData(&v, 1);

	process(d);

	return v;

	//return callbacks.tick(uptime);
}


void core::SnexOscillator::process(OscProcessData& d)
{
	callbacks.process(d);
}

void core::SnexOscillator::prepare(PrepareSpecs ps)
{
	rebuildCallbacksAfterChannelChange(ps.numChannels);
	callbacks.prepare(ps);
	
}

bool core::SnexOscillator::preprocess(String& code)
{
	if (code.contains("instance.prepare(;"))
	{
		// already preprocessed...
		return true;
	}

	SnexSource::preprocess(code);
	SnexSource::addDummyProcessFunctions(code, false, "OscProcessData");
	SnexSource::addDummyNodeCallbacks(code, false, false, false, false);
	
	return true;
}

core::NewSnexOscillatorDisplay::NewSnexOscillatorDisplay(SnexOscillator* osc, PooledUIUpdater* updater) :
	ScriptnodeExtraComponent<SnexOscillator>(osc, updater),
	menuBar(osc),
	display()
{
	display.setComplexDataUIBase(osc->getMainDisplayBuffer().get());
	display.setSpecialLookAndFeel(new data::ui::pimpl::complex_ui_laf(), true);

	addAndMakeVisible(display);
	addAndMakeVisible(menuBar);
	setSize(256, 144);
	getObject()->addCompileListener(this);

	auto rb = osc->getMainDisplayBuffer();

	rb->setPropertyObject(new SnexOscPropertyObject(osc));
}

core::NewSnexOscillatorDisplay::~NewSnexOscillatorDisplay()
{
	getObject()->removeCompileListener(this);
}

void core::NewSnexOscillatorDisplay::complexDataAdded(snex::ExternalData::DataType t, int index)
{
	getObject()->getMainDisplayBuffer()->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
}

void core::NewSnexOscillatorDisplay::parameterChanged(int snexParameterId, double newValue)
{
	getObject()->getMainDisplayBuffer()->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
}

void core::NewSnexOscillatorDisplay::complexDataTypeChanged()
{
	getObject()->getMainDisplayBuffer()->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
}

void core::NewSnexOscillatorDisplay::wasCompiled(bool ok)
{
	if (ok)
	{
		display.errorMessage = {};

		if (auto rb = getObject()->getMainDisplayBuffer())
			rb->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
		
		repaint();
	}
	else
	{
		display.p = {};
		display.errorMessage = getObject()->getWorkbench()->getLastResult().compileResult.getErrorMessage();
		repaint();
	}
}

void core::NewSnexOscillatorDisplay::SnexDisplay::paint(Graphics& g)
{
	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	Path grid;
	grid.addRectangle(getLocalBounds().toFloat().reduced(4.0f));

	laf->drawAnalyserGrid(g, *this, grid);
	laf->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());

	if (errorMessage.isNotEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(errorMessage, getLocalBounds().toFloat(), Justification::centred);
	}
	else if (!p.getBounds().isEmpty())
	{
		laf->drawOscilloscopePath(g, *this, p);
	}
}

void core::NewSnexOscillatorDisplay::SnexDisplay::refresh()
{
	if (!getLocalBounds().isEmpty())
	{
		auto pathBounds = getLocalBounds().toFloat();

		p.clear();
		p.startNewSubPath(0.0f, -1.0f);
		p.startNewSubPath(0.0f, 1.0f);
		p.startNewSubPath(0.0f, 0.0f);

		float i = 0.0f;

		auto& buffer = rb->getReadBuffer();

		for (i = 0.0f; i < buffer.getNumSamples(); i += 1.0f)
		{
			float v = buffer.getSample(0, i);
			FloatSanitizers::sanitizeFloatNumber(v);
			v = jlimit(-10.0f, 10.0f, v);
			p.lineTo(i, -1.0f * v);
		}

		p.lineTo(i - 1.0f, 0.0f);
		//p.closeSubPath();

		auto pb = pathBounds.reduced(4.0f);

		if (p.getBounds().getHeight() > 0.0f && p.getBounds().getWidth() > 0.0f)
			p.scaleToFit(pb.getX(), pb.getY(), pb.getWidth(), pb.getHeight(), false);
	}

	repaint();
}

void core::NewSnexOscillatorDisplay::timerCallback()
{
}

void core::NewSnexOscillatorDisplay::resized()
{
	auto t = getLocalBounds();
	menuBar.setBounds(t.removeFromTop(24));
	t.removeFromTop(20);

	display.setBounds(t.reduced(2));
}

Component* core::NewSnexOscillatorDisplay::createExtraComponent(void* obj, PooledUIUpdater* u)
{
	auto t = static_cast<mothernode*>(obj);
	auto typed = dynamic_cast<snex_osc_base<SnexOscillator>*>(t);
	return new NewSnexOscillatorDisplay(&typed->oscType, u);
}

void core::NewSnexOscillatorDisplay::SnexOscPropertyObject::transformReadBuffer(AudioSampleBuffer& b)
{
	if (osc != nullptr)
	{
		OscProcessData d;
		d.delta = 1.0 / b.getNumSamples();

		d.data.referToRawData(b.getWritePointer(0), b.getNumSamples());
		d.uptime = 0.0;

		SnexOscillator::OscTester tester(*osc);
		tester.callbacks.process(d);
	}
}

}

