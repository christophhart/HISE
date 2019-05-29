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


namespace core
{


namespace HarmonicEQ_impl
{
// Template Alias Definition =======================================================

using HarmonicEQ_ = fix<2, container::chain<core::gain, wrap::mod<core::midi>, filters::biquad, filters::biquad, filters::biquad, filters::biquad, dynamics::limiter>>;

struct instance : public hardcoded<instance, HarmonicEQ_, hc::no_modulation>
{
	SET_HISE_NODE_ID("HarmonicEQ");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "gain1");
		registerNode(get<1>(obj), "midi1");
		registerNode(get<2>(obj), "biquad1");
		registerNode(get<3>(obj), "biquad2");
		registerNode(get<4>(obj), "biquad3");
		registerNode(get<5>(obj), "biquad4");
		registerNode(get<6>(obj), "limiter1");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain1.Gain", -6.0);
		setParameterDefault("gain1.Smoothing", 1000.0);
		setParameterDefault("midi1.Mode", 3.0);
		setParameterDefault("biquad1.Frequency", 246.942);
		setParameterDefault("biquad1.Q", 9.9);
		setParameterDefault("biquad1.Gain", 18.0);
		setParameterDefault("biquad1.Smoothing", 0.125123);
		setParameterDefault("biquad1.Mode", 4.0);
		setParameterDefault("biquad2.Frequency", 474.11);
		setParameterDefault("biquad2.Q", 9.9);
		setParameterDefault("biquad2.Gain", 18.0);
		setParameterDefault("biquad2.Smoothing", 0.125123);
		setParameterDefault("biquad2.Mode", 4.0);
		setParameterDefault("biquad3.Frequency", 701.279);
		setParameterDefault("biquad3.Q", 9.9);
		setParameterDefault("biquad3.Gain", 18.0);
		setParameterDefault("biquad3.Smoothing", 0.125123);
		setParameterDefault("biquad3.Mode", 4.0);
		setParameterDefault("biquad4.Frequency", 928.448);
		setParameterDefault("biquad4.Q", 9.9);
		setParameterDefault("biquad4.Gain", 18.0);
		setParameterDefault("biquad4.Smoothing", 0.125123);
		setParameterDefault("biquad4.Mode", 4.0);
		setParameterDefault("limiter1.Threshhold", 0.0);
		setParameterDefault("limiter1.Attack", 5.2);
		setParameterDefault("limiter1.Release", 50.0);
		setParameterDefault("limiter1.Ratio", 1.0);

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("biquad1.Frequency", { 20.0, 20000.0, 0.1, 1.0 });
			auto mod_target2 = getParameter("biquad2.Frequency", { 20.0, 40000.0, 0.1, 1.0 });
			auto mod_target3 = getParameter("biquad3.Frequency", { 20.0, 60000.0, 0.1, 1.0 });
			auto mod_target4 = getParameter("biquad4.Frequency", { 20.0, 80000.0, 0.1, 1.0 });
			auto f = [mod_target1, mod_target2, mod_target3, mod_target4](double newValue)
			{
				mod_target1(newValue);
				mod_target2(newValue);
				mod_target3(newValue);
				mod_target4(newValue);
			};

			setInternalModulationParameter(get<1>(obj), f);
		}

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Q", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("biquad1.Q", { 0.3, 9.9, 0.1, 0.264718 });
			auto param_target2 = getParameter("biquad2.Q", { 0.3, 9.9, 0.1, 0.264718 });
			auto param_target3 = getParameter("biquad3.Q", { 0.3, 9.9, 0.1, 0.264718 });
			auto param_target4 = getParameter("biquad4.Q", { 0.3, 9.9, 0.1, 0.264718 });

			p.db = [param_target1, param_target2, param_target3, param_target4, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
				param_target3(normalised);
				param_target4(normalised);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Gain", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("biquad1.Gain", { 0.0, 18.0, 0.1, 1.0 });
			auto param_target2 = getParameter("biquad2.Gain", { 0.0, 18.0, 0.1, 1.0 });
			auto param_target3 = getParameter("biquad3.Gain", { 0.0, 18.0, 0.1, 1.0 });
			auto param_target4 = getParameter("biquad4.Gain", { 0.0, 18.0, 0.1, 1.0 });
			auto param_target5 = getParameter("gain1.Gain", { -6.0, 0.0, 0.1, 5.42227 });

			p.db = [param_target1, param_target2, param_target3, param_target4, param_target5, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
				param_target3(normalised);
				param_target4(normalised);
				param_target5(normalised);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Smoothing", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("biquad2.Smoothing", { 0.01, 0.2, 0.01, 0.30103 });
			auto param_target2 = getParameter("biquad3.Smoothing", { 0.01, 0.2, 0.01, 0.30103 });
			auto param_target3 = getParameter("biquad4.Smoothing", { 0.01, 0.2, 0.01, 0.30103 });
			auto param_target4 = getParameter("biquad1.Smoothing", { 0.01, 0.2, 0.01, 0.30103 });

			p.db = [param_target1, param_target2, param_target3, param_target4, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
				param_target3(normalised);
				param_target4(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using HarmonicEQ = HarmonicEQ_impl::instance;



}

HiseFxNodeFactory::HiseFxNodeFactory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<HiseDspNodeBase<core::seq>>();
	registerNode<HiseDspNodeBase<core::simple_saw>>();
	registerNode<HiseDspNodeBase<core::table>>();
	registerNode<HiseDspNodeBase<core::chain1>>();
	registerNode<HiseDspNodeBase<core::fix_delay>>();
	registerNode<HiseDspNodeBase<core::delayTest>>();
	registerNode<HiseDspNodeBase<core::oscillator>>();
	registerNode<HiseDspNodeBase<core::HarmonicEQ>>();
	registerNode<HiseDspNodeBase<core::gain>>();
	registerNode<HiseDspNodeBase<core::fix_panner>>();
	registerNode<HiseDspNodeBase<core::peak>>();
	registerNode<HiseDspNodeBase<core::tempo_sync>>();
	registerNode<HiseDspNodeBase<core::timer>>();
	registerNode<HiseDspNodeBase<core::midi>>({});
}

}