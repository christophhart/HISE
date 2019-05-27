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



namespace oversample8x1_impl
{
// Template Alias Definition =======================================================
using chain2_ = container::chain<core::oscillator, core::gain>;
using frame2_block1_ = container::frame2_block<chain2_>;
using oversample8x1_ = container::oversample8x<frame2_block1_>;

struct instance : public wr::one::parameter<oversample8x1_>
{
	SET_HISE_NODE_ID("oversample8x1");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		fillInternalParameterList(get<0, 0, 0>(obj), "oscillator3");
		fillInternalParameterList(get<0, 0, 1>(obj), "gain2");

		// Parameter Initalisation =====================================================
		initValues.add({ "oscillator3.Mode", 0.0 });
		initValues.add({ "oscillator3.Frequency", 100.0 });
		initValues.add({ "gain2.Gain", -2.165 });
		initValues.add({ "gain2.Smoothing", 20.0 });
		initStaticParameterData();

		// Internal Modulation =========================================================
		// Parameter Callbacks =========================================================
		{
			ParameterData p("Freq");
			p.range = { 0.0, 1.0, 0.01, 1.0 };
			auto rangeCopy = p.range;

			auto oscillator3_Frequency = getParameter("oscillator3.Frequency");
			oscillator3_Frequency.range = { 100.0, 120.0, 0.1, 0.229905 };

			p.db = [oscillator3_Frequency, rangeCopy](double newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				oscillator3_Frequency(normalised);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Vol");
			p.range = { 0.0, 1.0, 0.01, 1.0 };
			auto rangeCopy = p.range;

			auto gain2_Gain = getParameter("gain2.Gain");
			gain2_Gain.range = { -12.0, 0.0, 0.1, 5.42227 };

			p.db = [gain2_Gain, rangeCopy](double newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				gain2_Gain(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using oversample8x1 = oversample8x1_impl::instance;



namespace chain4_impl
{
// Template Alias Definition =======================================================
using chain4_ = container::chain<bypass::smoothed<core::oscillator>, bypass::smoothed<core::oscillator>>;

struct instance : public wr::one::parameter<chain4_>
{
	SET_HISE_NODE_ID("chain4");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	void createParameters(Array<ParameterData>& data)
	{
		fillInternalParameterList(get<0>(obj), "oscillator1");
		fillInternalParameterList(get<1>(obj), "oscillator2");

		// Parameter Initalisation =====================================================
		initValues.add({ "oscillator1.Mode", 0.0 });
		initValues.add({ "oscillator1.Frequency", 220.0 });
		initValues.add({ "oscillator2.Mode", 0.0 });
		initValues.add({ "oscillator2.Frequency", 775.5 });
		initStaticParameterData();

		// Internal Modulation =========================================================
		// Parameter Callbacks =========================================================
		{
#if 0
			ParameterData p("Switch");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto oscillator1_Bypassed = getParameter("oscillator1.Bypassed");
			oscillator1_Bypassed.range = { 1.0, 2.0, 0.5, 1.0 };
			auto oscillator2_Bypassed = getParameter("oscillator2.Bypassed");
			oscillator2_Bypassed.range = { 2.0, 3.0, 0.5, 1.0 };

			p.db = [oscillator1_Bypassed, oscillator2_Bypassed, rangeCopy](double newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				oscillator1_Bypassed(normalised);
				oscillator2_Bypassed(normalised);
			};
#endif

			ParameterData p("Switch");
			p.range = { 0.0, 3.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto oscillator1_Bypassed = getParameter("oscillator1.Bypassed");
			oscillator1_Bypassed.range = { 1.0, 2.0, 0.5, 1.0 };
			auto oscillator2_Bypassed = getParameter("oscillator2.Bypassed");
			oscillator2_Bypassed.range = { 2.0, 3.0, 0.5, 1.0 };

			p.db = [oscillator1_Bypassed, oscillator2_Bypassed, rangeCopy](double newValue)
			{
				oscillator1_Bypassed.setBypass(newValue);
				oscillator2_Bypassed.setBypass(newValue);
			};


			data.add(std::move(p));
		}
	}

};

}

using chain4 = chain4_impl::instance;



















} // }namespace core

class HiseFxNodeFactory : public NodeFactory
{
public:

	HiseFxNodeFactory(DspNetwork* network);;
	Identifier getId() const override { return "core"; }
};

}
