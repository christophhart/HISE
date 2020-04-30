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

namespace scriptnode
{

using namespace juce;
using namespace hise;

namespace analyse
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
#if NOT_JUST_OSC
	registerNode<fft>({});
	registerNode<oscilloscope>({});
#endif
}

}


namespace dynamics
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
#if NOT_JUST_OSC
	registerNode<gate, ModulationSourcePlotter>();
	registerNode<comp, ModulationSourcePlotter>();
	registerNode<limiter, ModulationSourcePlotter>();
	registerNode<envelope_follower, ModulationSourcePlotter>();
#endif
}

}

namespace fx
{

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
#if NOT_JUST_OSC
	registerPolyNode<sampleandhold, sampleandhold_poly>({});
	registerPolyNode<bitcrush, bitcrush_poly>({});
	registerPolyNode<fix<2, haas>, fix<2, haas_poly>>({});
	registerPolyNode<phase_delay, phase_delay_poly>({});
	registerNode<reverb>({});
#endif
}

}

namespace core
{


Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
#if NOT_JUST_OSC
#if INCLUDE_SOUL_NODE
	registerNodeRaw<SoulNode>();
#endif

	registerPolyNode<seq, seq_poly>();
	
#if HISE_INCLUDE_SNEX
	registerPolyNodeRaw<JitNode, JitPolyNode>();
	registerNode<core::simple_jit>({});
#endif
	registerNode<mono2stereo>({});
	registerNode<table>();
	registerNode<fix_delay>();
	registerNode<file_player>();
	
	registerNode<fm>();
	
	registerPolyNode<ramp_envelope, ramp_envelope_poly, ModulationSourcePlotter>();
	registerPolyNode<gain, gain_poly>();
	
	
	registerPolyNode<timer, timer_poly, TimerDisplay>();
	registerPolyNode<midi, midi_poly, MidiDisplay>({});
	registerPolyNode<smoother, smoother_poly>({});
#endif

	registerNodeRaw<ParameterMultiplyAddNode<core::pma<parameter::dynamic_base_holder>>>();

	registerModNode<hise_mod>();

	registerModNode<tempo_sync, TempoDisplay>();

	registerNode<routing2::send<cable::dynamic>, FunkySendComponent>();
	registerNode<routing2::receive<cable::dynamic>, FunkySendComponent>();

	registerModNode<peak>();
	registerPolyNode<ramp, ramp_poly>();

#if 0

	oscillator_impl<1> o;

	void* obj = &o;

	parameter::data_pool pool;

	

	parameter::dynamic_chain chain;

	Array<ParameterDataImpl> data;

	o.createParameters(data);

	auto& fp = data.getReference(0);



	chain.addParameter(new parameter::dynamic_from0to1(fp.dbNew, pool.create(fp.range)));
	
	String code = "Math.max(input, 0.5) * 1041.0";

	chain.addParameter(new parameter::dynamic_expression(data.getReference(1).dbNew, pool.create(code)));

	chain(1.0);

	parameter::dynamic wrappedChain;

	



	wrappedChain = std::move(chain);

	parameter::dynamic_to0to1 normaliser(wrappedChain, pool.create(NormalisableRange<double>(20.0, 40.0)));

	parameter::dynamic wrappedNormaliser;

	wrappedNormaliser = std::move(normaliser);

	constexpr int funky3 = sizeof(chain);
	constexpr int funky = sizeof(normaliser);

	wrappedNormaliser(30.0);

	

	//using bo = bypass::smoothed<oscillator>;

	//bo o;

	//parameter::bypass<bo> p;

	//p.connect<0>(o);

	//p.call(0.7);

	using A = HiseDspNodeBase<oscillator, OscDisplay>;
	using B = HiseDspNodeBase<oscillator_poly, OscDisplay>;
	
	//A o(network, {});
	//B o2(network, {});
#endif


	registerPolyNode<oscillator, oscillator_poly, OscDisplay>();

}
}



}
