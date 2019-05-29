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

#pragma once;

namespace scriptnode {
using namespace juce;
using namespace hise;



#define CHAIN(x) wr::multi::chain<x>


namespace core
{

#if 0
namespace chain1_impl
{

using chain1_ = wr::one::frame<2, wr::multi::chain<wr::one::mod<core::simple_saw>, math::mul>>;

struct instance : public wr::one::parameter<chain1_>
{
	// Node Definitions ============================================================
	SET_HISE_NODE_ID("chain1");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	// Interface methods ===========================================================

	

	void createParameters(Array<ParameterData>& data) override
	{
		fillInternalParameterList(get<0>(obj), "osc1");
		fillInternalParameterList(get<1>(obj), "mul1");

		// Static Parameter Initalisation =================================================
		
		initValues.add({ "osc1.PeriodTime", 1000.0 });
		initValues.add({ "mul1.Value", 1.0 });
		

		initStaticParameterData();

		// Internal Modulation Initialisation

		
		

		/*
		// Initialise internal modulation
		{
			auto p = getParameter(ip, "1.Frequency");
			NormalisableRange<double> r = { 100.0, 200.0, 0.1 };
			obj.getObject().get<0>().setCallback(DspHelpers::getFunctionFrom0To1ForRange(r, false, p.db));
		}
		*/

		
		

	}
};

}

using chain1 = chain1_impl::instance;
#endif


namespace delayTest_impl
{
// Template Alias Definition =======================================================
using delayTest_ = wrap::frame<2, container::split<core::gain, core::fix_delay>>;

struct instance : public hardcoded<instance, delayTest_, hc::no_modulation>
{
	SET_HISE_NODE_ID("delayTest");

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "gain3");
		registerNode(get<1>(obj), "fix_delay2");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain3.Gain", 0.0);
		setParameterDefault("gain3.Smoothing", 20.0);
		setParameterDefault("fix_delay2.DelayTime", 183.3);
		setParameterDefault("fix_delay2.FadeTime", 512.0);
	}

};

}

using delayTest = delayTest_impl::instance;











namespace panner_impl
{
// Template Alias Definition =======================================================

using panner_ = wrap::frame<2, container::multi<fix<1, core::gain>, fix<1, core::gain>>>;

struct instance : public no_mod<instance, panner_>
{
	SET_HISE_NODE_ID("panner");

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "gain2");
		registerNode(get<1>(obj), "gain3");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain2.Gain", -6.0206);
		setParameterDefault("gain2.Smoothing", 20.0);
		setParameterDefault("gain3.Gain", -6.0206);
		setParameterDefault("gain3.Smoothing", 20.0);

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Balance", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });
			auto param_target2 = getParameter("gain3.Gain", { -100.0, 0.0, 0.1, 5.42227 });

			param_target1.addConversion(ConverterIds::DryAmount);
			param_target2.addConversion(ConverterIds::WetAmount);

			p.db = [param_target1, param_target2, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
			};

			

			data.add(std::move(p));
		}
	}

};

}

using panner = panner_impl::instance;





namespace fix_panner_impl
{
// Template Alias Definition =======================================================
using fix_panner_ = wrap::frame<2, container::multi<fix<1, math::add>, fix<1, math::mul>>>;

struct instance : public hardcoded<instance, fix_panner_, hc::no_modulation>
{
	SET_HISE_NODE_ID("fix_panner");

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "mul1");
		registerNode(get<1>(obj), "mul2");

		// Parameter Initalisation =========================================================
		setParameterDefault("mul1.Value", 0.52);
		setParameterDefault("mul2.Value", 0.48);

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Balance", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("mul1.Value", { 0.0, 1.0, 0.01, 1.0 });
			auto param_target2 = getParameter("mul2.Value", { 0.0, 1.0, 0.01, 1.0 });

			param_target1.addConversion(ConverterIds::SubtractFromOne);

			p.db = [param_target1, param_target2, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using fix_panner = fix_panner_impl::instance;




namespace feedback2_impl
{
// Template Alias Definition =======================================================
using feedback2_ = container::feedback<core::fix_delay, core::gain>;

struct instance : public hardcoded<instance, feedback2_, hc::no_modulation>
{
	SET_HISE_NODE_ID("feedback2");

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "fix_delay1");
		registerNode(get<1>(obj), "gain3");

		// Parameter Initalisation =========================================================
		setParameterDefault("fix_delay1.DelayTime", 196.1);
		setParameterDefault("fix_delay1.FadeTime", 512.0);
		setParameterDefault("gain3.Gain", -9.02864);
		setParameterDefault("gain3.Smoothing", 20.0);
	}

};

}

using feedback2 = feedback2_impl::instance;




namespace chain1_impl
{
// Template Alias Definition =======================================================
using chain1_ = container::chain<wrap::mod<core::peak>, core::gain>;

struct instance : public no_mod<instance, chain1_>
{
	SET_HISE_NODE_ID("chain1");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "peak1");
		registerNode(get<1>(obj), "gain2");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain2.Gain", -100.0);
		setParameterDefault("gain2.Smoothing", 20.0);

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });
			auto f = [mod_target1](double newValue)
			{
				mod_target1(newValue);
			};

			setInternalModulationParameter(get<0>(obj), f);
		}
	}

};

}

using chain1 = chain1_impl::instance;


#if 0

namespace chain1_impl
{
// Template Alias Definition =======================================================
using chain1_ = container::chain<bypass::smoothed<core::oscillator>, bypass::smoothed<core::oscillator>, core::gain>;

struct instance : public wr::one::parameter<chain1_>
{
	SET_HISE_NODE_ID("chain1");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	void createParameters(Array<ParameterData>& data)
	{
		fillInternalParameterList(get<0>(obj), "oscillator1");
		fillInternalParameterList(get<1>(obj), "oscillator2");
		fillInternalParameterList(get<2>(obj), "gain1");

		// Parameter Initalisation =====================================================
		initValues.add({ "oscillator1.Mode", 0.0 });
		initValues.add({ "oscillator1.Frequency", 220.0 });
		initValues.add({ "oscillator2.Mode", 0.0 });
		initValues.add({ "oscillator2.Frequency", 748.0 });
		initValues.add({ "gain1.Gain", 0.0 });
		initValues.add({ "gain1.Smoothing", 20.0 });
		initStaticParameterData();

		// Internal Modulation =========================================================
		// Parameter Callbacks =========================================================
		{
			ParameterData p("Switch");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto oscillator1_Bypassed = getParameter("oscillator1.Bypassed");
			oscillator1_Bypassed.range = { 0.0, 0.5, 0.5, 1.0 };
			auto oscillator2_Bypassed = getParameter("oscillator2.Bypassed");
			oscillator2_Bypassed.range = { 0.5, 1.0, 0.5, 1.0 };

			p.db = [oscillator1_Bypassed, oscillator2_Bypassed, rangeCopy](double newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				oscillator1_Bypassed.setBypass(newValue);
				oscillator2_Bypassed.setBypass(newValue);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Volume");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto gain1_Gain = getParameter("gain1.Gain");
			gain1_Gain.range = { -100.0, 0.0, 0.1, 5.42227 };

			p.db = [gain1_Gain, rangeCopy](double newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				gain1_Gain(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using chain1 = chain1_impl::instance;


#endif







} // }namespace core

class HiseFxNodeFactory : public NodeFactory
{
public:

	HiseFxNodeFactory(DspNetwork* network);;
	Identifier getId() const override { return "core"; }
};

}
