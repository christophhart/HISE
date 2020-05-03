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

namespace scriptnode {

namespace filters
{
using namespace juce;
using namespace hise;


#define BIG 1


struct FilterNodeGraph : public ScriptnodeExtraComponent<CoefficientProvider>
{
	FilterNodeGraph(CoefficientProvider* d, PooledUIUpdater* h) :
		ScriptnodeExtraComponent<CoefficientProvider>(d, h),
		filterGraph(1)
	{
		lastCoefficients = {};
		this->addAndMakeVisible(filterGraph);
		filterGraph.addFilter(hise::FilterType::HighPass);

		setSize(256, 100);

		timerCallback();
	}

	static Component* createExtraComponent(CoefficientProvider* p, PooledUIUpdater* h)
	{
		return new FilterNodeGraph(p, h);
	}

	bool coefficientsChanged(const IIRCoefficients& first, const IIRCoefficients& second) const
	{
		for (int i = 0; i < 5; i++)
			if (first.coefficients[i] != second.coefficients[i])
				return true;

		return false;
	}

	void timerCallback() override
	{
		if (this->getObject() == nullptr)
			return;

		IIRCoefficients thisCoefficients = this->getObject()->getCoefficients();

		if (coefficientsChanged(lastCoefficients, thisCoefficients))
		{
			lastCoefficients = thisCoefficients;
			filterGraph.setCoefficients(0, this->getObject()->sr, thisCoefficients);
		}
	}

	void resized() override
	{
		if (this->getWidth() > 0)
			filterGraph.setBounds(this->getLocalBounds());
	}

	IIRCoefficients lastCoefficients;
	FilterGraph filterGraph;
};


template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setMode(double newMode)
{
	if (filter.isMonophonicOrInsideVoiceRendering())
		filter.get().setType((int)newMode);
	else
	{
		for(auto& f: filter)
			f.setType((int)newMode);
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setQ(double newQ)
{
	if (filter.isMonophonicOrInsideVoiceRendering())
		filter.get().setQ(newQ);
	else
	{
		for(auto& f: filter)
			f.setQ(newQ);
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setGain(double newGain)
{
	auto gainValue = Decibels::decibelsToGain(newGain);

	if (filter.isMonophonicOrInsideVoiceRendering())
		filter.get().setGain(gainValue);
	else
	{
		for (auto& f : filter)
			f.setGain(gainValue);
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setFrequency(double newFrequency)
{
	if (filter.isMonophonicOrInsideVoiceRendering())
		filter.get().setFrequency(newFrequency);
	else
	{
		for (auto& f : filter)
			f.setFrequency(newFrequency);
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::reset()
{
	if (filter.isMonophonicOrInsideVoiceRendering())
		filter.get().reset();
	else
	{
		for (auto& f : filter)
			f.reset();
	}
}

template <class FilterType, int NV>
IIRCoefficients FilterNodeBase<FilterType, NV>::getCoefficients()
{
	return filter.getCurrentOrFirst().getApproximateCoefficients();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::prepare(PrepareSpecs ps)
{
	sr = ps.sampleRate;

	auto c = ps.numChannels;
	auto s = ps.sampleRate;

	filter.prepare(ps);

	for(auto& f: filter)
	{
		f.setNumChannels(c);
		f.setSampleRate(s);
	};
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::createParameters(Array<ParameterData>& parameters)
{
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Frequency);
		p.range = { 20.0, 20000.0, 0.1 };
		p.range.setSkewForCentre(1000.0);
		p.defaultValue = 1000.0;
#if BIG
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setFrequency);
#endif
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Q);

#if BIG
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setQ);
#endif
		p.range = { 0.3, 9.9, 0.1 };
		p.range.setSkewForCentre(1.0);
		p.defaultValue = 1.0;
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Gain);
		p.range = { -18, 18, 0.1 };
		p.range.setSkewForCentre(0.0);
		p.defaultValue = 0.0;

#if BIG
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setGain);
#endif
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Smoothing);
		p.range = { 0.0, 1.0, 0.01 };
		p.range.setSkewForCentre(0.1);
		p.defaultValue = 0.01;

#if BIG
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setSmoothing);
#endif
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Mode);
		p.setParameterValueNames(filter.getCurrentOrFirst().getModes());
#if BIG
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setMode);
#endif
		parameters.add(std::move(p));
	}
}


template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setSmoothing(double newSmoothingTime)
{
	if (filter.isMonophonicOrInsideVoiceRendering())
		filter.get().setSmoothingTime(newSmoothingTime);
	else
	{
		for(auto& f: filter)
			f.setSmoothingTime(newSmoothingTime);
	}
}

#define DEFINE_FILTER_NODE_TEMPIMPL(className) template class FilterNodeBase<hise::MultiChannelFilter<className>, 1>; \
											template class FilterNodeBase<hise::MultiChannelFilter<className>, NUM_POLYPHONIC_VOICES>;

DEFINE_FILTER_NODE_TEMPIMPL(StateVariableFilterSubType);
DEFINE_FILTER_NODE_TEMPIMPL(StaticBiquadSubType);
DEFINE_FILTER_NODE_TEMPIMPL(SimpleOnePoleSubType);
DEFINE_FILTER_NODE_TEMPIMPL(RingmodFilterSubType);
DEFINE_FILTER_NODE_TEMPIMPL(PhaseAllpassSubType);
DEFINE_FILTER_NODE_TEMPIMPL(LadderSubType);
DEFINE_FILTER_NODE_TEMPIMPL(MoogFilterSubType);
DEFINE_FILTER_NODE_TEMPIMPL(StateVariableEqSubType);
DEFINE_FILTER_NODE_TEMPIMPL(LinkwitzRiley);

#undef DEFINE_FILTER_NODE_TEMPIMPL

template <int NV>
scriptnode::filters::fir_impl<NV>::fir_impl()
{

}

template <int NV>
void scriptnode::filters::fir_impl<NV>::rebuildCoefficients()
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

template <int NV>
void scriptnode::filters::fir_impl<NV>::contentChanged()
{
	AudioFileNodeBase::contentChanged();

	rebuildCoefficients();
}

template <int NV>
void scriptnode::filters::fir_impl<NV>::reset()
{
	SimpleReadWriteLock::ScopedReadLock sl(coefficientLock, true);

	if (leftFilters.isMonophonicOrInsideVoiceRendering())
	{
		leftFilters.get().reset();
		rightFilters.get().reset();
	}

	else
	{
		for (auto& f : leftFilters)
			f.reset();

		for (auto& f : rightFilters)
			f.reset();
	}
}

template <int NV>
bool scriptnode::filters::fir_impl<NV>::handleModulation(double&)
{
	return false;
}



template <int NV>
void scriptnode::filters::fir_impl<NV>::prepare(PrepareSpecs specs)
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

DEFINE_EXTERN_NODE_TEMPIMPL(fir_impl);

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
#if 1
	registerPolyNode<one_pole, one_pole_poly, FilterNodeGraph>();

	registerPolyNode<svf, svf_poly, FilterNodeGraph>();
	registerPolyNode<svf_eq, svf_eq_poly, FilterNodeGraph>();
	registerPolyNode<biquad, biquad_poly, FilterNodeGraph>();
	registerPolyNode<ladder, ladder_poly, FilterNodeGraph>();
	registerPolyNode<ring_mod, ring_mod_poly, FilterNodeGraph>();
	registerPolyNode<moog, moog_poly, FilterNodeGraph>();
	registerPolyNode<allpass, allpass_poly, FilterNodeGraph>();
	registerPolyNode<linkwitzriley, linkwitzriley_poly, FilterNodeGraph>();
	registerNode<convolution>();
	registerPolyNode<fir, fir_poly>();
#endif
}


}

}