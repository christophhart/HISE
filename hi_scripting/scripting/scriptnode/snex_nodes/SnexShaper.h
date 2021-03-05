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



namespace waveshapers
{
struct dynamic : public SnexSource
{
	SN_GET_SELF_AS_OBJECT(dynamic);

	using NodeType = core::waveshaper<dynamic>;

	struct ShaperCallbacks: public SnexSource::CallbackHandlerBase
	{
		ShaperCallbacks(SnexSource& s, ObjectStorageType& o) :
			CallbackHandlerBase(s, o)
		{};

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override
		{
			auto newProcessFunction = getFunctionAsObjectCallback("process");
			auto newProcessFrameFunction = getFunctionAsObjectCallback("processFrame");
			auto r = newProcessFunction.validateWithArgs(Types::ID::Void, { Types::ID::Pointer });

			if(r.wasOk())
				r = newProcessFrameFunction.validateWithArgs(Types::ID::Void, { Types::ID::Pointer });

			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

				ok = r.wasOk();
				std::swap(processFunction, newProcessFunction);
				std::swap(processFrameFunction, newProcessFrameFunction);
			}

			return r;
		}

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

			processFunction = {};
			processFrameFunction = {};
			ok = false;
		}

		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			return Result::ok();
		}

		void process(ProcessDataDyn& data)
		{
			if (auto s = ScopedCallbackChecker(*this))
			{
				processFunction.callVoid(&data);

				for (auto ch : data)
					FloatSanitizers::sanitizeArray(data.toChannelData(ch));
			}
		}

		template <typename FrameDataType> void processFrame(FrameDataType& d)
		{
			if (auto s = ScopedCallbackChecker(*this))
			{
				processFrameFunction.callVoid(d.begin());
				FloatSanitizers::sanitizeArray(d);
			}
		}

		FunctionData processFunction;
		FunctionData processFrameFunction;
	};

	dynamic():
		callbacks(*this, object)
	{
		setCallbackHandler(&callbacks);
	}

	void process(ProcessDataDyn& data)
	{
		if (allowProcessing())
			callbacks.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		if (allowProcessing())
		{
			callbacks.processFrame(d);
		}
	}

	bool preprocess(String& c) final override
	{
		SnexSource::preprocess(c);

		if (auto pn = getParentNode())
		{
			int nc = pn->getNumChannelsToProcess();
			using namespace snex::cppgen;
			Base b(Base::OutputType::AddTabs);

			String def1, def2;

			def1 << "void process(ProcessData<" << String(nc) << ">& data)";	  b << def1;
			{														 StatementBlock body(b);
				b << (getCurrentClassId().toString() + " instance;");
				b << "instance.process(data);";
			}

			def2 << "void processFrame(span<float, " << String(nc) << ">& data)"; b << def2;
			{														 StatementBlock body(b);
				b << (getCurrentClassId().toString() + " instance;");
				b << "instance.processFrame(data);";
			}

			c << b.toString();
		}

		return true;
	}

	String getEmptyText(const Identifier& id) const override;

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("snex_shaper"); };

	SnexTestBase* createTester() override
	{
		return new Tester<ShaperCallbacks>(*this);
	}

	class editor : public ScriptnodeExtraComponent<dynamic>,
				   public WaveformComponent::Broadcaster,
				   public SnexSource::SnexSourceListener
	{
	public:

		editor(dynamic* t, PooledUIUpdater* updater);

		~editor();

		snex::Types::span<float, 128> tData;

		void wasCompiled(bool ) override { rebuild = true; };
		void complexDataAdded(snex::ExternalData::DataType , int ) override { rebuild = true; };
		void parameterChanged(int , double newValue) override { rebuild = true; };
		void complexDataTypeChanged() override { rebuild = true; };

		void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;

		void timerCallback() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater);

		void resized() override;

		SnexMenuBar menuBar;
		SnexPathFactory f;
		BlackTextButtonLookAndFeel blaf;
		GlobalHiseLookAndFeel claf;

		bool rebuild = false;
		WaveformComponent waveform;
	};

	

	ShaperCallbacks callbacks;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};


}

}
