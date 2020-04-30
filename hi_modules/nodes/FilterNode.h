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

struct CoefficientProvider
{
	virtual ~CoefficientProvider() {};
	virtual IIRCoefficients getCoefficients() = 0;

	double sr = 44100.0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(CoefficientProvider);
};


template <class FilterType, int NV> 
class FilterNodeBase : public HiseDspBase,
	public CoefficientProvider
{
public:

	enum Parameters
	{
		Frequency,
		Q,
		Gain,
		Smoothing,
		Mode,
		numParameters
	};

	using FilterObject = FilterType;

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID(FilterType::getFilterTypeId());

	GET_SELF_AS_OBJECT();
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& parameters) override;
	void prepare(PrepareSpecs ps);
	void reset();

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto b = data.toAudioSampleBuffer();
		FilterHelpers::RenderData r(b, 0, data.getNumSamples());
		filter.get().render(r);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		filter.get().processFrame(data.begin(), data.size());
	}

	IIRCoefficients getCoefficients() override;
	void setFrequency(double newFrequency);
	void setGain(double newGain);
	void setQ(double newQ);
	void setMode(double newMode);
	void setSmoothing(double newSmoothingTime);

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, FilterNodeBase);
		DEF_PARAMETER(Gain, FilterNodeBase);
		DEF_PARAMETER(Q, FilterNodeBase);
		DEF_PARAMETER(Mode, FilterNodeBase);
		DEF_PARAMETER(Smoothing, FilterNodeBase);
	}

	AudioSampleBuffer buffer;
	PolyData<FilterObject, NumVoices> filter;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FilterNodeBase);
};



#define DEFINE_FILTER_NODE_TEMPLATE(monoName, polyName, className) extern template class FilterNodeBase<className, 1>; \
using monoName = FilterNodeBase<hise::MultiChannelFilter<className>, 1>; \
extern template class FilterNodeBase<className, NUM_POLYPHONIC_VOICES>; \
using polyName = FilterNodeBase<hise::MultiChannelFilter<className>, NUM_POLYPHONIC_VOICES>; 

DEFINE_FILTER_NODE_TEMPLATE(svf, svf_poly, StateVariableFilterSubType);
DEFINE_FILTER_NODE_TEMPLATE(biquad, biquad_poly, StaticBiquadSubType);
DEFINE_FILTER_NODE_TEMPLATE(one_pole, one_pole_poly, SimpleOnePoleSubType);
DEFINE_FILTER_NODE_TEMPLATE(ring_mod, ring_mod_poly, RingmodFilterSubType);
DEFINE_FILTER_NODE_TEMPLATE(allpass, allpass_poly, PhaseAllpassSubType);
DEFINE_FILTER_NODE_TEMPLATE(ladder, ladder_poly, LadderSubType);
DEFINE_FILTER_NODE_TEMPLATE(moog, moog_poly, MoogFilterSubType);
DEFINE_FILTER_NODE_TEMPLATE(svf_eq, svf_eq_poly, StateVariableEqSubType);
DEFINE_FILTER_NODE_TEMPLATE(linkwitzriley, linkwitzriley_poly, LinkwitzRiley);

#undef DEFINE_FILTER_NODE_TEMPLATE

template <int NV> struct fir_impl : public AudioFileNodeBase
{
	using CoefficientType = juce::dsp::FIR::Coefficients<float>;
	using FilterType = juce::dsp::FIR::Filter<float>;

	static constexpr int NumVoices = NV;

	SET_HISE_NODE_ID("fir");
	GET_SELF_AS_OBJECT();
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	fir_impl();;

	void prepare(PrepareSpecs specs) override;
	
	void rebuildCoefficients();

	void contentChanged() override;

	void reset();

	bool handleModulation(double&);
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

}

}
