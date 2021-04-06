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
	registerNode<wrap::data<fft, data::dynamic::displaybuffer>, ui::fft_display>();
	registerNode<wrap::data<oscilloscope, data::dynamic::displaybuffer>, ui::osc_display>();
	registerNode<wrap::data<goniometer, data::dynamic::displaybuffer>, ui::gonio_display>();
}

}


namespace dynamics
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerModNode<dp<gate>, data::ui::displaybuffer_editor>();
	registerModNode<dp<comp>, data::ui::displaybuffer_editor >();
	registerModNode<dp<limiter>, data::ui::displaybuffer_editor >();
	registerModNode<dp<envelope_follower>, data::ui::displaybuffer_editor >();
}

}

namespace fx
{

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<reverb>();
	registerPolyNode<sampleandhold, sampleandhold_poly>();
	registerPolyNode<bitcrush, bitcrush_poly>();
	registerPolyNode<wrap::fix<2, haas>, wrap::fix<2, haas_poly>>();
	registerPolyNode<phase_delay, phase_delay_poly>();
}

}

namespace stk_factory
{
Factory::Factory(DspNetwork* n):
	NodeFactory(n)
{
	registerNode<stk::nodes::JCRev>();
	registerNode<stk::nodes::DelayA>();
	registerNode<stk::nodes::BiQuad>();
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

namespace control
{
	using dynamic_cable_table = wrap::data<control::cable_table<parameter::dynamic_base_holder>, data::dynamic::table>;
	using dynamic_cable_pack = wrap::data<control::cable_pack<parameter::dynamic_base_holder>, data::dynamic::sliderpack>;

	using dynamic_smoother_parameter = control::smoothed_parameter<smoothers::dynamic>;

	Factory::Factory(DspNetwork* network) :
		NodeFactory(network)
	{
		registerNoProcessNode<pma_editor::NodeType, pma_editor>();
		registerNoProcessNode<control::sliderbank_editor::NodeType, control::sliderbank_editor, false>();
		registerNoProcessNode<dynamic_cable_pack, data::ui::sliderpack_editor>();
		registerNoProcessNode<dynamic_cable_table, data::ui::table_editor>();
		
		registerNoProcessNode<parameter::dynamic_duplispread_node, parameter::duplispread_editor>();

		registerNoProcessNode<faders::dynamic::NodeType, faders::dynamic::editor>();
		registerModNode<midi_logic::dynamic::NodeType, midi_logic::dynamic::editor>();
		registerModNode<smoothers::dynamic::NodeType, smoothers::dynamic::editor>();

		registerPolyModNode<control::timer<snex_timer>, timer_poly<snex_timer>, snex_timer::editor>();

		registerNodeRaw<InterpretedUnisonoWrapperNode>();
	}
}

namespace core
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	

	registerNode<fix_delay>();
	registerNode<fm>();
	
	registerModNode<wrap::data<core::table,       data::dynamic::table>,     data::ui::table_editor>();
	registerModNode<wrap::data<core::file_player, data::dynamic::audiofile>, data::ui::audiofile_editor_with_mod>();
	registerNode   <wrap::data<core::recorder,    data::dynamic::audiofile>, data::ui::audiofile_editor>();

	registerPolyNode<gain, gain_poly>();

	registerModNode<tempo_sync, TempoDisplay>();

#if HISE_INCLUDE_SNEX
	registerPolyNode<snex_osc<SnexOscillator>, snex_osc_poly<SnexOscillator>, NewSnexOscillatorDisplay>();
	registerNode<core::snex_node, core::snex_node::editor>();
#endif

	registerModNode<hise_mod>();
	
	registerModNode<dp<peak>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<ramp>, dp<ramp_poly>, data::ui::displaybuffer_editor>();

	registerNode<core::mono2stereo>();
	registerPolyNode<core::oscillator, core::oscillator_poly, OscDisplay>();
	registerNode<waveshapers::dynamic::NodeType, waveshapers::dynamic::editor>();
}
}

namespace filters
{
template <typename FilterType> using df = wrap::data<FilterType, data::dynamic::filter>;

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	using namespace data::ui;

	registerPolyNode<df<one_pole>,		df<one_pole_poly>,		filter_editor>();
	registerPolyNode<df<svf>,			df<svf_poly>,			filter_editor>();
	registerPolyNode<df<svf_eq>,		df<svf_eq_poly>,		filter_editor>();
	registerPolyNode<df<biquad>,		df<biquad_poly>,		filter_editor>();
	registerPolyNode<df<ladder>,		df<ladder_poly>,		filter_editor>();
	registerPolyNode<df<ring_mod>,		df<ring_mod_poly>,		filter_editor>();
	registerPolyNode<df<moog>,			df<moog_poly>,			filter_editor>();
	registerPolyNode<df<allpass>,		df<allpass_poly>,		filter_editor>();
	registerPolyNode<df<linkwitzriley>,	df<linkwitzriley_poly>, filter_editor>();

	registerNode<wrap::data<convolution, data::dynamic::audiofile>, data::ui::audiofile_editor>();
	//registerPolyNode<fir, fir_poly>();
}
}

}
