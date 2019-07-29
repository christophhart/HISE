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
};

template <class FilterType, int NV> 
class FilterNodeBase : public HiseDspBase,
					   public CoefficientProvider
{
public:

	using FilterObject = FilterType;

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID(FilterType::getFilterTypeId());
	SET_HISE_NODE_EXTRA_HEIGHT(60);
	GET_SELF_AS_OBJECT(FilterNodeBase);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& parameters) override;
	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void prepare(PrepareSpecs ps) noexcept;
	void reset();
	void process(ProcessData& d) noexcept;
	void processSingle(float* frameData, int numChannels);
	bool handleModulation(double&) noexcept { return false; };

	IIRCoefficients getCoefficients() override;
	void setFrequency(double newFrequency);
	void setGain(double newGain);
	void setQ(double newQ);
	void setMode(double newMode);
	void setSmoothingTime(double newSmoothingTime);

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
DEFINE_FILTER_NODE_TEMPLATE(linkwitzriley, linkwitzriley_poly, LinkwitzRiley);

#undef DEFINE_FILTER_NODE_TEMPLATE

}

}
