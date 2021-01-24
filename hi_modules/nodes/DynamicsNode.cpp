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
using namespace juce;
using namespace hise;

namespace dynamics
{

struct DynamicHelpers
{
	static Identifier getId(chunkware_simple::SimpleGate*)
	{
		RETURN_STATIC_IDENTIFIER("gate");
	}

	static Identifier getId(chunkware_simple::SimpleComp*)
	{
		RETURN_STATIC_IDENTIFIER("comp");
	}

	static Identifier getId(chunkware_simple::SimpleCompRms*)
	{
		RETURN_STATIC_IDENTIFIER("comp_rms");
	}

	static Identifier getId(chunkware_simple::SimpleLimit*)
	{
		RETURN_STATIC_IDENTIFIER("limiter");
	}
};


template <class DynamicProcessorType>
Identifier dynamics_wrapper<DynamicProcessorType>::getStaticId()
{
	DynamicProcessorType* t = nullptr;
	return DynamicHelpers::getId(t);
}


template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::setRatio(double v)
{
	auto ratio = (v != 0.0) ? 1.0 / v : 1.0;
	obj.setRatio(ratio);
}

template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::setRelease(double v)
{
	obj.setRelease(v);
}

template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::setAttack(double v)
{
	obj.setAttack(v);
}

template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::setThreshhold(double v)
{
	obj.setThresh(v);
}

template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(dynamics_wrapper, Threshhold);
		p.setRange({ -100.0, 0.0, 0.1 });
		p.setSkewForCentre(-12.0);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(dynamics_wrapper, Attack);
		p.setRange({ 0.0, 250.0, 0.1 });
		p.setSkewForCentre(50.0);
		p.setDefaultValue(50.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(dynamics_wrapper, Release);
		p.setRange({ 0.0, 250.0, 0.1 });
		p.setSkewForCentre(50.0);
		p.setDefaultValue(50.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(dynamics_wrapper, Ratio);
		p.setRange({ 1.0, 32.0, 0.1 });
		p.setSkewForCentre(4.0);
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
}

template <class DynamicProcessorType>
bool dynamics_wrapper<DynamicProcessorType>::handleModulation(double& max) noexcept
{
	max = jlimit(0.0, 1.0, 1.0 - obj.getGainReduction());
	return true;
}

template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::prepare(PrepareSpecs ps)
{
	obj.setSampleRate(ps.sampleRate);
}

template <class DynamicProcessorType>
void dynamics_wrapper<DynamicProcessorType>::reset() noexcept
{
	obj.initRuntime();
}

template <class DynamicProcessorType>
dynamics_wrapper<DynamicProcessorType>::dynamics_wrapper()
{

}

DEFINE_EXTERN_MONO_TEMPIMPL(dynamics_wrapper<chunkware_simple::SimpleGate>);
DEFINE_EXTERN_MONO_TEMPIMPL(dynamics_wrapper<chunkware_simple::SimpleComp>);
DEFINE_EXTERN_MONO_TEMPIMPL(dynamics_wrapper<chunkware_simple::SimpleLimit>);

envelope_follower::envelope_follower() :
	envelope(20.0, 50.0)
{

}

void envelope_follower::prepare(PrepareSpecs ps)
{
	envelope.setSampleRate(ps.sampleRate);
}

void envelope_follower::reset() noexcept
{
	envelope.reset();
}

void envelope_follower::setAttack(double v)
{
	envelope.setAttackDouble(v);
}

void envelope_follower::setRelease(double v)
{
	envelope.setReleaseDouble(v);
}

void envelope_follower::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(envelope_follower, Attack);
		p.setRange({ 0.0, 1000.0, 0.1 });
		p.setSkewForCentre(50.0);
		p.setDefaultValue(20.0);

		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(envelope_follower, Release);
		p.setRange({ 0.0, 1000.0, 0.1 });
		p.setSkewForCentre(50.0);
		p.setDefaultValue(20.0);

		data.add(std::move(p));
	}
}
    
}
    
}
