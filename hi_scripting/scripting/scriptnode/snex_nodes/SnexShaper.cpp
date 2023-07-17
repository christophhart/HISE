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

namespace waveshapers
{
	String dynamic::getEmptyText(const Identifier& id) const
	{
		using namespace snex::cppgen;

		Base c(Base::OutputType::AddTabs);

		cppgen::Struct s(c, id, {}, { TemplateParameter(NamespacedIdentifier("NumVoices"), 0, false) });

		addSnexNodeId(c, id);

		c.addComment("Implement the Waveshaper here...", Base::CommentType::RawWithNewLine);
		c << "float getSample(float input)";

		{
			StatementBlock body(c);
			c << "return input;";
		}

		c.addComment("These functions are the glue code that call the function above", Base::CommentType::Raw);
		c << "template <typename T> void process(T& data)";
		{							 StatementBlock body(c);
			c << "for(auto ch: data)";
			{						 StatementBlock loop(c);
				c << "for(auto& s: data.toChannelData(ch))";
				{					StatementBlock loop2(c);
					c << "s = getSample(s);";
				}
			}
		}

		c << "template <typename T> void processFrame(T& data)";
		{								 StatementBlock body(c);
			c << "for(auto& s: data)";
			c << "s = getSample(s);";
		}

		c << "void reset()";
		{								 StatementBlock body(c);
			c.addEmptyLine();
		}

		c << "void prepare(PrepareSpecs ps)";
		{								 StatementBlock body(c);
			c.addEmptyLine();
		}

		String pf;
		c.addEmptyLine();
		addDefaultParameterFunction(pf);
		c << pf;

		s.flushIfNot();

		return c.toString();
	}

	dynamic::editor::editor(dynamic* t, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<dynamic>(t, updater),
		waveform(nullptr, 0),
		menuBar(t)
	{
		t->addCompileListener(this);
		addAndMakeVisible(menuBar);

		getObject()->connectWaveformUpdaterToComplexUI(t->getMainDisplayBuffer().get(), true);

		waveform.setSpecialLookAndFeel(new data::ui::pimpl::complex_ui_laf(), true);
		waveform.setComplexDataUIBase(t->getMainDisplayBuffer().get());

		addAndMakeVisible(waveform);
		t->addWaveformListener(&waveform);

		this->setSize(256, 128 + 24 + 16);
	}

	dynamic::editor::~editor()
	{
		getObject()->removeWaveformListener(&waveform);
		getObject()->connectWaveformUpdaterToComplexUI(getObject()->getMainDisplayBuffer().get(), false);
		getObject()->removeCompileListener(this);
	}

	void dynamic::getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue)
	{
		for (int i = 0; i < 128; i++)
			tData[i] = 2.0f * (float)i / 127.0f - 1.0f;

		auto n = getParentNode()->getCurrentChannelAmount();

		float** d = (float**)alloca(sizeof(float*) * n);

		float* x = tData.begin();

		for (int i = 0; i < n; i++)
			d[i] = x;

		ProcessDataDyn pd(d, 128, n);

		SnexSource::Tester<ShaperCallbacks> tester(*this);

		tester.callbacks.process(pd);

		*tableValues = tData.begin();
		numValues = 128;
		normalizeValue = 1.0f;
	}

	void dynamic::editor::timerCallback()
	{
		if (rebuild)
			getObject()->getMainDisplayBuffer()->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationSync, true);
		
		rebuild = false;
	}

	juce::Component* dynamic::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto typed = static_cast<NodeType*>(obj);
		return new editor(&typed->shaper, updater);
	}

	void dynamic::editor::resized()
	{
		auto b = this->getLocalBounds();

		menuBar.setBounds(b.removeFromTop(24));
		b.removeFromTop(16);
		waveform.setBounds(b);
	}

	juce::Result dynamic::ShaperCallbacks::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
	{
		auto newProcessFunction = getFunctionAsObjectCallback("process");
		auto newProcessFrameFunction = getFunctionAsObjectCallback("processFrame");
		auto newPrepareFunc = getFunctionAsObjectCallback("prepare");
		auto newResetFunc = getFunctionAsObjectCallback("reset");

		Array<Types::ID> argTypes = { Types::ID::Pointer };
		Array<Types::ID> argTypes0 = {  };

#if SNEX_MIR_BACKEND
		argTypes.add(Types::ID::Pointer);
		argTypes0.add(Types::ID::Pointer);
#endif

		auto r = newProcessFunction.validateWithArgs(Types::ID::Void, argTypes);

		if (r.wasOk())
			r = newProcessFrameFunction.validateWithArgs(Types::ID::Void, argTypes);

		if (r.wasOk())
			r = newPrepareFunc.validateWithArgs(Types::ID::Void, argTypes);

		if (r.wasOk())
			r = newResetFunc.validateWithArgs(Types::ID::Void, argTypes0);

		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

			ok = r.wasOk();
			std::swap(processFunction, newProcessFunction);
			std::swap(processFrameFunction, newProcessFrameFunction);
			std::swap(prepareFunc, newPrepareFunc);
			std::swap(resetFunc, newResetFunc);
		}

		prepare(lastSpecs);

		return r;
	}

}



}

