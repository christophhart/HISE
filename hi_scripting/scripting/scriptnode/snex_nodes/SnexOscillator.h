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

namespace core
{

struct SnexOscillator : public SnexSource
{
	struct OscillatorCallbacks : public SnexSource::CallbackHandlerBase
	{
		OscillatorCallbacks(SnexSource& p, ObjectStorageType& o);;

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;

		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			

			struct TestData
			{
				TestData()
				{
					d.data.referTo(data);
					d.delta = 1.0 / 256.0;
					d.uptime = 0.0;

				}
				OscProcessData d;
				span<float, 256> data;
			};

			ScopedPointer<TestData> td = new TestData();

			auto f = getFunctionAsObjectCallback("process");

			f.callVoid(&td->d);

			return Result::ok();
		}

		void reset() override;
		float tick(double uptime);
		void process(OscProcessData& d);

		FunctionData tickFunction;
		FunctionData processFunction;
	};

	using OscTester = SnexSource::Tester<OscillatorCallbacks>;

	SnexOscillator();

	SnexTestBase* createTester() override { return new OscTester(*this); }

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("snex_osc"); };
	String getEmptyText(const Identifier& id) const override;

	void initialise(NodeBase* n);
	float tick(double uptime);
	void process(OscProcessData& d);

	OscillatorCallbacks callbacks;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexOscillator);
};


struct NewSnexOscillatorDisplay : public ScriptnodeExtraComponent<SnexOscillator>,
	public SnexSource::SnexSourceListener
{
	NewSnexOscillatorDisplay(SnexOscillator* osc, PooledUIUpdater* updater);
	~NewSnexOscillatorDisplay();

	void complexDataAdded(snex::ExternalData::DataType t, int index) override;
	void parameterChanged(int snexParameterId, double newValue) override;
	void complexDataTypeChanged() override;
	void wasCompiled(bool ok);

	void timerCallback() override;
	void resized() override;
	void paint(Graphics& g) override;

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u);

private:

	heap<float> buffer;
	OscProcessData od;

	bool rebuildPath = false;

	SnexMenuBar menuBar;
	Path p;
	String errorMessage;
	Rectangle<float> pathBounds;
};


template <typename T> struct snex_osc_base
{
	void initialise(NodeBase* n)
	{
		oscType.initialise(n);
	}

	T oscType;
};

template <int NV, typename T> struct snex_osc_impl : snex_osc_base<T>
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
		auto& thisData = oscData.get();
		auto uptime = thisData.tick();
		data[0] += this->oscType.tick(thisData.tick());
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
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

}
}