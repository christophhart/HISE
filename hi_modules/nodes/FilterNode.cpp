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


struct FilterNodeGraph : public HiseDspBase::ExtraComponent<CoefficientProvider>
{
	FilterNodeGraph(CoefficientProvider* d, PooledUIUpdater* h) :
		ExtraComponent<CoefficientProvider>(d, h),
		filterGraph(1)
	{
		lastCoefficients = {};
		this->addAndMakeVisible(filterGraph);
		filterGraph.addFilter(hise::FilterType::HighPass);

		timerCallback();
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
	if (filter.isVoiceRenderingActive())
		filter.get().setType((int)newMode);
	else
	{
		filter.forEachVoice([newMode](FilterObject& f)
		{
			f.setType((int)newMode);
		});
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setQ(double newQ)
{
	if (filter.isVoiceRenderingActive())
		filter.get().setQ(newQ);
	else
	{
		filter.forEachVoice([newQ](FilterObject& f)
		{
			f.setQ(newQ);
		});
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setGain(double newGain)
{
	auto gainValue = Decibels::decibelsToGain(newGain);

	if (filter.isVoiceRenderingActive())
		filter.get().setGain(gainValue);
	else
	{
		filter.forEachVoice([gainValue](FilterObject& f)
		{
			f.setGain(gainValue);
		});
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setFrequency(double newFrequency)
{
	if (filter.isVoiceRenderingActive())
		filter.get().setFrequency(newFrequency);
	else
	{
		filter.forEachVoice([newFrequency](FilterObject& f)
		{
			f.setFrequency(newFrequency);
		});
	}
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::processSingle(float* frameData, int numChannels)
{
	filter.get().processSingle(frameData, numChannels);
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::process(ProcessData& d) noexcept
{
	buffer.setDataToReferTo(d.data, d.numChannels, d.size);
	FilterHelpers::RenderData r(buffer, 0, d.size);
	filter.get().render(r);
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::reset()
{
	if (filter.isVoiceRenderingActive())
		filter.get().reset();
	else
		filter.forEachVoice([](FilterObject& f) { f.reset(); });
}

template <class FilterType, int NV>
IIRCoefficients FilterNodeBase<FilterType, NV>::getCoefficients()
{
	return filter.getCurrentOrFirst().getApproximateCoefficients();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::prepare(PrepareSpecs ps) noexcept
{
	sr = ps.sampleRate;

	int c = ps.numChannels;
	int s = ps.sampleRate;

	filter.prepare(ps);

	filter.forEachVoice([c, s](FilterObject& f)
	{
		f.setNumChannels(c);
		f.setSampleRate(s);
	});
}

template <class FilterType, int NV>
Component* FilterNodeBase<FilterType, NV>::createExtraComponent(PooledUIUpdater* updater)
{
	return new FilterNodeGraph(this, updater);
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::createParameters(Array<ParameterData>& parameters)
{
	{
		ParameterData p("Frequency");
		p.range = { 20.0, 20000.0, 0.1 };
		p.range.setSkewForCentre(1000.0);
		p.defaultValue = 1000.0;
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setFrequency);
		parameters.add(std::move(p));
	}
	{
		ParameterData p("Q");
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setQ);
		p.range = { 0.3, 9.9, 0.1 };
		p.range.setSkewForCentre(1.0);
		p.defaultValue = 1.0;
		parameters.add(std::move(p));
	}
	{
		ParameterData p("Gain");
		p.range = { -18, 18, 0.1 };
		p.range.setSkewForCentre(0.0);
		p.defaultValue = 0.0;
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setGain);
		parameters.add(std::move(p));
	}
	{
		ParameterData p("Smoothing");
		p.range = { 0.0, 1.0, 0.01 };
		p.range.setSkewForCentre(0.1);
		p.defaultValue = 0.01;
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setSmoothingTime);
		parameters.add(std::move(p));
	}
	{
		ParameterData p("Mode");
		p.setParameterValueNames(filter.getCurrentOrFirst().getModes());
		p.db = BIND_MEMBER_FUNCTION_1(FilterNodeBase::setMode);
		parameters.add(std::move(p));
	}
}


template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setSmoothingTime(double newSmoothingTime)
{
	if (filter.isVoiceRenderingActive())
		filter.get().setSmoothingTime(newSmoothingTime);
	else
	{
		filter.forEachVoice([newSmoothingTime](FilterObject& f)
		{
			f.setSmoothingTime(newSmoothingTime);
		});
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
DEFINE_FILTER_NODE_TEMPIMPL(LinkwitzRiley);

#undef DEFINE_FILTER_NODE_TEMPIMPL

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	registerPolyNode<one_pole, one_pole_poly>();
	registerPolyNode<svf, svf_poly>();
	registerPolyNode<biquad, biquad_poly>();
	registerPolyNode<ladder, ladder_poly>();
	registerPolyNode<ring_mod, ring_mod_poly>();
	registerPolyNode<moog, moog_poly>();
	registerPolyNode<allpass, allpass_poly>();
	registerPolyNode<linkwitzriley, linkwitzriley_poly>();
}


}

}