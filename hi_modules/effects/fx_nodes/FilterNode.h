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

template <class FilterType> class FilterNodeBase : public HiseDspBase
{
public:

	static Identifier getStaticId() { return FilterType::getFilterTypeId(); }
	SET_HISE_NODE_EXTRA_HEIGHT(60);
	GET_SELF_AS_OBJECT(FilterNodeBase<FilterType>);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& parameters) override
	{
		{
			ParameterData p("Frequency");

			p.db = std::bind(&MultiChannelFilter<FilterType>::setFrequency, &filter, std::placeholders::_1);
			p.range = { 20.0, 20000.0, 0.1 };
			p.range.setSkewForCentre(1000.0);
			p.defaultValue = 1000.0;

			parameters.add(std::move(p));
		}

		{
			ParameterData p("Q");
			p.db = std::bind(&MultiChannelFilter<FilterType>::setQ, &filter, std::placeholders::_1);
			p.range = { 0.3, 9.9, 0.1 };
			p.range.setSkewForCentre(1.0);
			p.defaultValue = 1.0;

			parameters.add(std::move(p));
		}

		{
			ParameterData p("Gain");


			p.db = [this](double newValue)
			{
				filter.setGain(Decibels::decibelsToGain(newValue));
			};

			p.range = { -18, 18, 0.1 };
			p.range.setSkewForCentre(0.0);
			p.defaultValue = 0.0;

			parameters.add(std::move(p));
		}

		{
			ParameterData p("Smoothing");
			p.range = { 0.0, 1.0, 0.01 };
			p.range.setSkewForCentre(0.1);
			p.defaultValue = 0.01;
			p.db = std::bind(&MultiChannelFilter<FilterType>::setSmoothingTime, &filter, std::placeholders::_1);

			parameters.add(std::move(p));
		}

		{
			ParameterData p("Mode");
			p.setParameterValueNames(filter.getModes());
			
			p.db = [this](double newValue)
			{
				filter.setType((int)newValue);
			};

			parameters.add(std::move(p));
		}
	}

	struct Graph : public ExtraComponent<FilterNodeBase<FilterType>>
	{
		Graph(FilterNodeBase<FilterType>* d, PooledUIUpdater* h) :
			ExtraComponent<FilterNodeBase<FilterType>>(d, h),
			filterGraph(1)
		{
			lastCoefficients = this->getObject()->filter.getApproximateCoefficients();
			this->addAndMakeVisible(filterGraph);
			filterGraph.addFilter(hise::FilterType::HighPass);
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
			IIRCoefficients thisCoefficients = this->getObject()->filter.getApproximateCoefficients();

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

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return new Graph(this, updater);
	}

	bool handleModulation(double&) noexcept { return false; };

	forcedinline void prepare(PrepareSpecs ps) noexcept
	{
		sr = ps.sampleRate;
		filter.setNumChannels(ps.numChannels);
		filter.setSampleRate(ps.sampleRate);
	}

	forcedinline void process(ProcessData& d) noexcept
	{
		buffer.setDataToReferTo(d.data, d.numChannels, d.size);
		FilterHelpers::RenderData r(buffer, 0, d.size);
		filter.render(r);
	}

	void reset()
	{
		filter.reset();
	}

	void processSingle(float* frameData, int numChannels)
	{
		filter.processSingle(frameData, numChannels);
	}

	AudioSampleBuffer buffer;
	
	hise::MultiChannelFilter<FilterType> filter;

	double sr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FilterNodeBase);
};

namespace filters
{
using svf = FilterNodeBase<MultiChannelFilter<StateVariableFilterSubType>>;
using biquad = FilterNodeBase<MultiChannelFilter<StaticBiquadSubType>>;
using one_pole = FilterNodeBase<MultiChannelFilter<SimpleOnePoleSubType>>;
using ring_mod = FilterNodeBase<MultiChannelFilter<RingmodFilterSubType>>;
using allpass = FilterNodeBase<MultiChannelFilter<PhaseAllpassSubType>>;
using ladder = FilterNodeBase<MultiChannelFilter<LadderSubType>>;
using moog = FilterNodeBase<MultiChannelFilter<MoogFilterSubType>>;
using linkwitzriley = FilterNodeBase<MultiChannelFilter<LinkwitzRiley>>;

class FilterFactory : public NodeFactory
{
public:

	FilterFactory(DspNetwork* n) :
		NodeFactory(n)
	{
		registerNode<HiseDspNodeBase<filters::one_pole>>();
		registerNode< HiseDspNodeBase<svf>>();
		registerNode< HiseDspNodeBase<filters::biquad>>();
		registerNode< HiseDspNodeBase<filters::ladder>>();
		registerNode< HiseDspNodeBase<filters::ring_mod>>();
		registerNode< HiseDspNodeBase<filters::moog>>();
		registerNode< HiseDspNodeBase<filters::allpass>>();
		registerNode<HiseDspNodeBase<filters::linkwitzriley>>();
	};

	Identifier getId() const override { return "filters"; }
};

}



}
