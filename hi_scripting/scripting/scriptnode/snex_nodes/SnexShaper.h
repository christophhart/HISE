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
struct dynamic : public SnexSource,
			     public WaveformComponent::Broadcaster
{
	SN_GET_SELF_AS_OBJECT(dynamic);

	using NodeType = core::snex_shaper<dynamic>;

	struct ShaperCallbacks: public SnexSource::CallbackHandlerBase
	{
		ShaperCallbacks(SnexSource& s, ObjectStorageType& o) :
			CallbackHandlerBase(s, o)
		{
			
		};

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

			processFunction = {};
			processFrameFunction = {};
			prepareFunc = {};
			resetFunc = {};
			ok = false;
		}

		bool runRootTest() const override { return true; }

		void runPrepareTest(PrepareSpecs ps) override
		{
			prepare(ps);
			resetShaper();
		}

		void runProcessTest(ProcessDataDyn& d) override
		{
			process(d);
		}

		void runHiseEventTest(HiseEvent& ) override
		{
		}

		void resetShaper()
		{
			if (auto s = ScopedCallbackChecker(*this))
				resetFunc.callVoidUncheckedWithObject();
		}

		void process(ProcessDataDyn& data)
		{
			if (auto s = ScopedCallbackChecker(*this))
			{
				processFunction.callVoidUncheckedWithObject(&data);

				for (auto ch : data)
                {
                    auto b = data.toChannelData(ch);
					FloatSanitizers::sanitizeArray(b);
                }
			}
		}

		template <typename FrameDataType> void processFrame(FrameDataType& d)
		{
			if (auto s = ScopedCallbackChecker(*this))
			{
				processFrameFunction.callVoidUncheckedWithObject(d.begin());
				FloatSanitizers::sanitizeArray(d);
			}
		}

		void prepare(PrepareSpecs ps)
		{
			lastSpecs = ps;

			if (auto s = ScopedCallbackChecker(*this))
				prepareFunc.callVoidUncheckedWithObject(&lastSpecs);
		}

		PrepareSpecs lastSpecs;

		FunctionData prepareFunc;
		FunctionData resetFunc;
		FunctionData processFunction;
		FunctionData processFrameFunction;
		FunctionData resetF;
	};

	dynamic():
		callbacks(*this, object)
	{
		setCallbackHandler(&callbacks);
	}

	void reset()
	{
		callbacks.resetShaper();
	}

	void prepare(PrepareSpecs ps)
	{
		rebuildCallbacksAfterChannelChange(ps.numChannels);
		callbacks.prepare(ps);
	}

	void process(ProcessDataDyn& data)
	{
		callbacks.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		callbacks.processFrame(d);
	}

	bool preprocess(String& code) final override
	{
		if (code.contains("instance.reset();"))
		{
			// already preprocessed...
			return true;
		}

		SnexSource::preprocess(code);
		SnexSource::addDummyProcessFunctions(code, true);
		SnexSource::addDummyNodeCallbacks(code, false, true, false);
		
		return true;
	}

	void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;

	String getEmptyText(const Identifier& id) const override;

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("snex_shaper"); };

	SnexTestBase* createTester() override
	{
		return new Tester<ShaperCallbacks>(*this);
	}

	class editor : public ScriptnodeExtraComponent<dynamic>,
				   public SnexSource::SnexSourceListener
	{
	public:

		editor(dynamic* t, PooledUIUpdater* updater);

		~editor();

		

		void wasCompiled(bool ) override { rebuild = true; };
		void complexDataAdded(snex::ExternalData::DataType , int ) override { rebuild = true; };
		void parameterChanged(int , double newValue) override { rebuild = true; };
		void complexDataTypeChanged() override { rebuild = true; };

		

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

	snex::Types::span<float, 128> tData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};


}

}
