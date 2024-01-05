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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace filters
{




template <class FilterType, int NV> 
class FilterNodeBase : public data::filter_base,
					   public polyphonic_base,
					   public hise::ComplexDataUIUpdaterBase::EventListener
{
public:

	enum Parameters
	{
		Frequency,
		Q,
		Gain,
		Smoothing,
		Mode,
		Enabled,
		numParameters
	};

	using FilterObject = FilterType;

	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID(FilterType::getFilterTypeId());

	FilterNodeBase() :
		polyphonic_base(getStaticId(), false)
	{};

	SN_GET_SELF_AS_OBJECT(FilterNodeBase);
	SN_DESCRIPTION("A filter node");

	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_INITIALISE;

	void createParameters(ParameterDataList& parameters);
	void prepare(PrepareSpecs ps);
	void reset();

	void setExternalData(const ExternalData& d, int index) override
	{
		if (this->externalData.obj != nullptr)
			this->externalData.obj->getUpdater().removeEventListener(this);

		jassert(d.isTypeOrEmpty(ExternalData::DataType::FilterCoefficients));

		filter_base::setExternalData(d, index);

		if (auto fd = dynamic_cast<FilterDataObject*>(d.obj))
		{
			fd->getUpdater().addEventListener(this);
			
			if(sr > 0.0)
				fd->setSampleRate(sr);
		}
	}

	std::pair<IIRCoefficients, int> getApproximateCoefficients() const override;

	void onComplexDataEvent(hise::ComplexDataUIUpdaterBase::EventType e, var newValue) override;

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (enabled)
		{
			auto b = data.toAudioSampleBuffer();
			FilterHelpers::RenderData r(b, 0, data.getNumSamples());
			filter.get().render(r);
		}
		
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if(enabled)
			filter.get().processFrame(data.begin(), data.size());
	}
	
	void setFrequency(double newFrequency);
	void setGain(double newGain);
	void setQ(double newQ);
	void setMode(double newMode);
	void setSmoothing(double newSmoothingTime);
	void setEnabled(double isEnabled);

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, FilterNodeBase);
		DEF_PARAMETER(Gain, FilterNodeBase);
		DEF_PARAMETER(Q, FilterNodeBase);
		DEF_PARAMETER(Smoothing, FilterNodeBase);
		DEF_PARAMETER(Mode, FilterNodeBase);
		DEF_PARAMETER(Enabled, FilterNodeBase);
	}
	SN_PARAMETER_MEMBER_FUNCTION;
	

	PolyData<FilterObject, NumVoices> filter;
	double sr = -1.0;
	bool enabled = true;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FilterNodeBase);
};






#define DEFINE_FILTER_NODE_TEMPLATE2(fid, FilterType) template <int NV> using fid = FilterNodeBase<hise::MultiChannelFilter<FilterType>, NV>;

#if 0
#define DEFINE_FILTER_NODE_TEMPLATE(monoName, polyName, className) extern template class FilterNodeBase<className, 1>; \
using monoName = FilterNodeBase<hise::MultiChannelFilter<className>, 1>; \
extern template class FilterNodeBase<className, NUM_POLYPHONIC_VOICES>; \
using polyName = FilterNodeBase<hise::MultiChannelFilter<className>, NUM_POLYPHONIC_VOICES>; 
#endif

DEFINE_FILTER_NODE_TEMPLATE2(svf, StateVariableFilterSubType);
DEFINE_FILTER_NODE_TEMPLATE2(biquad, StaticBiquadSubType);
DEFINE_FILTER_NODE_TEMPLATE2(one_pole, SimpleOnePoleSubType);
DEFINE_FILTER_NODE_TEMPLATE2(ring_mod, RingmodFilterSubType);
DEFINE_FILTER_NODE_TEMPLATE2(allpass, PhaseAllpassSubType);
DEFINE_FILTER_NODE_TEMPLATE2(ladder, LadderSubType);
DEFINE_FILTER_NODE_TEMPLATE2(moog, MoogFilterSubType);
DEFINE_FILTER_NODE_TEMPLATE2(svf_eq, StateVariableEqSubType);
DEFINE_FILTER_NODE_TEMPLATE2(linkwitzriley, LinkwitzRiley);

#undef DEFINE_FILTER_NODE_TEMPLATE2

#if 0
template <int NV> struct fir_impl : public AudioFileNodeBase
{
	using CoefficientType = juce::dsp::FIR::Coefficients<float>;
	using FilterType = juce::dsp::FIR::Filter<float>;

	static constexpr int NumVoices = NV;

	SN_NODE_ID("fir");
	SN_GET_SELF_AS_OBJECT(fir_impl);

	SN_EMPTY_SET_PARAMETER;
	SN_EMPTY_HANDLE_EVENT;

	fir_impl();;

	bool isPolyphonic() const { return NumVoices > 1; }

	void prepare(PrepareSpecs specs)
	{
		dsp::ProcessSpec ps;
		ps.numChannels = 1;
		ps.sampleRate = specs.sampleRate;
		ps.maximumBlockSize = specs.blockSize;

		leftFilters.prepare(specs);
		rightFilters.prepare(specs);

		for (auto& f : leftFilters)
			f.prepare(ps);

		for (auto& f : rightFilters)
			f.prepare(ps);

		rebuildCoefficients();
	}
	
	void rebuildCoefficients()
	{
		if (currentBuffer->range.getNumSamples() == 0)
			return;

		auto ptrs = currentBuffer->range.getArrayOfReadPointers();

		auto tapSize = jmin(128, currentBuffer->range.getNumSamples());

		leftCoefficients = new CoefficientType(ptrs[0], tapSize);

		if (currentBuffer->range.getNumChannels() > 1)
			rightCoefficients = new CoefficientType(ptrs[1], tapSize);
		else
			rightCoefficients = leftCoefficients;

		SimpleReadWriteLock::ScopedWriteLock sl(coefficientLock);

		for (auto& f : leftFilters)
		{
			f.coefficients = leftCoefficients;
			f.reset();
		}

		for (auto& f : rightFilters)
		{
			f.coefficients = rightCoefficients;
			f.reset();
		}

		ok = true;
	}

	void contentChanged() override
	{
		AudioFileNodeBase::contentChanged();

		rebuildCoefficients();
	}

	void reset()
	{
		SimpleReadWriteLock::ScopedReadLock sl(coefficientLock, true);

		for (auto& f : leftFilters)
			f.reset();

		for (auto& f : rightFilters)
			f.reset();
	}

	SN_EMPTY_MOD;

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (!ok)
			return;

		SimpleReadWriteLock::ScopedReadLock sl(coefficientLock, true);

		dsp::AudioBlock<float> l(d.getRawDataPointers(), 1, d.getNumSamples());
		dsp::ProcessContextReplacing<float> pcrl(l);
		leftFilters.get().process(pcrl);
		
		if (d.getNumChannels() > 1)
		{
			dsp::AudioBlock<float> r(d.getRawDataPointers() + 1, 1, d.getNumSamples());
			dsp::ProcessContextReplacing<float> pcrr(r);
			rightFilters.get().process(pcrr);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (!ok)
			return;

		SimpleReadWriteLock::ScopedReadLock sl(coefficientLock, true);

		data[0] = leftFilters.get().processSample(data[0]);

		if (data.size() > 1)
			data[1] = rightFilters.get().processSample(data[1]);
	}
	
private:

	bool ok = false;

	SimpleReadWriteLock coefficientLock;

	CoefficientType::Ptr leftCoefficients;
	CoefficientType::Ptr rightCoefficients;

	PolyData<FilterType, NumVoices> leftFilters;
	PolyData<FilterType, NumVoices> rightFilters;

};

DEFINE_EXTERN_NODE_TEMPLATE(fir, fir_poly, fir_impl);

#endif

}

}
