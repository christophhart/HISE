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
using namespace snex;

struct FlashingModKnob : public Component,
						 public PooledUIUpdater::SimpleTimer
{
	FlashingModKnob(PooledUIUpdater* u, ModValue* mv) :
		SimpleTimer(u, true),
		modValue(mv)
	{};

	void timerCallback() override
	{
		double unused;

		auto ui_led = modValue->getChangedValue(unused);

		if (ui_led)
		{
			on = !on;
			repaint();
		}
	}

	void paint(Graphics& g) override;

	ModValue* modValue;
	
	bool on = false;
};

namespace control
{



struct snex_timer : public SnexSource
{
	using NodeType = control::timer<snex_timer>;

	struct TimerCallbackHandler : public SnexSource::CallbackHandlerBase
	{
		TimerCallbackHandler(SnexSource& p, ObjectStorageType& o) :
			CallbackHandlerBase(p, o)
		{};

		void reset()
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
			ok = false;
			tc = {};
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override
		{
			auto newTc = getFunctionAsObjectCallback("getTimerValue");
			auto newReset = getFunctionAsObjectCallback("reset");
			auto newPrepare = getFunctionAsObjectCallback("prepare");

			auto r = newTc.validateWithArgs(Types::ID::Double, {});

			if (r.wasOk())
				r = newReset.validateWithArgs(Types::ID::Void, {});

			if (r.wasOk())
				r = newPrepare.validateWithArgs("void", { "PrepareSpecs" });

			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
				std::swap(newTc, tc);
				std::swap(prepareFunc, newPrepare);
				std::swap(resetFunc, newReset);
				ok = r.wasOk();
			}

			prepare(lastSpecs);
			resetTimer();

			return r;
		}

		double getTimerValue()
		{
			if (auto c = ScopedCallbackChecker(*this))
				return tc.call<double>();
			
			return 0.0;
		}

		void resetTimer()
		{
			if (auto s = ScopedCallbackChecker(*this))
			{
				resetFunc.callVoid();
			}
		}

		void prepare(PrepareSpecs ps)
		{
			lastSpecs = ps;

			if (auto s = ScopedCallbackChecker(*this))
			{
				prepareFunc.callVoid(&lastSpecs);
			}
		}

		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			return Result::ok();
		}

		PrepareSpecs lastSpecs;

		FunctionData tc;
		FunctionData resetFunc;
		FunctionData prepareFunc;
	};

	snex_timer() :
		SnexSource(),
		callbacks(*this, object)
	{
		setCallbackHandler(&callbacks);
	}


	String getEmptyText(const Identifier& id) const override;

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("timer"); };

	SnexTestBase* createTester() override
	{
		return new Tester<TimerCallbackHandler>(*this);
	}

	double getTimerValue();

	void reset()
	{
		callbacks.resetTimer();
	}

	void prepare(PrepareSpecs ps)
	{
		callbacks.prepare(ps);
	}

	TimerCallbackHandler callbacks;
	mutable ModValue lastValue;

	class editor : public ScriptnodeExtraComponent<snex_timer>,
				  SnexSource::SnexSourceListener
	{
	public:

		editor(snex_timer* t, PooledUIUpdater* updater);

		~editor()
		{
			getObject()->removeCompileListener(this);
		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater);

		void resized() override;

		void paint(Graphics& g) override;

		void timerCallback() override;

		SnexMenuBar menuBar;
		
		FlashingModKnob modKnob;

		ModulationSourceBaseComponent dragger;
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(snex_timer);
};


}

}

