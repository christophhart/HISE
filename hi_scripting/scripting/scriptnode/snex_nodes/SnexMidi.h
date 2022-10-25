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

namespace midi_logic
{
struct dynamic : public OptionalSnexSource
{
	using NodeType = control::midi<dynamic>;

#if HISE_INCLUDE_SNEX
	struct CustomMidiCallback : public SnexSource::CallbackHandlerBase
	{
		CustomMidiCallback(SnexSource& parent, SnexSource::HandlerBase::ObjectStorageType& o) :
			CallbackHandlerBase(parent, o)
		{};

		void reset() override;

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;

		void prepare(PrepareSpecs ps)
		{
			lastSpecs = ps;

			if (auto c = ScopedCallbackChecker(*this))
				prepareFunction.callVoid(&lastSpecs);
		}

		int getMidiValue(HiseEvent* e, double* v)
		{
			if (auto c = ScopedCallbackChecker(*this))
				return midiFunction.call<int>(e, v);

			return 0;
		}

		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			auto wb = static_cast<snex::ui::WorkbenchManager*>(getNodeWorkbench(parent.getParentNode()));
			
			if (auto rwb = wb->getRootWorkbench())
			{
				auto& td = rwb->getTestData();

				struct TestData
				{
					PrepareSpecs ps;
					double d;
					HiseEvent e;
				};

				// needs to be on the heap or it crashes in optimised mode...
				ScopedPointer<TestData> data = new TestData();

				data->ps = td.getPrepareSpecs();
				data->d = 0.0;
				data->e = {};

				auto f = getFunctionAsObjectCallback("prepare");
				f.callVoid(&data->ps);

				auto m = getFunctionAsObjectCallback("getMidiValue");

				for (int i = 0; i < td.getNumTestEvents(false); i++)
				{
					data->e = td.getTestHiseEvent(i);
					m.call<int>(&data->e, &data->d);
				}

				return Result::ok();

			}

			jassertfalse;

			return Result::ok();
		}

		PrepareSpecs lastSpecs;
		snex::jit::FunctionData prepareFunction;
		snex::jit::FunctionData midiFunction;
	};
#else
	using CustomMidiCallback = OptionalSnexSource::DummyCallbackHandler;
#endif

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

	String getEmptyText(const Identifier& id) const override;

	dynamic();;

#if HISE_INCLUDE_SNEX
	class editor : public ScriptnodeExtraComponent<dynamic>,
		public SnexSource::SnexSourceListener,
		public Value::Listener
	{
	public:

		editor(dynamic* t, PooledUIUpdater* updater);

		~editor()
		{
			if (auto obj = getObject())
			{
				getObject()->removeCompileListener(this);
				midiMode.mode.asJuceValue().removeListener(this);
			}
		}

		void valueChanged(Value& value) override;

		void paint(Graphics& g) override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<NodeType*>(obj);
			return new editor(&typed->mType, updater);
		}

		void wasCompiled(bool ok) override {};
		void parameterChanged(int snexParameterId, double newValue) override
		{

		}

		void complexDataAdded(snex::ExternalData::DataType t, int index) override
		{}

		void resized() override;

		void timerCallback() override;

		SnexMenuBar menuBar;

		SnexPathFactory f;
		BlackTextButtonLookAndFeel blaf;
		GlobalHiseLookAndFeel claf;

		ComboBoxWithModeProperty midiMode;

		ModulationSourceBaseComponent dragger;
		VuMeterWithModValue meter;
	};	
#endif

	void prepare(PrepareSpecs ps);

	void initialise(NodeBase* n) override;

	void setMode(Identifier id, var newValue);

	bool getMidiValue(HiseEvent& e, double& v);

	bool getMidiValueWrapped(HiseEvent& e, double& v);

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("midi"); };


	SnexTestBase* createTester() override
	{
		return new Tester<CustomMidiCallback>(*this);
	}

	ModValue lastValue;
	Mode currentMode = Mode::Gate;
	NodePropertyT<String> mode;

	CustomMidiCallback callbacks;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};



}
}
