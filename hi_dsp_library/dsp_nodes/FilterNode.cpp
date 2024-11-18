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




template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setMode(double newMode)
{
	for (auto& f : filter)
		f.setType((int)newMode);

	this->sendCoefficientUpdateMessage();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setQ(double newQ)
{
	for (auto& f : filter)
		f.setQ(newQ);

	this->sendCoefficientUpdateMessage();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setGain(double newGain)
{
	auto gainValue = Decibels::decibelsToGain(newGain);

	for (auto& f : filter)
		f.setGain(gainValue);

	this->sendCoefficientUpdateMessage();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setFrequency(double newFrequency)
{
	for (auto& f : filter)
		f.setFrequency(newFrequency);

	this->sendCoefficientUpdateMessage();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setEnabled(double isEnabled)
{
	enabled = isEnabled > 0.5;
	this->sendCoefficientUpdateMessage();
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::reset()
{
	for (auto& f : filter)
		f.reset();
}


template <class FilterType, int NV>
FilterDataObject::CoefficientData FilterNodeBase<FilterType, NV>::getApproximateCoefficients() const
{
	if (!enabled)
		return {};

	return filter.getFirst().getApproximateCoefficients();
}

template <class FilterType, int NV>
void scriptnode::filters::FilterNodeBase<FilterType, NV>::onComplexDataEvent(hise::ComplexDataUIUpdaterBase::EventType e, var newValue)
{
	
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::prepare(PrepareSpecs ps)
{
	auto c = ps.numChannels;
	auto s = ps.sampleRate;
	sr = s;

	filter.prepare(ps);

	for(auto& f: filter)
	{
		f.setNumChannels(c);
		f.setSampleRate(s);
	};

	if (auto fd = dynamic_cast<FilterDataObject*>(this->externalData.obj))
		fd->setSampleRate(ps.sampleRate);
}

template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::createParameters(ParameterDataList& parameters)
{
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Frequency);
		p.setRange({ 20.0, 20000.0});
		p.setSkewForCentre(1000.0);
		p.setDefaultValue(1000.0);
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Q);

		p.setRange({ 0.3, 9.9});
		p.setSkewForCentre(1.0);
		p.setDefaultValue(1.0);
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Gain);
		p.setRange({ -18, 18 });
		p.setSkewForCentre(0.0);
		p.setDefaultValue(0.0);
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Smoothing);
		p.setSkewForCentre(0.1);
		p.setDefaultValue(0.01);
		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Mode);

		auto& f = filter.getFirst();
		p.setParameterValueNames(f.getModes());

		parameters.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(FilterNodeBase, Enabled);
		p.setParameterValueNames({ "Off", "On" });
		p.setDefaultValue(1.0);

		parameters.add(std::move(p));
	}
}


template <class FilterType, int NV>
void FilterNodeBase<FilterType, NV>::setSmoothing(double newSmoothingTime)
{
	for (auto& f : filter)
		f.setSmoothingTime(newSmoothingTime);
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


}

}