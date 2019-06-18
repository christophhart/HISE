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



namespace meta
{


namespace chain3_impl
{
// Template Alias Definition =======================================================
using chain3_ = container::chain<core::oscillator_poly, core::peak>;

template <int NV> struct instance : public hardcoded<chain3_>
{
	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("chain3");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "oscillator2");
		registerNode(get<1>(obj), "peak1");

		// Parameter Initalisation =========================================================
		setParameterDefault("oscillator2.Mode", 0.0);
		setParameterDefault("oscillator2.Frequency", 220.0);
		setParameterDefault("oscillator2.Freq Ratio", 1.0);

		// Setting node properties =========================================================
		setNodeProperty("oscillator2.UseMidi", false, false);
	}

};

REGISTER_POLY;

}
DEFINE_EXTERN_NODE_TEMPLATE(chain3, chain3_poly, chain3_impl::instance);
DEFINE_EXTERN_NODE_TEMPIMPL(chain3_impl::instance);



namespace chain1_impl
{
// Template Alias Definition =======================================================
using chain1_ = container::frame2_block<container::chain<routing::receive, filters::one_pole, core::fix_delay, routing::send>>;

struct instance : public hardcoded<chain1_>
{
	SET_HISE_NODE_ID("chain1");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0, 0>(obj), "receive4");
		registerNode(get<0, 1>(obj), "one_pole1");
		registerNode(get<0, 2>(obj), "fix_delay1");
		registerNode(get<0, 3>(obj), "send5");

		// Parameter Initalisation =========================================================
		setParameterDefault("receive4.Feedback", 0.55);
		setParameterDefault("one_pole1.Frequency", 1888.9);
		setParameterDefault("one_pole1.Q", 1.0);
		setParameterDefault("one_pole1.Gain", 0.0);
		setParameterDefault("one_pole1.Smoothing", 0.01);
		setParameterDefault("one_pole1.Mode", 0.0);
		setParameterDefault("fix_delay1.DelayTime", 208.7);
		setParameterDefault("fix_delay1.FadeTime", 512.0);

		// Setting node properties =========================================================
		setNodeProperty("receive4.AddToSignal", true, false);
		setNodeProperty("send5.Connection", "receive4", false);
	}

};

}

using chain1 = chain1_impl::instance;


namespace chain2_impl
{
// Template Alias Definition =======================================================
using chain2_ = container::chain<core::gain_poly>;

struct instance : public hardcoded<chain2_>
{
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_ID("chain2");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "gain2");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain2.Gain", 0.0);
		setParameterDefault("gain2.Smoothing", 20.0);

		// Setting node properties =========================================================

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Volume", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("gain2.Gain", { -12.0, 0.0, 0.1, 1.0 });

			p.db = [param_target1, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using chain2 = chain2_impl::instance;


namespace nested_impl
{
// Template Alias Definition =======================================================

using nested_ = container::chain<meta::chain2>;

struct instance : public hardcoded<nested_>
{
	SET_HISE_NODE_ID("nested");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "chain4");

		// Parameter Initalisation =========================================================
		setParameterDefault("chain4.Volume", 0.81);

		// Setting node properties =========================================================
	}

};

}

using nested = nested_impl::instance;




namespace synth1_impl
{
// Template Alias Definition =======================================================
using modchain1_ = container::modchain<core::oscillator_poly>;
using synth1_ = container::synth<wrap::mod<core::midi>, core::oscillator_poly, wrap::mod<core::midi>, modchain1_, core::gain_poly, core::ramp_envelope_poly>;

struct instance : public hardcoded<synth1_>
{
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_ID("synth1");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	
	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "midi2");
		registerNode(get<1>(obj), "oscillator1");
		registerNode(get<2>(obj), "midi1");
		registerNode(get<3>(obj), "oscillator2");
		registerNode(get<4>(obj), "gain1");
		registerNode(get<5>(obj), "ramp_envelope2");

		// Parameter Initalisation =========================================================
		setParameterDefault("midi2.Mode", 3.0);
		setParameterDefault("oscillator1.Mode", 0.0);
		setParameterDefault("oscillator1.Frequency", 123.471);
		setParameterDefault("oscillator1.Freq Ratio", 1.01302);
		setParameterDefault("midi1.Mode", 2.0);
		setParameterDefault("oscillator2.Mode", 0.0);
		setParameterDefault("oscillator2.Frequency", 0.527208);
		setParameterDefault("oscillator2.Freq Ratio", 1.0);
		setParameterDefault("gain1.Gain", -21.9748);
		setParameterDefault("gain1.Smoothing", 4.2);
		setParameterDefault("ramp_envelope2.Gate", 0.0);
		setParameterDefault("ramp_envelope2.Ramp Time", 552.0);

		

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("oscillator1.Frequency", { 20.0, 20000.0, 0.1, 1.0 });
			auto f = [mod_target1](double newValue)
			{
				mod_target1(newValue);
			};


			setInternalModulationParameter(get<0>(obj), f);
		}
		{
			auto mod_target1 = getParameter("oscillator2.Frequency", { 0.4, 10.0, 0.1, 0.229905 });
			auto f = [mod_target1](double newValue)
			{
				mod_target1(newValue);
			};


			setInternalModulationParameter(get<2>(obj), f);
		}
		{
			auto mod_target1 = getParameter("gain1.Gain", { -100.0, 0.0, 0.1, 5.42227 });
			auto mod_target2 = getParameter("oscillator1.Freq Ratio", { 1.0, 1.05, 1.0, 1.0 });
			auto f = [mod_target1, mod_target2](double newValue)
			{
				mod_target1(newValue);
				mod_target2(newValue);
			};


			setInternalModulationParameter(get<3>(obj), f);
		}
	}

};

}

using synth1 = synth1_impl::instance;

namespace synth2_impl
{
// Template Alias Definition =======================================================
using chain1_ = container::chain<core::oscillator_poly, math::sig2mod, math::mul, wrap::mod<core::peak>, math::clear>;
using split1_ = container::split<chain1_, core::oscillator_poly>;
using frame2_block1_ = container::frame2_block<split1_>;
using synth2_ = container::synth<wrap::mod<core::midi>, frame2_block1_, core::ramp_envelope_poly>;

struct instance : public hardcoded<synth2_>
{
	SET_HISE_NODE_ID("synth2");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "midi1");
		registerNode(get<1, 0, 0, 0>(obj), "oscillator5");
		registerNode(get<1, 0, 0, 1>(obj), "sig2mod1");
		registerNode(get<1, 0, 0, 2>(obj), "mul2");
		registerNode(get<1, 0, 0, 3>(obj), "peak1");
		registerNode(get<1, 0, 0, 4>(obj), "clear1");
		registerNode(get<1, 0, 1>(obj), "oscillator4");
		registerNode(get<2>(obj), "ramp_envelope1");

		// Parameter Initalisation =========================================================
		setParameterDefault("midi1.Mode", 3.0);
		setParameterDefault("oscillator5.Mode", 0.0);
		setParameterDefault("oscillator5.Frequency", 123.471);
		setParameterDefault("oscillator5.Freq Ratio", 3.0);
		setParameterDefault("sig2mod1.Value", 1.0);
		setParameterDefault("mul2.Value", 1.0);
		setParameterDefault("clear1.Value", 0.0);
		setParameterDefault("oscillator4.Mode", 0.0);
		setParameterDefault("oscillator4.Frequency", 123.471);
		setParameterDefault("oscillator4.Freq Ratio", 5.67946);
		setParameterDefault("ramp_envelope1.Gate", 0.0);
		setParameterDefault("ramp_envelope1.Ramp Time", 224.0);

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("oscillator4.Frequency", { 20.0, 20000.0, 0.1, 1.0 });
			auto mod_target2 = getParameter("oscillator5.Frequency", { 20.0, 20000.0, 0.1, 1.0 });
			auto f = [mod_target1, mod_target2](double newValue)
			{
				mod_target1(newValue);
				mod_target2(newValue);
			};


			setInternalModulationParameter(get<0>(obj), f);
		}
		{
			auto mod_target1 = getParameter("oscillator4.Freq Ratio", { 1.0, 6.0, 1.0, 1.0 });
			auto f = [mod_target1](double newValue)
			{
				mod_target1(newValue);
			};


			setInternalModulationParameter(get<1, 0, 0, 3>(obj), f);
		}
	}

};

}

using synth2 = synth2_impl::instance;


namespace sine_synth_impl
{
// Template Alias Definition =======================================================
using chain5_ = core::empty;
using chain6_ = container::chain<core::oscillator_poly, core::gain_poly>;
using split2_ = container::split<chain5_, chain6_>;
using sine_synth_ = container::chain<wrap::mod<core::midi_poly>, split2_>;

template <int NV> struct instance : public hardcoded<sine_synth_>
{
	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("sine_synth");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "midi2");
		registerNode(get<1, 1, 0>(obj), "oscillator3");
		registerNode(get<1, 1, 1>(obj), "gain2");

		// Parameter Initalisation =========================================================
		setParameterDefault("midi2.Mode", 0.0);
		setParameterDefault("oscillator3.Mode", 0.0);
		setParameterDefault("oscillator3.Frequency", 220.0);
		setParameterDefault("oscillator3.Freq Ratio", 2.0);
		setParameterDefault("gain2.Gain", -100.0);
		setParameterDefault("gain2.Smoothing", 21.8);

		// Setting node properties =========================================================
		setNodeProperty("midi2.Callback", "undefined", false);
		setNodeProperty("oscillator3.UseMidi", true, false);
		setNodeProperty("gain2.ResetValue", 0, false);
		setNodeProperty("gain2.UseResetValue", true, false);

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });
			auto f = [mod_target1](double newValue)
			{
				mod_target1(newValue);
			};


			setInternalModulationParameter(get<0>(obj), f);
		}

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Harmonic", { 1.0, 16.0, 0.01, 1.0 });

			auto param_target1 = getParameter("oscillator3.Freq Ratio", { 1.0, 16.0, 1.0, 1.0 });

			

			p.db = [param_target1, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}
DEFINE_EXTERN_NODE_TEMPLATE(sine_synth, sine_synth_poly, sine_synth_impl::instance);
DEFINE_EXTERN_NODE_TEMPIMPL(sine_synth_impl::instance);


namespace frame2_block1_impl
{
// Template Alias Definition =======================================================
using chain3_ = container::chain<math::clear, core::oscillator, math::sig2mod, wrap::mod<core::peak>, math::clear>;
using chain1_ = container::chain<routing::receive, stk::delay_a, routing::send>;
using chain2_ = container::chain<routing::receive, stk::delay_a, routing::send>;
using multi1_ = container::multi<fix<1, chain1_>, fix<1, chain2_>>;
using split1_ = container::split<chain3_, multi1_>;
using frame2_block1_ = container::frame2_block<split1_>;

struct instance : public hardcoded<frame2_block1_>
{
	SET_HISE_NODE_ID("frame2_block1");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0, 0, 0>(obj), "clear1");
		registerNode(get<0, 0, 1>(obj), "oscillator1");
		registerNode(get<0, 0, 2>(obj), "sig2mod1");
		registerNode(get<0, 0, 3>(obj), "peak1");
		registerNode(get<0, 0, 4>(obj), "clear2");
		registerNode(get<0, 1, 0, 0>(obj), "receive1");
		registerNode(get<0, 1, 0, 1>(obj), "delay_a1");
		registerNode(get<0, 1, 0, 2>(obj), "send3");
		registerNode(get<0, 1, 1, 0>(obj), "receive2");
		registerNode(get<0, 1, 1, 1>(obj), "delay_a2");
		registerNode(get<0, 1, 1, 2>(obj), "send2");

		// Parameter Initalisation =========================================================
		setParameterDefault("clear1.Value", 0.0);
		setParameterDefault("oscillator1.Mode", 0.0);
		setParameterDefault("oscillator1.Frequency", 0.160709);
		setParameterDefault("oscillator1.Freq Ratio", 1.0);
		setParameterDefault("sig2mod1.Value", 0.0);
		setParameterDefault("clear2.Value", 0.0);
		setParameterDefault("receive1.Feedback", 0.85);
		setParameterDefault("delay_a1.Delay", 4.97568);
		setParameterDefault("receive2.Feedback", 0.85);
		setParameterDefault("delay_a2.Delay", 0.0243211);

		// Setting node properties =========================================================
		setNodeProperty("oscillator1.UseMidi", false, false);
		setNodeProperty("receive1.AddToSignal", true, false);
		setNodeProperty("send3.Connection", "receive1", false);
		setNodeProperty("receive2.AddToSignal", true, false);
		setNodeProperty("send2.Connection", "receive2", false);

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("delay_a1.Delay", { 0.0, 5.0, 0.0001, 1.0 });
			auto mod_target2 = getParameter("delay_a2.Delay", { 0.0, 5.0, 0.0001, 1.0 });
			auto f = [mod_target1, mod_target2](double newValue)
			{
				mod_target1(newValue);
				mod_target2(newValue);
			};


			setInternalModulationParameter(get<0, 0, 3>(obj), f);
		}

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Feedback", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("receive1.Feedback", { 0.0, 1.0, 0.01, 1.0 });
			auto param_target2 = getParameter("receive2.Feedback", { 0.0, 1.0, 0.01, 1.0 });

			p.db = [param_target1, param_target2, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Speed", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("oscillator1.Frequency", { 0.1, 10.0, 0.1, 0.229905 });

			p.db = [param_target1, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using frame2_block1 = frame2_block1_impl::instance;




namespace funkyBoy_impl
{
// Template Alias Definition =======================================================
using funkyBoy_ = container::chain<core::gain>;

struct instance : public hardcoded<funkyBoy_>
{
	SET_HISE_NODE_ID("funkyBoy");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	String getSnippetText() const override
	{
		return "349.3ocqQ9rRCCDDFe1VWEsBhH9ZTPnmsgZQoPSIzzJ3w0zwjklraXyFqwWBOqW7UvqdPvW.O4Cjtaq+osof8f6gcX9X9l42tSO4HDHzNsAxNzqxEiKZIKfSYAZopvioi.x9z.oPy3BTUOHxDgd4ImDwDBLNy3kTA.nqLX7BxjpsJRYYY8YIoC3InqQa6MfomFNjpdLEKA0nJyHzyPgIRH89EmsnglYczhrTyvhBqG9eQgqbTdLSykhALUHpsv3ojonRymRTkuxJ.RUKU6R6iYn9bVbNByt+ogFq4WFyCriEVx2dzgY35ZctuFC.emYpti3ZSOwQ1xb4hkZxEu5xtoTi6Jmfpt7Dtd9JGllVRE.eMl5yucp+Gt2ddto+XbxrcfU83fWdK7oCcruoMomY2CklHr9HCPIjctyYUHCvJP1T6ei7GEO5cP62aZQtF0OQJ0QbQ3xb2vYgMO7IbP3xa.";
	}

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "gain1");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain1.Gain", 0.0);
		setParameterDefault("gain1.Smoothing", 20.0);

		// Setting node properties =========================================================
		setNodeProperty("gain1.ResetValue", 0, false);
		setNodeProperty("gain1.UseResetValue", 0, false);
	}

};

}

using funkyBoy = funkyBoy_impl::instance;



#if 0
namespace flanger_impl
{
// Template Alias Definition =======================================================
using modchain1_ = container::chain<core::oscillator, math::sig2mod, wrap::mod<core::peak>>;
using multi1_ = container::multi<fix<1, stk::delay_a>, fix<1, stk::delay_a>>;
using chain1_ = container::chain<routing::receive, multi1_, math::tanh, routing::send, core::gain>;
using split1_ = container::split<core::gain, modchain1_, chain1_>;
using flanger_ = container::frame2_block<split1_>;

struct instance : public hardcoded<flanger_>
{
	SET_HISE_NODE_ID("flanger");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0, 0>(obj), "gain1");
		registerNode(get<0, 1, 0>(obj), "oscillator2");
		registerNode(get<0, 1, 1>(obj), "sig2mod1");
		registerNode(get<0, 1, 2>(obj), "peak1");
		registerNode(get<0, 2, 0>(obj), "receive1");
		registerNode(get<0, 2, 1, 0>(obj), "delay_a1");
		registerNode(get<0, 2, 1, 1>(obj), "delay_a5");
		registerNode(get<0, 2, 2>(obj), "tanh1");
		registerNode(get<0, 2, 3>(obj), "send1");
		registerNode(get<0, 2, 4>(obj), "gain2");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain1.Gain", -100.0);
		setParameterDefault("gain1.Smoothing", 20.0);
		setParameterDefault("oscillator2.Mode", 0.0);
		setParameterDefault("oscillator2.Frequency", 0.4753);
		setParameterDefault("oscillator2.Freq Ratio", 1.0);
		setParameterDefault("sig2mod1.Value", 0.0);
		setParameterDefault("receive1.Feedback", 0.72);
		setParameterDefault("delay_a1.Delay", 1.09292);
		setParameterDefault("delay_a5.Delay", 11.9071);
		setParameterDefault("tanh1.Value", 1.0);
		setParameterDefault("gain2.Gain", 0.0);
		setParameterDefault("gain2.Smoothing", 20.0);

		// Setting node properties =========================================================
		setNodeProperty("gain1.ResetValue", 0, false);
		setNodeProperty("gain1.UseResetValue", 0, false);
		setNodeProperty("oscillator2.UseMidi", false, false);
		setNodeProperty("receive1.AddToSignal", true, false);
		setNodeProperty("send1.Connection", "receive1", false);
		setNodeProperty("gain2.ResetValue", 0, false);
		setNodeProperty("gain2.UseResetValue", 0, false);

		// Internal Modulation =============================================================
		{
			auto mod_target1 = getParameter("delay_a1.Delay", { 1.0, 12.0, 0.0001, 1.0 });
			auto mod_target2 = getParameter("delay_a5.Delay", { 1.0, 12.0, 0.0001, 1.0 });
			auto f = [mod_target1, mod_target2](double newValue)
			{
				mod_target1(newValue);
				mod_target2(newValue);
			};


			setInternalModulationParameter(get<0, 1, 2>(obj), f);
		}

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Amount", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("gain1.Gain", { -100.0, 0.0, 0.1, 5.42227 });
			auto param_target2 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });

			p.db = [param_target1, param_target2, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Rate", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("oscillator2.Frequency", { 0.01, 1.0, 0.0001, 1.0 });

			p.db = [param_target1, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
			};


			data.add(std::move(p));
		}
		{
			ParameterData p("Feedback", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("receive1.Feedback", { 0.0, 1.0, 0.01, 1.0 });

			p.db = [param_target1, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
			};


			data.add(std::move(p));
		}
	}

};

}

using flanger = flanger_impl::instance;
#endif

DEFINE_FACTORY_FOR_NAMESPACE;


}



}
