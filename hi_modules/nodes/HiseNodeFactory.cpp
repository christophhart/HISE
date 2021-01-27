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
	registerNode<reverb>();

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
	registerPolyNode<add, add_poly>();

	registerNode<clear>();

	registerPolyNode<tanh, tanh_poly>();

	registerPolyNode<mul, mul_poly>();

	
	registerPolyNode<sub, sub_poly>();
	registerPolyNode<div, div_poly>();
	
	registerPolyNode<clip, clip_poly>();
	registerNode<sin>();
	registerNode<pi>();
	registerNode<sig2mod>();
	registerNode<abs>();
	registerNode<square>();
	registerNode<sqrt>();
	registerNode<pow>();

	sortEntries();
}
}

namespace core
{

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerPolyNode<seq, seq_poly>();
	
	

	registerModNode<core::dynamic_table::TableNodeType, core::dynamic_table::display>();
	registerNode<fix_delay>();
	registerNode<file_player>();
	
	registerNode<fm>();
	
	registerPolyNode<gain, gain_poly>();
	//registerPolyNode<smoother, smoother_poly>();
	registerNode<new_jit>();
	registerModNode<core::midi<DynamicMidiEventProcessor>, MidiDisplay>();
	registerPolyModNode<timer<SnexEventTimer>, timer_poly<SnexEventTimer>, TimerDisplay>();
	registerPolyNode<snex_osc<SnexOscillator>, snex_osc_poly<SnexOscillator>, SnexOscillatorDisplay>();
	registerNodeRaw<ParameterMultiplyAddNode>();
	registerModNode<hise_mod>();
	registerModNode<tempo_sync, TempoDisplay>();
	registerModNode<peak>();
	registerPolyNode<ramp, ramp_poly>();
	registerNode<core::mono2stereo>();
	registerPolyNode<core::oscillator, core::oscillator_poly, OscDisplay>();

	

}
}

namespace filters
{

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
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
	//registerPolyNode<fir, fir_poly>();
}
}

}
