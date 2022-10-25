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

#if HISE_INCLUDE_SNEX
using OptionalSnexSource = SnexSource;
#else
struct OptionalSnexSource
{
	struct DummyCallbackHandler
	{
		void prepare(PrepareSpecs) {};

		DummyCallbackHandler(OptionalSnexSource& , int) {};
	};

	struct SnexTestBase
	{
		virtual ~SnexTestBase() {};
	};

	template <typename T> struct Tester: public SnexTestBase
	{
		Tester(OptionalSnexSource& p) {};
	};

	virtual SnexTestBase* createTester() = 0;

	void setCallbackHandler(DummyCallbackHandler*) {};

	virtual void initialise(NodeBase* ) {};

	virtual String getEmptyText(const Identifier& id) const { return {}; }

	virtual Identifier getTypeId() const = 0;

	virtual bool preprocess(String& code)
	{
		ignoreUnused(code);
		return false;
	}

	virtual ~OptionalSnexSource() {};

	int object;

	NodeBase* getParentNode() { return p; }
	
	struct editor
	{
		static Component* createExtraComponent(void*, PooledUIUpdater*) { return nullptr; }
	};

    /** You can define additional parameters that control the behaviour of this node. Be aware that the parameter offset starts after the fixed parameters of this node (so P==0 is your first parameter). */
	template <int P> void setParameter(double) {};

private:
	
	NodeBase::Ptr p;
};
#endif

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


/** A node that will send a periodic modulation signal.
    @ingroup snex_nodes
 
 */
struct snex_timer : public OptionalSnexSource
{
	enum class TimerMode
	{
		Ping,
		Toggle,
		Random,
		Custom,
		numModes
	};

#if HISE_INCLUDE_SNEX
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
#else
	using TimerCallbackHandler = OptionalSnexSource::DummyCallbackHandler;
#endif

	snex_timer() :
		OptionalSnexSource(),
		callbacks(*this, object),
		mode(PropertyIds::Mode, "Ping")
	{
		setCallbackHandler(&callbacks);
	}

	static StringArray getModes() { return { "Ping", "Toggle", "Random", "Custom" }; }

	String getEmptyText(const Identifier& id) const override;

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("timer"); };

	SnexTestBase* createTester() override
	{
		return new Tester<TimerCallbackHandler>(*this);
	}

	void initialise(NodeBase* n)
	{
		OptionalSnexSource::initialise(n);

		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(snex_timer::updateMode), true);
	}

	void updateMode(Identifier, var newValue)
	{
		currentMode = (TimerMode)getModes().indexOf(newValue.toString());
		reset();
	}

    /** This function will be called once for each timer interval and specifies which value it should send
     
        The return value must be between 0.0 and 1.0 and will be scaled to the target parameter automatically.
    */
	double getTimerValue();

    /** You can use this callback to reset the state of the timer. */
	void reset()
	{
		switch (currentMode)
		{
		case TimerMode::Ping:	pingTimer.reset(); break;
#if HISE_INCLUDE_SNEX
		case TimerMode::Custom: callbacks.resetTimer(); break;
#endif
		case TimerMode::Toggle: toggleTimer.reset(); break;
		case TimerMode::Random: randomTimer.reset(); break;
        default: break;
		}
	}

	bool preprocess(String& code) override
	{
		OptionalSnexSource::preprocess(code);
		return true;
	}

    /** Initialises the processing. */
	void prepare(PrepareSpecs ps)
	{
		callbacks.prepare(ps);
		toggleTimer.prepare(ps);
		randomTimer.prepare(ps);
		pingTimer.prepare(ps);
	}

	TimerCallbackHandler callbacks;
	mutable ModValue lastValue;

#if HISE_INCLUDE_SNEX
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

		ComboBoxWithModeProperty modeSelector;
		ModulationSourceBaseComponent dragger;
	};
#endif

	NodePropertyT<String> mode;

	TimerMode currentMode;

	timer_logic::toggle<NUM_POLYPHONIC_VOICES> toggleTimer;
	timer_logic::random<1> randomTimer;
	timer_logic::ping<1> pingTimer;

	JUCE_DECLARE_WEAK_REFERENCEABLE(snex_timer);
};


}

}

