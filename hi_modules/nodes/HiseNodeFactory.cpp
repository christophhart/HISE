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
	registerNodeCustomSNEX<reverb>({});

#if NOT_JUST_OSC
	registerPolyNode<sampleandhold, sampleandhold_poly>({});
	registerPolyNode<bitcrush, bitcrush_poly>({});
	registerPolyNode<fix<2, haas>, fix<2, haas_poly>>({});
	registerPolyNode<phase_delay, phase_delay_poly>({});
	
#endif
}

}

namespace math
{

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{

	registerPolyNodeCustomSNEX<add, add_poly>();

#if NOT_JUST_OSC
	registerPolyNode<mul, mul_poly>();

	registerNode<clear>();
	registerPolyNode<sub, sub_poly>();
	registerPolyNode<div, div_poly>();
	registerPolyNode<tanh, tanh_poly>();
	registerPolyNode<clip, clip_poly>();
	registerNode<sin>();
	registerNode<pi>();
	registerNode<sig2mod>();
	registerNode<abs>();
	registerNode<square>();
	registerNode<sqrt>();
	registerNode<pow>();
#endif

	sortEntries();
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
	
	registerNode<mono2stereo>({});
	registerNode<table>();
	registerNode<fix_delay>();
	registerNode<file_player>();
	
	registerNode<fm>();
	
	registerPolyNode<ramp_envelope, ramp_envelope_poly, ModulationSourcePlotter>();
	registerPolyNode<gain, gain_poly>();
	
	
	
	
	registerPolyNode<smoother, smoother_poly>({});
#endif

	registerNode<new_jit>();

	registerModNode<core::midi<SnexEventProcessor>, MidiDisplay>();

	registerPolyModNode<timer<SnexEventTimer>, timer_poly<SnexEventTimer>, TimerDisplay>();

	registerPolyNode<snex_osc<SnexOscillator>, snex_osc_poly<SnexOscillator>, SnexOscillatorDisplay>();

	//registerPolyNode<midi, midi_poly, MidiDisplay>({});

	registerNodeRaw<ParameterMultiplyAddNode<core::pma<parameter::dynamic_base_holder, 1>>>();

	registerModNode<hise_mod>();

	registerModNode<tempo_sync, TempoDisplay>();

	registerModNode<peak>();
	registerPolyNode<ramp, ramp_poly>();
	
	registerPolyNode<core::oscillator, core::oscillator_poly, OscDisplay>();

}
}



}
