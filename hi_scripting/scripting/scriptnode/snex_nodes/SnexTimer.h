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

namespace control
{



struct snex_timer : public SnexSource
{
	using NodeType = control::timer_impl<1, snex_timer>;

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

			auto r = newTc.validateWithArgs(Types::ID::Double, {});

			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
				std::swap(newTc, tc);
				ok = r.wasOk();
			}

			return r;
		}

		double getTimerValue()
		{
			if (auto c = ScopedCallbackChecker(*this))
				return tc.call<double>();
			
			return 0.0;
		}

		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			return Result::ok();
		}

		FunctionData tc;
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

	TimerCallbackHandler callbacks;
	ModValue lastValue;

	HISE_EMPTY_PREPARE;

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
		VuMeterWithModValue meter;

		Rectangle<float> flashDot;
		float alpha = 0.0f;
		ModulationSourceBaseComponent dragger;
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(snex_timer);
};


}

}

